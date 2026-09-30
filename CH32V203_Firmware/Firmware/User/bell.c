#include <string.h>
#include "mydef.h"
#include "app_runtime.h"
#include "storage.h"
#include "eeprom.h"
#include "player.h"
#include "keypad.h"
#include "lcd.h"
#include "rtc_util.h"
#include "relay_light.h"
#include "text_util.h"

char welcome[71];
char panchangOrder[32];
Location location;
Panchang panchang;
RTC_Time rtcNow;
uint8_t playDemo = 0;

/* UTC offset: accepts "5:30", "5h30", "5.30"/"5,30" (h+min) or decimal. */
static float parse_timezone(const char *s)
{
    const char *p = s;
    while (*p == ' ' || *p == '\t')
        p++;

    int sign = 1;
    if (*p == '+')
        p++;
    else if (*p == '-') {
        sign = -1;
        p++;
    }

    int h = 0, m = 0;
    const char *q = p;
    while (*q >= '0' && *q <= '9') {
        h = h * 10 + (*q - '0');
        q++;
    }

    if (*q == ':') {
        const char *r = q + 1;
        while (*r == ' ' || *r == '\t')
            r++;
        if (*r >= '0' && *r <= '9') {
            m = 0;
            while (*r >= '0' && *r <= '9')
                m = m * 10 + (*r++ - '0');
            if (m <= 59) {
                const char *tail = r;
                while (*tail == ' ' || *tail == '\t')
                    tail++;
                if (*tail == '\0' || *tail == ':')
                    return sign * ((float)h + (float)m / 60.0f);
            }
        }
    }

    if (*q == 'h' || *q == 'H') {
        const char *r = q + 1;
        while (*r == ' ' || *r == '\t')
            r++;
        if (*r >= '0' && *r <= '9') {
            m = 0;
            while (*r >= '0' && *r <= '9')
                m = m * 10 + (*r++ - '0');
            if (m <= 59) {
                while (*r == ' ' || *r == '\t')
                    r++;
                if (*r == '\0')
                    return sign * ((float)h + (float)m / 60.0f);
            }
        }
    }

    if ((*q == '.' || *q == ',') && q > p &&
        q[1] >= '0' && q[1] <= '9' && q[2] >= '0' && q[2] <= '9') {
            m = (q[1] - '0') * 10 + (q[2] - '0');
            if (m <= 59) {
                const char *a = q + 3;
                while (*a == '0')
                    a++;
                while (*a == ' ' || *a == '\t')
                    a++;
                if (*a == '\0')
                    return sign * ((float)h + (float)m / 60.0f);
            }
    }

    return sign * Text_ParseFloat(p);
}

uint8_t loadSettings(void)
{
    FIL f;
    char line[64] = {0};
    TextBuilder text;

    if (f_open(&f, "settings.txt", FA_READ) != FR_OK)
        return 0;
    Storage_ReadLine(&f, line, sizeof(line));
    Text_Init(&text, welcome, sizeof(welcome));
    Text_Append(&text, "HappyBell 2025 - ");
    Text_AppendN(&text, line, 49);
    Text_Append(&text, "    ");
    Storage_ReadLine(&f, line, 32);
    location.lat = Text_ParseFloat(line);
    Storage_ReadLine(&f, line, 32);
    location.lon = Text_ParseFloat(line);
    Storage_ReadLine(&f, line, 32);
    location.tz = parse_timezone(line);
    Storage_ReadLine(&f, line, 32);
    location.calender = Text_ParseInt(line);
    Storage_ReadLine(&f, line, sizeof(panchangOrder));
    strncpy(panchangOrder, line, sizeof(panchangOrder) - 1);
    if (line[0])
        panchangOrder[strlen(line) - 1] = 0;  /* esp_bell_idf drops the trailing char */
    f_close(&f);
    return 1;
}

uint8_t checkLeave(void)
{
    FIL f;
    char today[10], line[16];

    if (f_open(&f, "holiday.txt", FA_READ) != FR_OK)
        return 0;
    TextBuilder text;
    Text_Init(&text, today, sizeof(today));
    Text_AppendUInt(&text, rtcNow.year, 2);
    Text_AppendUInt(&text, rtcNow.month, 2);
    Text_AppendUInt(&text, rtcNow.date, 2);
    while (Storage_ReadLine(&f, line, sizeof(line))) {
        if (strcmp(line, today) == 0) {
            f_close(&f);
            return 1;
        }
    }
    f_close(&f);
    return 0;
}

