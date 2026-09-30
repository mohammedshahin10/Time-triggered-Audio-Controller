#ifndef TAMIL_PANCHANGAM_H
#define TAMIL_PANCHANGAM_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CALENDER_SOLAR 0
//Tamil / Kerala / Punjab / Punjabi / Assamese / Odia
#define CALENDER_LUNAR 1 
//Telugu / Kannada / Marathi / Gujarati / North Indian
#define CALENDER_BENGALI 2

#define FEST_NONE 0
#define DAY_NONE 0

typedef struct {
    int year;
    int month;
    int day;
    float hour;
} DateTime;

typedef struct {
    float lat;
    float lon;
    /* Hours east of UTC; India IST = 5.5. settings.txt accepts 5.5, 5:30, 5h30, 5.30, 5,30
     * (dot/comma + two minute digits = hours+minutes, not decimal hours). */
    float tz;
    int calender;
} Location;

typedef struct {
    float sunrise;
    float sunset;

    float moonrise;
    float moonset;

    int tithi;
    float tithi_end;

    int nakshatra;
    float nak_end;

    int yoga;
    int karana1;

    int sun_sign;
    int moon_sign;

    float rahu_start;
    float rahu_end;

    float gulikai_start;
    float gulikai_end;

    float yama_start;
    float yama_end;

    float dur1_start;
    float dur1_end;

    float nalla_m_start;
    float nalla_m_end;

    float nalla_e_start;
    float nalla_e_end;

    float fixed_nalla_m_start;
    float fixed_nalla_m_end;

    float fixed_nalla_e_start;
    float fixed_nalla_e_end;

    int month;
    int day;
    
    int lunar_month;
    int lunar_day;
    int festival;
    int special_day;

    int samvatsaram;
    int ayanam;
    int rithu;
} Panchang;

void calc_panchang(DateTime dt, Location loc, Panchang* p);
float round5(float t);

#ifdef __cplusplus
}
#endif

#endif /* TAMIL_PANCHANGAM_H */
