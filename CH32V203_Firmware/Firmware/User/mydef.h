#ifndef _MY_DEF_H_
#define _MY_DEF_H_

#include <stdint.h>
#include "tamil_panchangam.h"
#include "rtc.h"

/* EEPROM-store memory map, adopted from esp_bell_idf mydef.h.
 * Bits 0-100 are bit-addressable playlist/feature enable flags. */
#define MEM_AMP_ON            13
#define MEM_AMP_ON_OFF        14
#define MEM_NIGHT_PLAY        16
#define MEM_START_AT          17
#define MEM_END_AT            18
#define MEM_BELL              20
#define MEM_START             21
#define MEM_LIGHTON_AT_HOUR   22
#define MEM_LIGHTON_AT_MIN    23
#define MEM_LIGHTOFF_AT_HOUR  24
#define MEM_LIGHTOFF_AT_MIN   25
#define MEM_SEQ_ID            26  /* 2 bytes: current trigger-window id */
#define MEM_SEQ_POS           28  /* groups completed in current pass */
#define MEM_GROUP_AT          50  /* 2 bytes per rotating group */

/* Audio clip folder bases (SD folder = 3-digit token id, file = 4-digit) */
#define TOAY_PANCHANGAM  1
#define MUHURTAM         3
#define SUN_RISE         4
#define SUN_SET          5
#define MOON_RISE        6
#define MOON_SET         7
#define DURMUHURTAM      8
#define RAGUKALAM        9
#define KULIGAI          10
#define EMAKANDAM        11
#define ABIJITH          12
#define STARTS           13
#define ENDS             14
#define AYANAM           20
#define RITHU            30
#define REGIONAL_YEARS   40
#define NAKSTRA          100
#define RASI             200
#define THITHI           300
#define YOUGAM           400
#define KARANAM          500
#define REGIONAL_MONTH   601
#define WEEKS            650
#define REGIONAL_DATE    700
#define HOURS            800
#define MINUTE           900

extern char welcome[71];
extern Location location;
extern Panchang panchang;
extern RTC_Time rtcNow;
extern uint8_t playDemo;

/* bell.c */
uint8_t loadSettings(void);
uint8_t checkLeave(void);
uint8_t checkSong(void);
uint8_t fileFound(const char *name);
uint8_t playFile(const char *name, uint32_t endTime, uint8_t repeat);
uint8_t firstFolder(char typeFilter, char *out);
void showDateTime(uint8_t leave);
void Light_Service(void);

/* menu.c */
void displayTime(uint8_t h, uint8_t m);
uint8_t showMenuBool(const char *msg, uint16_t mem);
uint8_t showMenuRange(const char *msg, uint16_t mem, uint8_t min, uint8_t max, const char *suffix, int mod);
uint8_t setMusic(char prefix, const char *menu, uint16_t memoryPos);
uint8_t setDate(void);
uint8_t setTime(uint8_t dat);
uint8_t showPlaylist(void);

/* main.c */
uint8_t setRelay(uint8_t state);

#endif
