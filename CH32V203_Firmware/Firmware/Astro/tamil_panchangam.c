#include "tamil_panchangam.h"
#include "text_util.h"
#include "storage.h"
#include "ff.h"
#include <string.h>

/* Compact 2000..2050 astronomy engine. Angles are degrees and time is days
 * from J2000 noon. Keeping the epoch small lets binary32 retain sub-minute
 * resolution without pulling double-precision libm into the firmware. */
#define PI_F       3.14159265358979323846f
#define RAD_F      (PI_F / 180.0f)
#define DEG_F      (180.0f / PI_F)
#define EVENT_ALT  (-0.8333f)

typedef float AstroDay;

typedef struct {
    float lon;
    float lat;
} MoonPosition;

typedef struct {
    int8_t d, m, mp, f;
    int16_t milli_deg;
} MoonTerm;

/* Leading Meeus/ELP2000-82 terms from the ESP golden implementation.
 * Millidegree coefficients keep the consumed Panchang boundaries accurate
 * without restoring its 120-entry double-precision tables. */
static const MoonTerm moon_lon_terms[] = {
    { 0, 0, 1, 0, 6289}, { 2, 0,-1, 0, 1274}, { 2, 0, 0, 0,  658},
    { 0, 0, 2, 0,  214}, { 0, 1, 0, 0, -185}, { 0, 0, 0, 2, -114},
    { 2, 0,-2, 0,   59}, { 2,-1,-1, 0,   57}, { 2, 0, 1, 0,   53},
    { 2,-1, 0, 0,   46}, { 0, 1,-1, 0,  -41}, { 1, 0, 0, 0,  -35},
    { 0, 1, 1, 0,  -30}, { 2, 0, 0, 0,   15}, { 0, 0, 1,-2,  -13},
    { 0, 0, 1, 2,   11}, { 4, 0,-1,-2,   11}, { 0, 0, 3, 0,   10},
    { 4, 0,-2, 0,    9}, { 2, 1,-1, 0,   -8}, { 2, 1, 0, 0,   -7},
    { 1, 0, 1, 0,   -5}, { 1, 1, 0, 0,    5}, { 2,-1,-1, 0,    4},
    {-1, 0, 0, 0,    4}, { 2, 1,-1, 0,    4}, { 0, 0,-2, 0,    4},
    { 0, 0, 2, 2,   -3}, { 2,-1,-2, 0,   -3}, {-1,-1, 1, 0,    2}
};

static const MoonTerm moon_lat_terms[] = {
    {0, 0, 0, 1, 5128}, {0, 0, 1, 1,  281}, {0, 0, 1,-1,  274},
    {2, 0,-1,-1,  252}, {2, 0, 0, 1,   59},
    {0, 0,-1, 1,   17}, {2,-1, 0,-1,    9},
    {0, 0, 0,-1,    9}, {2,-1, 1, 1,    8}, {2,-1,-1,-1,    4},
    {2,-1,-1, 1,   -3}, {2, 0, 0,-1,    4}, {2, 0, 2,-1,    2},
    {4, 0, 2, 1,    2}
};

static float norm360(float v)
{
    int32_t turns = (int32_t)(v / 360.0f);
    v -= (float)turns * 360.0f;
    if (v < 0.0f) v += 360.0f;
    return v;
}

static float norm180(float v)
{
    v = norm360(v);
    return v > 180.0f ? v - 360.0f : v;
}

/* Preserve sub-arcminute phase precision in binary32 across 2000..2050.
 * Reducing the whole-number rotations before adding the fractional rate avoids
 * forming rate*day products near 240,000 degrees. */
static float linear_angle(float base, float daily_rate, AstroDay day)
{
    int32_t whole_day = (int32_t)day;
    int32_t whole_rate = (int32_t)(daily_rate + 0.5f);
    float angle = base + (float)((whole_day * whole_rate) % 360);
    angle += (daily_rate - (float)whole_rate) * (float)whole_day;
    angle += daily_rate * (day - (float)whole_day);
    return norm360(angle);
}