static uint8_t checkAnyoneFile(const char *name, char *path, uint32_t size)
{
    TextBuilder text;
    Text_Init(&text, path, size);
    Text_Append(&text, name);
    Text_Append(&text, ".mp3");
    if (Storage_FileExists(path))
        return 1;
    Text_Init(&text, path, size);
    Text_Append(&text, name);
    Text_Append(&text, ".enc");
    return Storage_FileExists(path);
}

uint8_t fileFound(const char *name)
{
    char path[64];
    return checkAnyoneFile(name, path, sizeof(path));
}

/* Plays name.mp3/.enc; UP key stops, endTime (ms tick) cuts the loop. */
uint8_t playFile(const char *name, uint32_t endTime, uint8_t repeat)
{
    char path[64];
    if (!checkAnyoneFile(name, path, sizeof(path)))
        return 0;

    while (repeat--) {
        Player_Play(path);
        Player_SetWait(KEY_UP, endTime, 10U);
        LCD_PutsAt(1, 0, "");
        while (Player_IsPlaying()) {
            Player_Service();
            if (endTime && millis() > endTime) {
                Player_Stop();
                Player_SetWait(KEY_NONE, 0, 0);
                return 1;
            }
            if (KEY_GetKey() & KEY_UP)
                Player_Stop();
            App_DelayMs(10);
        }
        Player_SetWait(KEY_NONE, 0, 0);
    }
    return 1;
}

static int read2(const char *p)
{
    return (p[0] - '0') * 10 + (p[1] - '0');
}

static uint8_t hexNib(char h)
{
    if (h >= '0' && h <= '9') return h - '0';
    if (h >= 'a' && h <= 'f') return h - 'a' + 10;
    if (h >= 'A' && h <= 'F') return h - 'A' + 10;
    return 0;
}

static uint8_t readHex2(const char *p)
{
    return (hexNib(p[0]) << 4) | hexNib(p[1]);
}

static void extract_time(float t, int *ph, int *pm)
{
    *ph = (int)t;
    *pm = (int)((t - *ph) * 60.0f + 0.5f);
    if (*pm >= 60) { *pm -= 60; *ph += 1; }
    if (*ph >= 24) *ph -= 24;
}

/* Sequence-playback helpers (same shapes as esp_bell_idf bell.cpp) */
#define playFolderFile(val) do { \
    TextBuilder text; Text_Init(&text, file, sizeof(file)); \
    Text_Append(&text, pos); Text_AppendChar(&text, '/'); \
    Text_AppendUInt(&text, (uint32_t)(val), 4); \
    playFile(file, endTime, repeat); } while (0)

#define playTimeOnly(t) do { int h, m; extract_time(t, &h, &m); \
    playFolderFile(HOURS + h); playFolderFile(MINUTE + m); } while (0)

#define playTimePos(t, e) do { playTimeOnly(t); playFolderFile(e); } while (0)

#define playTimePre(s, t) do { playFolderFile(s); playTimeOnly(t); } while (0)

#define playFolderFileEnds(val, time) do { playFolderFile(val); \
    playTimePos(time, ENDS); } while (0)

#define playStartEnd(s, e) do { playTimePos(s, STARTS); \
    playTimePos(e, ENDS); } while (0)

#define playFolderFileStartEnd(val, s, e) do { playFolderFile(val); \
    playStartEnd(s, e); } while (0)

