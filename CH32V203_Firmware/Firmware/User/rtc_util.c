#include "rtc_util.h"
#include "text_util.h"

static const char WEEK_DAYS[7][4] = {"SUN","MON","TUE","WED","THR","FRI","SAT"};
static char rtcText[16];

uint8_t RTC_IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t RTC_WeekDay(const RTC_Time *t)
{
    /* Zeller, Gregorian */
    int y = 2000 + t->year, m = t->month, d = t->date;
    if (m < 3) { m += 12; y--; }
    int k = y % 100, j = y / 100;
    int h = (d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (uint8_t)((h + 6) % 7);
}

uint8_t RTC_Get12Hour(const RTC_Time *t)
{
    uint8_t h = t->hours;
    if (h > 12) h -= 12;
    if (h == 0) h = 12;
    return h;
}

int RTC_DayOfYear(const RTC_Time *t)
{
    static const int dim[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int total = 0, i;
    for (i = 1; i < t->month; i++)
        total += dim[i];
    if (t->month > 2 && RTC_IsLeapYear(2000 + t->year))
        total++;
    return total + t->date;
}

int RTC_WeekOfYear(const RTC_Time *t)
{
    int y = 2000 + t->year;
    int jan1 = (y - 1 + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400) % 7;
    int week = (jan1 + RTC_DayOfYear(t) - 1) / 7;
    if (jan1 < 4) week++;
    return week;
}

uint32_t RTC_MakeDate(uint8_t y, uint8_t m, uint8_t d)
{
    return (uint32_t)y * 10000UL + (uint32_t)m * 100UL + d;
}

const char *RTC_StrTime(const RTC_Time *t)
{
    TextBuilder text;
    Text_Init(&text, rtcText, sizeof(rtcText));
    Text_AppendUInt(&text, RTC_Get12Hour(t), 2);
    Text_AppendChar(&text, ':');
    Text_AppendUInt(&text, t->minutes, 2);
    Text_Append(&text, t->hours < 12 ? "AM" : "PM");
    return rtcText;
}

const char *RTC_StrDate(const RTC_Time *t)
{
    TextBuilder text;
    Text_Init(&text, rtcText, sizeof(rtcText));
    Text_AppendUInt(&text, t->date, 2);
    Text_AppendChar(&text, ':');
    Text_AppendUInt(&text, t->month, 2);
    Text_AppendChar(&text, ':');
    Text_AppendUInt(&text, t->year, 2);
    Text_AppendChar(&text, ' ');
    Text_Append(&text, WEEK_DAYS[RTC_WeekDay(t)]);
    return rtcText;
}