static float norm_hour(float v)
{
    while (v < 0.0f) v += 24.0f;
    while (v >= 24.0f) v -= 24.0f;
    return v;
}

/* Seventh-order sine after quadrant reduction. Maximum absolute polynomial
 * error over [-pi/2,pi/2] is below 1.6e-4. */
static float small_sin(float degrees)
{
    float x, x2;
    degrees = norm180(degrees);
    if (degrees > 90.0f) degrees = 180.0f - degrees;
    else if (degrees < -90.0f) degrees = -180.0f - degrees;
    x = degrees * RAD_F;
    x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f +
           x2 * (0.0083333310f - x2 * 0.0001984090f)));
}

static float small_cos(float degrees)
{
    return small_sin(degrees + 90.0f);
}

static float small_sqrt(float x)
{
    union { float f; uint32_t u; } seed;
    float y;
    int i;
    if (x <= 0.0f) return 0.0f;
    seed.f = x;
    seed.u = (seed.u >> 1) + 0x1fc00000U;
    y = seed.f;
    for (i = 0; i < 3; i++) y = 0.5f * (y + x / y);
    return y;
}

static float atan_unit(float z)
{
    float z2 = z * z;
    return z * (0.9998660f + z2 * (-0.3302995f +
           z2 * (0.1801410f + z2 * (-0.0851330f +
           z2 * 0.0208351f))));
}

static float small_atan2(float y, float x)
{
    float ax = x < 0.0f ? -x : x;
    float ay = y < 0.0f ? -y : y;
    float a;
    if (ax == 0.0f && ay == 0.0f) return 0.0f;
    if (ax >= ay) a = atan_unit(ay / ax);
    else a = PI_F * 0.5f - atan_unit(ax / ay);
    if (x < 0.0f) a = PI_F - a;
    if (y < 0.0f) a = -a;
    return a;
}

static float small_asin(float x)
{
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return small_atan2(x, small_sqrt(1.0f - x * x));
}

static float small_acos(float x)
{
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return small_atan2(small_sqrt(1.0f - x * x), x);
}

static int32_t day_number(int y, int m, int d)
{
    int a = (14 - m) / 12;
    int yy = y + 4800 - a;
    int mm = m + 12 * a - 3;
    return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 -
           yy / 100 + yy / 400 - 32045 - 2451545;
}

static AstroDay astro_day(int y, int m, int d, float utc_hour)
{
    return (float)day_number(y, m, d) - 0.5f + utc_hour / 24.0f;
}

static AstroDay local_day(DateTime dt, Location loc, float local_hour)
{
    return astro_day(dt.year, dt.month, dt.day, local_hour - loc.tz);
}

static float local_hour(DateTime dt, Location loc, AstroDay day)
{
    return norm_hour((day - astro_day(dt.year, dt.month, dt.day, 0.0f)) *
                     24.0f + loc.tz);
}

static float ayanamsa(AstroDay day)
{
    float t = day / 36525.0f;
    return 23.855563f + 1.3965636f * t + 0.0000139f * t * t;
}

static float sun_longitude(AstroDay day)
{
    float g = linear_angle(357.5291f, 0.98560028f, day);
    float q = linear_angle(280.459f, 0.98564736f, day);
    return norm360(q + 1.915f * small_sin(g) +
                   0.020f * small_sin(2.0f * g));
}