static void playPanchangam(const char *pos, uint32_t endTime)
{
    char file[64];
    uint8_t repeat = 1;
    int i;

    playFolderFile(TOAY_PANCHANGAM);
    for (i = 0; panchangOrder[i]; i++) {
        switch (panchangOrder[i]) {
        case 'y': playFolderFile(REGIONAL_YEARS + panchang.samvatsaram); break;
        case 'a': playFolderFile(AYANAM + panchang.ayanam); break;
        case 'r': playFolderFile(RITHU + panchang.rithu); break;
        case 'M': playFolderFile(REGIONAL_MONTH + panchang.month); break;
        case 'D': playFolderFile(REGIONAL_DATE + panchang.day); break;
        case 'W': playFolderFile(WEEKS + RTC_WeekDay(&rtcNow)); break;
        case 'N': playFolderFileEnds(NAKSTRA + panchang.nakshatra, panchang.nak_end); break;
        case 'R': playFolderFile(RASI + panchang.moon_sign); break;
        case 'T': playFolderFileEnds(THITHI + panchang.tithi, panchang.tithi_end); break;
        case 'Y': playFolderFile(YOUGAM + panchang.yoga); break;
        case 'K': playFolderFile(KARANAM + panchang.karana1); break;
        case 'S': playTimePre(SUN_RISE, panchang.sunrise); break;
        case 's': playTimePre(SUN_SET, panchang.sunset); break;
        case 'O': if (panchang.moonrise) playTimePre(MOON_RISE, panchang.moonrise); break;
        case 'o': if (panchang.moonset) playTimePre(MOON_SET, panchang.moonset); break;
        case 'A': playFolderFileStartEnd(RAGUKALAM, panchang.rahu_start, panchang.rahu_end); break;
        case 'G': playFolderFileStartEnd(KULIGAI, panchang.gulikai_start, panchang.gulikai_end); break;
        case 'k': playFolderFileStartEnd(EMAKANDAM, panchang.yama_start, panchang.yama_end); break;
        case 'd': playFolderFileStartEnd(DURMUHURTAM, panchang.dur1_start, panchang.dur1_end); break;
        case 'U':
            playFolderFileStartEnd(MUHURTAM, panchang.nalla_m_start, panchang.nalla_m_end);
            playStartEnd(panchang.nalla_e_start, panchang.nalla_e_end);
            break;
        case 'u':
            playFolderFileStartEnd(MUHURTAM, panchang.fixed_nalla_m_start, panchang.fixed_nalla_m_end);
            playStartEnd(panchang.fixed_nalla_e_start, panchang.fixed_nalla_e_end);
            break;
        default: break;
        }
    }
}

/* Plays one schedule line. all=1 plays every token; repeat passes replay
 * group tokens only. seqId + EEPROM resume state skip groups already
 * played before a power loss inside the same trigger window. */
