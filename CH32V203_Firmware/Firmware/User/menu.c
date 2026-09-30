#include <string.h>
#include "app_runtime.h"
#include "mydef.h"
#include "storage.h"
#include "eeprom.h"
#include "player.h"
#include "keypad.h"
#include "lcd.h"
#include "rtc_util.h"
#include "text_util.h"

#define MENU_TIMEOUT 10000

void displayTime(uint8_t h, uint8_t m)
{
    char buf[12];
    uint8_t am = (h < 12);
    if (h > 12) h -= 12;
    if (h == 0) h = 12;
    TextBuilder text;
    Text_Init(&text, buf, sizeof(buf));
    Text_AppendUInt(&text, h, 2);
    Text_AppendChar(&text, ':');
    Text_AppendUInt(&text, m, 2);
    Text_AppendChar(&text, ' ');
    Text_Append(&text, am ? "AM" : "PM");
    LCD_WriteString(buf);
}

uint8_t showMenuBool(const char *msg, uint16_t mem)
{
    uint8_t dat = EEPROM_Read8(mem);
    uint8_t key;

    LCD_Clear();
    LCD_WriteString(msg);
    do {
        LCD_PutsAt(1, 0, dat ? "YES" : "NO ");
        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key)
            return 0;
        if (key == KEY_UP || key == KEY_DOWN) {
            dat = !dat;
            EEPROM_Write8(mem, dat);
        }
    } while (key != KEY_MENU);
    return 1;
}

uint8_t showMenuRange(const char *msg, uint16_t mem, uint8_t min, uint8_t max,
                      const char *suffix, int mod)
{
    uint8_t dat = EEPROM_Read8(mem);
    uint8_t key;

    LCD_Clear();
    LCD_WriteString(msg);
    do {
        if (dat < min) dat = max;
        if (dat > max) dat = min;
        EEPROM_Write8(mem, dat);
        LCD_PutsAt(1, 0, "");
        LCD_PrintNum(mod ? (dat % mod) : dat);
        if (suffix)
            LCD_WriteString(suffix);
        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key)
            return 0;
        if (key == KEY_UP) dat++;
        else if (key == KEY_DOWN) dat--;
    } while (key != KEY_MENU);
    return 1;
}

uint8_t showPlaylist(void)
{
    FIL f;
    char line[64];
    uint8_t key, dat = 0;

    if (f_open(&f, "menu.txt", FA_READ) != FR_OK)
        return 1;

    LCD_Clear();
    LCD_WriteString("EDIT PLAYLIST");
    do {
        LCD_PutsAt(1, 0, dat ? "YES" : "NO ");
        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key) {
            f_close(&f);
            return 0;
        }
        if (key == KEY_UP || key == KEY_DOWN)
            dat = !dat;
    } while (key != KEY_MENU);

    while (dat) {
        uint32_t len = Storage_ReadLine(&f, line, sizeof(line));
        if (len == 0)
            break;
        if (len < 5)
            continue;
        char *colon = strchr(line, ':');
        if (!colon)
            continue;
        *colon = 0;

        uint8_t mem = Text_ParseInt(line);
        char *msg = colon + 1;
        if (strlen(msg) > 16)
            msg[16] = 0;
        uint8_t val = EEPROM_ReadBool(mem);

        LCD_Clear();
        LCD_WriteString(msg);
        do {
            LCD_PutsAt(1, 0, val ? "PLAY" : "OFF ");
            key = KEY_WaitKey(MENU_TIMEOUT);
            if (!key) {
                f_close(&f);
                return 0;
            }
            if (key == KEY_UP || key == KEY_DOWN) {
                val = !val;
                EEPROM_WriteBool(mem, val);
            }
        } while (key != KEY_MENU);
    }
    f_close(&f);
    return 1;
}

uint8_t setDate(void)
{
    uint8_t key, pos = 0;
    RTC_Time t;

    if (RTC_ReadTime(&t) != RTC_OK)
        return 0;

    LCD_Clear();
    LCD_WriteString("SET DATE :");
    LCD_CursorOn();
    LCD_BlinkOn();

    while (1) {
        LCD_PutsAt(1, 0, "\002 ");
        LCD_WriteString(RTC_StrDate(&t));
        LCD_SetCursor(1, pos == 0 ? 3 : (pos == 1 ? 6 : 9));

        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key) {
            LCD_CursorOff();
            LCD_BlinkOff();
            return 0;
        }
        if (key == KEY_MENU) {
            pos++;
            if (pos > 2) {
                LCD_CursorOff();
                LCD_BlinkOff();
                return 1;
            }
        }

        if (pos == 0) {
            if (key == KEY_UP) t.date++;
            if (key == KEY_DOWN) t.date = t.date <= 1 ? 31 : t.date - 1;
        } else if (pos == 1) {
            if (key == KEY_UP) t.month++;
            if (key == KEY_DOWN) t.month = t.month <= 1 ? 12 : t.month - 1;
        } else {
            if (key == KEY_UP) t.year++;
            if (key == KEY_DOWN) t.year = t.year <= 26 ? 26 : t.year - 1;
        }

        if (t.date > 31) t.date = 1;
        if ((t.month == 4 || t.month == 6 || t.month == 9 || t.month == 11) && t.date > 30)
            t.date = 30;
        else if (t.month == 2 && t.date > 28 && !RTC_IsLeapYear(2000 + t.year))
            t.date = 28;
        if (t.month > 12) t.month = 1;
        if (t.year > 50) t.year = 26;

        t.day = RTC_WeekDay(&t) + 1;
        RTC_SetTime(&t);
        rtcNow = t;
    }
}