static void moon_position(AstroDay day, MoonPosition *p)
{
    float ut_century = day / 36525.0f;
    float year_offset = ut_century * 100.0f;
    float t;
    day += (62.92f + 0.32217f * year_offset +
            0.005589f * year_offset * year_offset) / 86400.0f;
    t = day / 36525.0f;
    float t2 = t * t;
    float lp = norm360(linear_angle(218.3164477f, 13.1763964744f, day) - 0.0015786f * t2);
    float elong = norm360(linear_angle(297.8501921f, 12.1907491199f, day) - 0.0018819f * t2);
    float sun_m = norm360(linear_angle(357.5291092f, 0.98560028175f, day) - 0.0001536f * t2);
    float moon_m = norm360(linear_angle(134.9633964f, 13.0649929502f, day) + 0.0087414f * t2);
    float arg_lat = norm360(linear_angle(93.2720950f, 13.2293502401f, day) - 0.0036539f * t2);
    float eccentricity = 1.0f - 0.002516f * t - 0.0000074f * t2;
    float lon_sum = 0.0f, lat_sum = 0.0f;
    unsigned i;

    for (i = 0; i < sizeof(moon_lon_terms) / sizeof(moon_lon_terms[0]); i++) {
        const MoonTerm *term = &moon_lon_terms[i];
        float scale = term->m ? eccentricity : 1.0f;
        if (term->m == 2 || term->m == -2) scale *= eccentricity;
        lon_sum += (float)term->milli_deg * scale * small_sin(
            term->d * elong + term->m * sun_m + term->mp * moon_m + term->f * arg_lat);
    }
    for (i = 0; i < sizeof(moon_lat_terms) / sizeof(moon_lat_terms[0]); i++) {
        const MoonTerm *term = &moon_lat_terms[i];
        float scale = term->m ? eccentricity : 1.0f;
        if (term->m == 2 || term->m == -2) scale *= eccentricity;
        lat_sum += (float)term->milli_deg * scale * small_sin(
            term->d * elong + term->m * sun_m + term->mp * moon_m + term->f * arg_lat);
    }
    p->lon = norm360(lp + lon_sum * 0.001f +
                     0.003958f * small_sin(119.75f + 131.849f * t));
    p->lat = lat_sum * 0.001f;
}

static float sidereal_sun(AstroDay day)
{
    return norm360(sun_longitude(day) - ayanamsa(day));
}

static float sidereal_moon(AstroDay day)
{
    MoonPosition moon;
    moon_position(day, &moon);
    return norm360(moon.lon - ayanamsa(day));
}

static float sun_event(DateTime dt, Location loc, int rising)
{
    AstroDay base = astro_day(dt.year, dt.month, dt.day, 0.0f);
    float adjustment = 0.0f, utc_minutes = 0.0f;
    int i;
    for (i = 0; i < 2; i++) {
        AstroDay event_day = base + adjustment;
        float t = event_day / 36525.0f;
        float mean_lon = norm360(linear_angle(280.46646f, 0.98564736f, event_day) +
                                 0.0003032f * t * t);
        float anomaly = norm360(linear_angle(357.52911f, 0.98560028f, event_day) -
                                0.0001537f * t * t);
        float eccentricity = 0.016708634f - t * (0.000042037f + 0.0000001267f * t);
        float omega = 125.04f - 1934.136f * t;
        float seconds = 21.448f - t * (46.815f + t * (0.00059f - t * 0.001813f));
        float obliquity = 23.0f + (26.0f + seconds / 60.0f) / 60.0f +
                          0.00256f * small_cos(omega);
        float tan_half = small_sin(obliquity * 0.5f) / small_cos(obliquity * 0.5f);
        float y = tan_half * tan_half;
        float eq_minutes = 4.0f * DEG_F *
            (y * small_sin(2.0f * mean_lon) - 2.0f * eccentricity * small_sin(anomaly) +
             4.0f * eccentricity * y * small_sin(anomaly) * small_cos(2.0f * mean_lon) -
             0.5f * y * y * small_sin(4.0f * mean_lon) -
             1.25f * eccentricity * eccentricity * small_sin(2.0f * anomaly));
        float center = small_sin(anomaly) *
                       (1.914602f - t * (0.004817f + 0.000014f * t)) +
                       small_sin(2.0f * anomaly) * (0.019993f - 0.000101f * t) +
                       small_sin(3.0f * anomaly) * 0.000289f;
        float apparent_lon = mean_lon + center - 0.00569f -
                             0.00478f * small_sin(omega);
        float decl = small_asin(small_sin(obliquity) * small_sin(apparent_lon)) * DEG_F;
        float h = (small_sin(-0.7891071f) - small_sin(loc.lat) * small_sin(decl)) /
                  (small_cos(loc.lat) * small_cos(decl));
        float hour_angle = small_acos(h) * DEG_F;
        float delta = -loc.lon + (rising ? -hour_angle : hour_angle);
        utc_minutes = 720.0f + delta * 4.0f - eq_minutes;
        if (utc_minutes < 0.0f) utc_minutes += 1440.0f;
        adjustment = utc_minutes / 1440.0f;
    }
    return norm_hour(utc_minutes / 60.0f + loc.tz);
}