static void playSequence(char *line, uint32_t endTime, uint8_t all, uint16_t seqId)
{
    char file[64];
    DateTime dt;
    uint8_t gDone = 0;

    dt.year = 2000 + rtcNow.year;
    dt.month = rtcNow.month;
    dt.day = rtcNow.date;
    dt.hour = (float)rtcNow.hours + (float)rtcNow.minutes / 60.0f +
              (float)rtcNow.seconds / 3600.0f;
    calc_panchang(dt, location, &panchang);

    if (!playDemo)
        setRelay(1);

    LCD_PutsAt(0, 0, "Playing...");

    if (EEPROM_Read16(MEM_SEQ_ID) == seqId) {
        gDone = EEPROM_Read8(MEM_SEQ_POS);
    } else {
        EEPROM_Write16(MEM_SEQ_ID, seqId);
        EEPROM_Write8(MEM_SEQ_POS, 0);
    }

    while (1) {
        uint8_t gIndex = 0;
        const char *ptr = line;

        while (*ptr) {
            char type = *ptr++;
            char pos[5];
            uint8_t repeat = 1;

            if (type == 'f') {
                strncpy(pos, ptr, 4);
                pos[4] = 0;
                ptr += 4;
            } else {
                strncpy(pos, ptr, 3);
                pos[3] = 0;
                ptr += 3;
                if (!EEPROM_ReadBool(Text_ParseInt(pos)))
                    continue;
            }

            if (type == 'g') {
                gIndex++;
                if (gIndex <= gDone)
                    continue;
                uint16_t address = MEM_GROUP_AT + Text_ParseInt(pos) * 2;
                uint16_t val = EEPROM_Read16(address);
                if (val == 0 || val == 0xFFFF) val = 1;
                TextBuilder text;
                Text_Init(&text, file, sizeof(file));
                Text_Append(&text, pos); Text_AppendChar(&text, '/');
                Text_AppendUInt(&text, val, 4);
                if (!fileFound(file)) {
                    val = 1;
                    Text_Init(&text, file, sizeof(file));
                    Text_Append(&text, pos); Text_AppendChar(&text, '/');
                    Text_AppendUInt(&text, val, 4);
                }
                if (fileFound(file)) {
                    EEPROM_Write16(address, val + 1);
                    playFile(file, endTime, 1);
                }
                EEPROM_Write8(MEM_SEQ_POS, gIndex);
            }
            else if (all) {
                if (type == 'f') {
                    playFile(pos, endTime, 1);
                    continue;
                }
                uint16_t val = 1;
                if (type == 'm') val = rtcNow.minutes;
                else if (type == 'h') val = rtcNow.hours;
                else if (type == 'w') val = RTC_WeekDay(&rtcNow);
                else if (type == 'd') val = rtcNow.date;
                else if (type == 'M') val = rtcNow.month;
                else if (type == 'W') val = RTC_WeekOfYear(&rtcNow);
                else if (type == 'D') val = RTC_DayOfYear(&rtcNow);
                else if (type == 's') val = EEPROM_Read8(MEM_START);
                else if (type == 'b') {
                    repeat = RTC_Get12Hour(&rtcNow);
                    val = EEPROM_Read8(MEM_BELL);
                }
                else if (type == 'F') {
                    if (!panchang.festival) continue;
                    val = panchang.festival;
                }
                else if (type == 'S') {
                    if (!panchang.special_day) continue;
                    val = panchang.special_day;
                }
                else if (type == 'p') {
                    playPanchangam(pos, endTime);
                    continue;
                }
                else {
                    /* Ignore unsupported sequence tokens without inventing a file. */
                    continue;
                }
                TextBuilder text;
                Text_Init(&text, file, sizeof(file));
                Text_Append(&text, pos); Text_AppendChar(&text, '/');
                Text_AppendUInt(&text, val, 4);
                playFile(file, endTime, repeat);
            }
        }

        gDone = 0;
        EEPROM_Write8(MEM_SEQ_POS, 0);

        if (!endTime || millis() > endTime)
            break;
        all = 0;
    }

    Player_Stop();
    setRelay(0);
}

static uint8_t playTime(char *line)
{
    uint16_t now_t = rtcNow.hours * 60;
    if (!playDemo)
        now_t += rtcNow.minutes;

    uint8_t h = read2(line); line += 2;
    uint8_t m = read2(line); line += 2;
    uint8_t du = readHex2(line); line += 2;

    uint16_t st = h * 60 + m;
    uint16_t et = st + du;
    uint32_t endTime = 0;
    if (du)
        endTime = millis() + (uint32_t)(et - now_t) * 60000UL;

    /* window id: same date + same start = same trigger, resumes on reboot */
    uint16_t seqId = ((uint16_t)rtcNow.date << 11) | st;

    if (st == now_t) {
        playSequence(line, playDemo ? 0 : endTime, 1, seqId);
        return 1;
    }
    if (st <= now_t && et > (now_t + 2)) {
        playSequence(line, endTime, 0, seqId);
        return 1;
    }
    return 0;
}

static uint8_t playWeek(char *line)
{
    uint8_t w = readHex2(line);
    if ((1 << RTC_WeekDay(&rtcNow)) & w)
        return playTime(line + 2);
    return 0;
}

uint8_t checkSong(void)
{
    FIL f;
    char line[256];

    if (f_open(&f, "playlist.txt", FA_READ) != FR_OK)
        return 0;

    while (Storage_ReadLine(&f, line, sizeof(line))) {
        char type = line[0];
        char *ptr = line + 1;

        if (type == '1') {
            int y = read2(ptr); ptr += 2;
            int m = read2(ptr); ptr += 2;
            int d = read2(ptr); ptr += 2;
            if (rtcNow.year == y && rtcNow.month == m && rtcNow.date == d &&
                playTime(ptr)) {
                f_close(&f);
                return 1;
            }
        }
        else if (type == '2') {
            int sy = read2(ptr); ptr += 2;
            int sm = read2(ptr); ptr += 2;
            int sd = read2(ptr); ptr += 2;
            int ey = read2(ptr); ptr += 2;
            int em = read2(ptr); ptr += 2;
            int ed = read2(ptr); ptr += 2;
            uint32_t cur = RTC_MakeDate(rtcNow.year, rtcNow.month, rtcNow.date);
            if (cur >= RTC_MakeDate(sy, sm, sd) &&
                cur <= RTC_MakeDate(ey, em, ed) && playWeek(ptr)) {
                f_close(&f);
                return 1;
            }
        }
        else if (type == '3') {
            if (playWeek(ptr)) {
                f_close(&f);
                return 1;
            }
        }
    }
    f_close(&f);
    return 0;
}