uint8_t setTime(uint8_t dat)
{
    uint8_t key, pos = 0;
    uint8_t hour, min;
    RTC_Time t;

    if (dat == 1) {
        if (RTC_ReadTime(&t) != RTC_OK)
            return 0;
        hour = t.hours;
        min = t.minutes;
    } else if (dat == 2) {
        hour = EEPROM_Read8(MEM_LIGHTON_AT_HOUR);
        min = EEPROM_Read8(MEM_LIGHTON_AT_MIN);
    } else {
        hour = EEPROM_Read8(MEM_LIGHTOFF_AT_HOUR);
        min = EEPROM_Read8(MEM_LIGHTOFF_AT_MIN);
    }

    LCD_Clear();
    if (dat == 1) LCD_WriteString("SET TIME :");
    else if (dat == 2) LCD_WriteString("LIGHT ON AT");
    else LCD_WriteString("LIGHT OFF AT");
    LCD_CursorOn();
    LCD_BlinkOn();

    while (1) {
        LCD_PutsAt(1, 0, "\002 ");
        displayTime(hour, min);
        LCD_SetCursor(1, pos == 0 ? 3 : 6);

        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key) {
            LCD_CursorOff();
            LCD_BlinkOff();
            return 0;
        }
        if (key == KEY_MENU) {
            pos++;
            if (pos > 1)
                break;
            continue;
        }

        if (pos == 0) {
            if (key == KEY_UP) hour++;
            if (key == KEY_DOWN) hour = (hour == 0) ? 23 : hour - 1;
        } else {
            if (key == KEY_UP) min++;
            if (key == KEY_DOWN) min = (min == 0) ? 59 : min - 1;
        }
        if (hour > 23) hour = 0;
        if (min > 59) min = 0;
    }

    LCD_CursorOff();
    LCD_BlinkOff();

    if (dat == 1) {
        if (RTC_ReadTime(&t) != RTC_OK)
            return 0;
        t.hours = hour;
        t.minutes = min;
        t.day = RTC_WeekDay(&t) + 1;
        RTC_SetTime(&t);
        rtcNow = t;
    } else if (dat == 2) {
        EEPROM_Write8(MEM_LIGHTON_AT_HOUR, hour);
        EEPROM_Write8(MEM_LIGHTON_AT_MIN, min);
    } else {
        EEPROM_Write8(MEM_LIGHTOFF_AT_HOUR, hour);
        EEPROM_Write8(MEM_LIGHTOFF_AT_MIN, min);
    }
    return 1;
}

uint8_t setMusic(char prefix, const char *menu, uint16_t memoryPos)
{
    char dir[6], path[64];
    uint8_t key, pos, ret = 0;

    if (!firstFolder(prefix, dir))
        return 1;
    if (!Storage_FolderExists(dir))
        return 1;

    LCD_Clear();
    LCD_WriteString(menu);
    pos = EEPROM_Read8(memoryPos);

    while (1) {
        char num[18];
        if (pos > 100) pos = 100;
        if (pos < 1) pos = 1;
        TextBuilder text;
        Text_Init(&text, num, sizeof(num));
        Text_Append(&text, "Track:");
        Text_AppendUInt(&text, pos, 3);
        LCD_PutsAt(1, 0, num);

        Text_Init(&text, path, sizeof(path));
        Text_Append(&text, dir); Text_AppendChar(&text, '/');
        Text_AppendUInt(&text, pos, 4); Text_Append(&text, ".mp3");
        if (!Storage_FileExists(path)) {
            Text_Init(&text, path, sizeof(path));
            Text_Append(&text, dir); Text_AppendChar(&text, '/');
            Text_AppendUInt(&text, pos, 4); Text_Append(&text, ".enc");
            if (!Storage_FileExists(path)) {
                if (pos == 1) {
                    ret = 1;
                    break;
                }
                pos = 1;
                continue;
            }
        }

        Player_Play(path);
        key = KEY_WaitKey(MENU_TIMEOUT);
        if (!key)
            break;
        if (key == KEY_MENU) {
            EEPROM_Write8(memoryPos, pos);
            ret = 1;
            break;
        }
        if (key == KEY_UP && pos < 100) pos++;
        else if (key == KEY_DOWN && pos > 1) pos--;
        Player_Stop();
    }

    Player_Stop();
    App_DelayMs(100);
    return ret;
}