static float moon_altitude(AstroDay day, Location loc)
{
    MoonPosition moon;
    float eps = 23.4393f - 0.00000036f * day;
    float ra, dec, lst, ha, s;
    moon_position(day, &moon);
    ra = small_atan2(small_sin(moon.lon) * small_cos(eps) -
                     (small_sin(moon.lat) / small_cos(moon.lat)) * small_sin(eps),
                     small_cos(moon.lon)) * DEG_F;
    dec = small_asin(small_sin(moon.lat) * small_cos(eps) +
                     small_cos(moon.lat) * small_sin(eps) * small_sin(moon.lon)) * DEG_F;
    lst = norm360(linear_angle(280.460618f, 360.985647f, day) + loc.lon);
    ha = norm180(lst - ra);
    s = small_sin(loc.lat) * small_sin(dec) +
        small_cos(loc.lat) * small_cos(dec) * small_cos(ha);
    return small_asin(s) * DEG_F;
}

static void moon_events(DateTime dt, Location loc, float *rise, float *set)
{
    float prev_t = 0.0f;
    float prev = moon_altitude(local_day(dt, loc, 0.0f), loc);
    int got_rise = 0, got_set = 0, i;
    *rise = *set = 0.0f;
    for (i = 1; i <= 144; i++) {
        float t = (float)i / 6.0f;
        float alt = moon_altitude(local_day(dt, loc, t), loc);
        int rising = prev < EVENT_ALT && alt >= EVENT_ALT;
        int setting = prev > EVENT_ALT && alt <= EVENT_ALT;
        if ((!got_rise && rising) || (!got_set && setting)) {
            float lo = prev_t, hi = t;
            int j;
            for (j = 0; j < 8; j++) {
                float mid = (lo + hi) * 0.5f;
                float ma = moon_altitude(local_day(dt, loc, mid), loc);
                if ((rising && ma >= EVENT_ALT) || (setting && ma <= EVENT_ALT)) hi = mid;
                else lo = mid;
            }
            if (rising) { *rise = norm_hour((lo + hi) * 0.5f); got_rise = 1; }
            else { *set = norm_hour((lo + hi) * 0.5f); got_set = 1; }
        }
        if (got_rise && got_set) break;
        prev = alt;
        prev_t = t;
    }
}

static int tithi_at(AstroDay day)
{
    return (int)(norm360(sidereal_moon(day) - sidereal_sun(day)) / 12.0f) + 1;
}

static int nakshatra_at(AstroDay day)
{
    return (int)(sidereal_moon(day) / (360.0f / 27.0f)) + 1;
}

static int yoga_at(AstroDay day)
{
    return (int)(norm360(sidereal_sun(day) + sidereal_moon(day)) /
                 (360.0f / 27.0f)) + 1;
}

static int karana_at(AstroDay day)
{
    int half = (int)(norm360(sidereal_moon(day) - sidereal_sun(day)) / 6.0f);
    if (half == 0) return 11;
    if (half >= 57) return 8 + half - 57;
    return ((half - 1) % 7) + 1;
}

