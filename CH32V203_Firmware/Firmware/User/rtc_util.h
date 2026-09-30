#ifndef __RTC_UTIL_H
#define __RTC_UTIL_H

#include <stdint.h>
#include "rtc.h"

/* Date/time helpers over RTC_Time (year is 0-99, 20xx). */
uint8_t RTC_WeekDay(const RTC_Time *t);            /* 0=Sun..6=Sat */
uint8_t RTC_Get12Hour(const RTC_Time *t);
int RTC_WeekOfYear(const RTC_Time *t);
int RTC_DayOfYear(const RTC_Time *t);
uint8_t RTC_IsLeapYear(int year);
uint32_t RTC_MakeDate(uint8_t y, uint8_t m, uint8_t d); /* yymmdd as int */
const char *RTC_StrTime(const RTC_Time *t);        /* "hh:mmAM" */
const char *RTC_StrDate(const RTC_Time *t);        /* "dd:mm:yy DAY" */

#endif