uint8_t firstFolder(char typeFilter, char *out)
{
    FIL f;
    char line[256];

    if (f_open(&f, "playlist.txt", FA_READ) != FR_OK)
        return 0;

    while (Storage_ReadLine(&f, line, sizeof(line))) {
        char *ptr;
        char type = line[0];
        if (type == '1') ptr = line + 13;
        else if (type == '2') ptr = line + 21;
        else if (type == '3') ptr = line + 9;
        else continue;

        while (*ptr) {
            type = *ptr++;
            if (type == 'f') {
                ptr += 4;
                continue;
            }
            if (type == typeFilter) {
                strncpy(out, ptr, 3);
                out[3] = 0;
                f_close(&f);
                return 1;
            }
            ptr += 3;
        }
    }
    f_close(&f);
    return 0;
}

void Light_Service(void)
{
    static uint32_t previous;
    static uint8_t started;
    RTC_Time lightNow;
    uint32_t now = millis();

    if (started && (uint32_t)(now - previous) < 1000U)
        return;
    started = 1;
    {
        uint8_t on_h = EEPROM_Read8(MEM_LIGHTON_AT_HOUR);
        uint8_t on_m = EEPROM_Read8(MEM_LIGHTON_AT_MIN);
        uint8_t off_h = EEPROM_Read8(MEM_LIGHTOFF_AT_HOUR);
        uint8_t off_m = EEPROM_Read8(MEM_LIGHTOFF_AT_MIN);

        if (on_h > 23) on_h = 0;
        if (on_m > 59) on_m = 0;
        if (off_h > 23) off_h = 0;
        if (off_m > 59) off_m = 0;

        if (RTC_ReadTime(&lightNow) == RTC_OK) {
            uint16_t on_t = on_h * 60 + on_m;
            uint16_t off_t = off_h * 60 + off_m;
            uint16_t now_t = lightNow.hours * 60 + lightNow.minutes;
            uint8_t light = 0;

            if (on_t < off_t)
                light = (now_t >= on_t && now_t < off_t);
            else if (on_t > off_t)
                light = (now_t >= on_t || now_t < off_t);

            Light_Set(light);
        }
    }
    previous = millis();
}

static uint8_t msgPos = 0;

static void scrollText(void)
{
    uint8_t pos = 0, count;
    char ch;

    LCD_SetCursor(0, 0);
    for (count = 0; count < 16; count++) {
        ch = ((msgPos + pos) > 70) ? 0 : welcome[msgPos + pos];
        if (ch == 0 || (uint8_t)ch == 0xFFU || ch == '\r' || ch == '\n') {
            msgPos = 0;
            pos = 0;
            ch = welcome[0];
        }
        LCD_WriteChar(ch);
        pos++;
    }
    msgPos++;
}

void showDateTime(uint8_t leave)
{
    LCD_SetCursor(0, 0);
    if (EEPROM_Read8(MEM_AMP_ON)) {
        LCD_PutsAt(0, 0, "AMP: ON");
        setRelay(1);
    } else {
        scrollText();
        setRelay(0);
    }

    if (leave && (rtcNow.seconds % 2)) {
        LCD_PutsAt(1, 0, "  TODAY HOLIDAY ");
    } else {
        LCD_SetCursor(1, 0);
        LCD_WriteString(RTC_StrTime(&rtcNow));
        LCD_WriteChar(' ');
        LCD_WriteString(RTC_StrDate(&rtcNow));
    }
}