typedef int (*ValueAt)(AstroDay);

static float change_time(DateTime dt, Location loc, AstroDay start,
                         int initial, ValueAt value_at)
{
    AstroDay prev = start;
    AstroDay end = start + 1.5f;
    AstroDay day;
    for (day = start + 1.0f / 144.0f; day <= end; day += 1.0f / 144.0f) {
        if (value_at(day) != initial) {
            AstroDay lo = prev, hi = day;
            int i;
            for (i = 0; i < 8; i++) {
                AstroDay mid = (lo + hi) * 0.5f;
                if (value_at(mid) == initial) lo = mid; else hi = mid;
            }
            return local_hour(dt, loc, hi);
        }
        prev = day;
    }
    return 0.0f;
}

static int weekday(int y, int m, int d)
{
    int k, j, h;
    if (m < 3) { m += 12; y--; }
    k = y % 100; j = y / 100;
    h = (d + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (h + 6) % 7;
}

static int days_in_month(int y, int m)
{
    static const uint8_t days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return days[m - 1];
}

static void previous_date(int *y, int *m, int *d)
{
    if (--*d > 0) return;
    if (--*m == 0) { *m = 12; --*y; }
    *d = days_in_month(*y, *m);
}

static void segment(float rise, float set, int index, float *start, float *end)
{
    float part = (set - rise) / 8.0f;
    if (part < 0.0f) part = -part;
    *start = rise + part * (float)(index - 1);
    *end = *start + part;
}

static int solar_month_at_sunset(int y, int m, int d, Location loc)
{
    DateTime dt = {y, m, d, 0.0f};
    float sunset = sun_event(dt, loc, 0);
    return (int)(sidereal_sun(local_day(dt, loc, sunset)) / 30.0f);
}

static void solar_calendar(DateTime dt, Location loc, Panchang *p)
{
    int month = solar_month_at_sunset(dt.year, dt.month, dt.day, loc);
    int y = dt.year, m = dt.month, d = dt.day, count = 1;
    while (count < 32) {
        int py = y, pm = m, pd = d;
        previous_date(&py, &pm, &pd);
        if (solar_month_at_sunset(py, pm, pd, loc) != month) break;
        y = py; m = pm; d = pd; count++;
    }
    p->month = month;
    p->day = count;
}

static AstroDay previous_new_moon(AstroDay day)
{
    float diff = norm360(sidereal_moon(day) - sidereal_sun(day));
    AstroDay estimate = day - diff * (29.5306f / 360.0f);
    int i;
    for (i = 0; i < 3; i++)
        estimate -= norm180(sidereal_moon(estimate) - sidereal_sun(estimate)) / 12.19f;
    return estimate;
}

static AstroDay sunrise_day(int y, int m, int d, Location loc)
{
    DateTime dt = {y, m, d, 0.0f};
    return local_day(dt, loc, sun_event(dt, loc, 1));
}

static int lunar_month_start(int y, int m, int d, Location loc)
{
    int py = y, pm = m, pd = d;
    previous_date(&py, &pm, &pd);
    return tithi_at(sunrise_day(py, pm, pd, loc)) > tithi_at(sunrise_day(y, m, d, loc));
}

static int solar_year_index(DateTime dt)
{
    int d, after = 0;
    for (d = 13; d <= 15; d++) {
        AstroDay a = astro_day(dt.year, 4, d, 0.0f);
        if (sidereal_sun(a) > 330.0f && sidereal_sun(a + 1.0f) < 30.0f) {
            after = day_number(dt.year, dt.month, dt.day) >= day_number(dt.year, 4, d + 1);
            break;
        }
    }
    d = 1 + dt.year - 1987 - (after ? 0 : 1);
    d %= 60;
    return d <= 0 ? d + 60 : d;
}

static int lunar_year_index(DateTime dt, Location loc)
{
    int m, d, after = 0;
    for (m = 3; m <= 4 && !after; m++) {
        int first = m == 3 ? 15 : 1;
        int last = m == 3 ? 31 : 10;
        for (d = first; d <= last; d++) {
            AstroDay day = sunrise_day(dt.year, m, d, loc);
            if ((int)(sidereal_sun(day) / 30.0f) + 1 == 12 &&
                lunar_month_start(dt.year, m, d, loc)) {
                after = day_number(dt.year, dt.month, dt.day) >= day_number(dt.year, m, d);
                m = 5;
                break;
            }
        }
    }
    d = 1 + dt.year - 1987 - (after ? 0 : 1);
    d %= 60;
    return d <= 0 ? d + 60 : d;
}

static void lunar_calendar(AstroDay day, int *month, int *date)
{
    AstroDay new_moon = previous_new_moon(day);
    *month = ((int)(sidereal_sun(new_moon) / 30.0f) + 1) % 12;
    *date = tithi_at(day);
}

static void bengali_calendar(DateTime dt, int *month, int *date)
{
    int start_year = (dt.month > 4 || (dt.month == 4 && dt.day >= 14)) ?
                     dt.year : dt.year - 1;
    int next = start_year + 1;
    int leap = (next % 4 == 0 && (next % 100 != 0 || next % 400 == 0));
    static const uint8_t base[12] = {31,31,31,31,31,30,30,30,30,30,30,30};
    int doy = day_number(dt.year, dt.month, dt.day) - day_number(start_year, 4, 14);
    int i;
    for (i = 0; i < 12; i++) {
        int length = base[i] + (i == 10 && leap);
        if (doy < length) break;
        doy -= length;
    }
    *month = i < 12 ? i : 11;
    *date = doy + 1;
}

static void detect_special_day(DateTime dt, Panchang *p)
{
    FIL f;
    char buf[64];
    int values[3];
    p->special_day = DAY_NONE;
    if (f_open(&f, "special.txt", FA_READ) != FR_OK &&
        f_open(&f, "specialday.txt", FA_READ) != FR_OK) return;
    while (Storage_ReadLine(&f, buf, sizeof(buf))) {
        if (Text_ParseIntList(buf, values, 3, ", ") == 3 &&
            values[0] == dt.month && values[1] == dt.day) {
            p->special_day = values[2];
            break;
        }
    }
    f_close(&f);
}

static void detect_festival(Panchang *p, int festival_month)
{
    FIL f;
    char buf[112];
    p->festival = FEST_NONE;
    if (f_open(&f, "festival.txt", FA_READ) != FR_OK) return;
    while (Storage_ReadLine(&f, buf, sizeof(buf))) {
        int values[6] = {0};
        int count = Text_ParseIntList(buf, values, 6, ", \t");
        int regional_day = 0, id;
        if (count == 6) { regional_day = values[4]; id = values[5]; }
        else if (count == 5) id = values[4];
        else continue;
        if (values[0] != 255 && values[0] != festival_month) continue;
        if (values[1] && values[1] != p->tithi) continue;
        if (values[2] && values[2] != p->nakshatra) continue;
        if (values[3] != 255 && values[3] && values[3] != p->sun_sign) continue;
        if (regional_day && regional_day != p->day) continue;
        p->festival = id;
        break;
    }
    f_close(&f);
}

float round5(float time)
{
    int minutes = (int)(time * 60.0f + 0.5f);
    minutes = ((minutes + 2) / 5) * 5;
    if (minutes >= 1440) minutes -= 1440;
    return (float)minutes / 60.0f;
}

void __attribute__((noinline)) calc_panchang(DateTime dt, Location loc, Panchang *p)
{
    static const float nalla_ms[7] = {7.5f,6.5f,7.5f,9.5f,10.5f,9.5f,7.5f};
    static const float nalla_me[7] = {9,8,9,11,12,11,9};
    static const float nalla_es[7] = {15.5f,16.5f,16.5f,16.5f,16.5f,16.5f,16.5f};
    static const float nalla_ee[7] = {17,18,18,18,18,18,18};
    static const uint8_t rahu[7] = {8,2,7,5,6,4,3};
    static const uint8_t yama[7] = {5,4,3,2,1,7,6};
    static const uint8_t guli[7] = {7,6,5,4,3,2,1};
    static const uint8_t durm1[7] = {14,12,4,8,6,3,1};
    int wd, solar_month, solar_date, index;
    float sun, moon, day_length, unit;
    AstroDay rise_day;

    memset(p, 0, sizeof(*p));
    p->lunar_month = p->lunar_day = -1;
    p->sunrise = sun_event(dt, loc, 1);
    p->sunset = sun_event(dt, loc, 0);
    if (p->sunrise > p->sunset) {
        float swap = p->sunrise; p->sunrise = p->sunset; p->sunset = swap;
    }
    moon_events(dt, loc, &p->moonrise, &p->moonset);
    rise_day = local_day(dt, loc, p->sunrise);
    sun = sidereal_sun(rise_day);
    moon = sidereal_moon(rise_day);
    p->tithi = tithi_at(rise_day);
    p->nakshatra = nakshatra_at(rise_day);
    p->yoga = yoga_at(rise_day);
    p->karana1 = karana_at(rise_day);
    p->moon_sign = (int)(moon / 30.0f) + 1;
    p->sun_sign = (int)(sidereal_sun(local_day(dt, loc, p->sunset)) / 30.0f) + 1;
    p->tithi_end = change_time(dt, loc, rise_day, p->tithi, tithi_at);
    p->nak_end = change_time(dt, loc, rise_day, p->nakshatra, nakshatra_at);

    wd = weekday(dt.year, dt.month, dt.day);
    segment(p->sunrise, p->sunset, rahu[wd], &p->rahu_start, &p->rahu_end);
    segment(p->sunrise, p->sunset, guli[wd], &p->gulikai_start, &p->gulikai_end);
    segment(p->sunrise, p->sunset, yama[wd], &p->yama_start, &p->yama_end);
    p->nalla_m_start = norm_hour(nalla_ms[wd] + p->sunrise - 6.0f);
    p->nalla_m_end = norm_hour(nalla_me[wd] + p->sunrise - 6.0f);
    p->nalla_e_start = norm_hour(nalla_es[wd] + p->sunset - 18.0f);
    p->nalla_e_end = norm_hour(nalla_ee[wd] + p->sunset - 18.0f);
    p->fixed_nalla_m_start = nalla_ms[wd];
    p->fixed_nalla_m_end = nalla_me[wd];
    p->fixed_nalla_e_start = nalla_es[wd];
    p->fixed_nalla_e_end = nalla_ee[wd];
    day_length = p->sunset - p->sunrise;
    unit = day_length / 15.0f;
    index = durm1[wd];
    p->dur1_start = p->sunrise + (float)(index - 1) * unit;
    p->dur1_end = p->dur1_start + unit;
    p->ayanam = (sun >= 270.0f || sun < 90.0f) ? 0 : 1;
    p->rithu = ((int)(sun / 30.0f)) / 2;

    solar_calendar(dt, loc, p);
    solar_month = p->month;
    solar_date = p->day;
    detect_special_day(dt, p);
    detect_festival(p, solar_month);
    lunar_calendar(rise_day, &p->lunar_month, &p->lunar_day);
    if (loc.calender == CALENDER_LUNAR) {
        p->samvatsaram = lunar_year_index(dt, loc);
        p->month = p->lunar_month;
        p->day = p->lunar_day;
        if (p->month >= 0) p->rithu = p->month / 2;
    } else if (loc.calender == CALENDER_BENGALI) {
        p->samvatsaram = solar_year_index(dt);
        bengali_calendar(dt, &p->month, &p->day);
    } else {
        p->samvatsaram = solar_year_index(dt);
        p->month = solar_month;
        p->day = solar_date;
    }
}
