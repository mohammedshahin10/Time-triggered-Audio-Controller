#include "happybell_build.h"

#if HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_FULL

#include "ch32v20x.h"
#include "debug.h"
#include "mydef.h"
#include "app_runtime.h"
#include "storage.h"
#include "eeprom.h"
#include "player.h"
#include "keypad.h"
#include "lcd.h"
#include "rtc.h"
#include "relay_light.h"

/*
 * HAPPY BELL - CH32V203G8R6
 * Local bell/announcement controller ported from the reference application.
 * Pins: see README.md. Audio: PA8 PWM -> LPF -> AUX_OUT.
 */

/* Force this immutable HD44780 glyph into FLASH despite -msmall-data-limit=8. */
static const uint8_t setupCursorGlyph[8]
    __attribute__((section(".rodata.setup_cursor"))) = {
        0x00U, 0x0EU, 0x15U, 0x17U, 0x11U, 0x0EU, 0x00U, 0x00U
    };

uint8_t setRelay(uint8_t state)
{
    App_DelayMs(100);
    Relay_Set(state);
    App_DelayMs(300);
    return 1;
}

/* UP key: play the fixed announcement track, like esp managalaVathiyam. */
static void playAnnouncement(void)
{
    LCD_Clear();
    LCD_WriteString("Playing...");
    setRelay(1);
    App_DelayMs(100);
    playDemo = !playFile("00001", 0, 1);
    if (!playDemo) {
        Player_Stop();
        setRelay(0);
    }
}

static void ampOnPressed(void)
{
    uint8_t ampOn = !EEPROM_Read8(MEM_AMP_ON);
    setRelay(ampOn);
    EEPROM_Write8(MEM_AMP_ON, ampOn);
    LCD_Clear();
    LCD_WriteString(ampOn ? "AMP: ON" : "AMP: OFF");
    App_DelayMs(2000);
}

/* MENU key: full setup chain; any timeout aborts the rest. */
static void setupMenu(void)
{
    if (setRelay(1)
        && setMusic('b', "BELL SOUND:", MEM_BELL)
        && setMusic('s', "STARTING MUSIC:", MEM_START)
        && setRelay(0)
        && showMenuBool("PLAY AT NIGHT :", MEM_NIGHT_PLAY)
        && (EEPROM_Read8(MEM_NIGHT_PLAY)
            || (showMenuRange("BELL START AT :", MEM_START_AT, 3, 9, "AM", 12)
                && showMenuRange("BELL CLOSE AT :", MEM_END_AT, 17, 23, "PM", 12)))
        && setTime(1)
        && setDate()
        && setTime(2)
        && setTime(3)
        && showMenuBool("AMP ON/OFF :", MEM_AMP_ON_OFF)
        && showPlaylist()) {
    } else {
        setRelay(0);
    }
    LCD_CursorOff();
    LCD_BlinkOff();
}

static void idleMenu(void)
{
    uint8_t key = KEY_GetKey();

    if ((key & KEY_DOWN) && EEPROM_Read8(MEM_AMP_ON_OFF)) {
        if (KEY_WaitKey(10000) & KEY_DOWN)
            ampOnPressed();
    }
    if (key & KEY_UP) {
        if (KEY_WaitKey(10000) & KEY_UP)
            playAnnouncement();
    }
    if (key & KEY_MENU) {
        if (KEY_WaitKey(10000) == KEY_MENU)
            setupMenu();
    }
}

static void Bell_Service(void)
{
    static uint32_t previous;
    static uint32_t lastRun;
    static uint8_t started;
    static uint8_t leave;
    uint32_t now;

    now = millis();
    if (started && (uint32_t)(now - lastRun) < 20U)
        return;
    started = 1;

    idleMenu();

    now = millis();
    if ((now - previous) >= 600 || playDemo) {
        previous = now;
        RTC_ReadTime(&rtcNow);

        if (playDemo || (rtcNow.seconds < 3 && !EEPROM_Read8(MEM_AMP_ON) &&
            (EEPROM_Read8(MEM_NIGHT_PLAY) ||
             (rtcNow.hours >= EEPROM_Read8(MEM_START_AT) &&
              rtcNow.hours <= EEPROM_Read8(MEM_END_AT))))) {
            leave = checkLeave();
            if (!leave)
                checkSong();
            playDemo = 0;
        }
        showDateTime(leave);
    }
    lastRun = millis();
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    LCD_Init();
    LCD_CreateChar(2U, setupCursorGlyph);

    if (SystemCoreClock != 96000000U) {
        LCD_Clear();
        LCD_WriteString("CLOCK ERROR");
        LCD_SetCursor(1, 0);
        LCD_WriteString("NOT 96MHZ");
        while (1)
            Delay_Ms(500);
    }

    KEY_Init();
    RTC_Init();
    RelayLight_Init();
    Delay_Ms(100);

    LCD_WriteString("Loading...");

    if (RTC_Begin() != RTC_OK) {
        LCD_Clear();
        LCD_WriteString("RTC ERROR");
        LCD_SetCursor(1, 0);
        LCD_WriteString("CHECK DS1307");
        while (1)
            Delay_Ms(500);
    }
    RTC_ReadTime(&rtcNow);

    if (!Storage_Mount()) {
        LCD_Clear();
        LCD_WriteString("SD Card Error");
        while (1)
            Delay_Ms(500);
    }

    EEPROM_Begin();
    loadSettings();

    Player_Init();
    AppRuntime_Init();

    while (1) {
        Player_Service();
        Light_Service();
        Bell_Service();
        App_DelayMs(1);
    }
}

#elif HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_DIAGNOSTIC

#include "ch32v20x.h"
#include "debug.h"
#include "keypad.h"
#include "lcd.h"
#include "rtc.h"
#include "storage.h"
#include "sd_text_viewer.h"

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    LCD_Init();
    KEY_Init();
    RTC_Init();
    Delay_Ms(100);

    LCD_WriteString("Loading...");
    if (RTC_Begin() != RTC_OK) {
        LCD_Clear();
        LCD_WriteString("RTC ERROR");
        LCD_SetCursor(1, 0);
        LCD_WriteString("CHECK DS1307");
        while (1)
            Delay_Ms(500);
    }

    if (!Storage_Mount()) {
        LCD_Clear();
        LCD_WriteString("SD Card Error");
        while (1)
            Delay_Ms(500);
    }

    while (1) {
        SD_TextViewer_Run("TEST.TXT");
        LCD_Clear();
        LCD_WriteString("MENU: TEST.TXT");
        LCD_SetCursor(1, 0);
        LCD_WriteString("UP/DOWN: PAGE");
        while (KEY_GetKey() != KEY_MENU)
            Delay_Ms(20);
    }
}

#else

#include "ch32v20x.h"
#include "debug.h"
#include "ch32v20x_it.h"
#include "storage.h"
#include "lcd.h"
#include "keypad.h"
#include "relay_light.h"
#include "ch32_mp3_player.h"

static void App_WaitMs(uint16_t durationMs)
{
    while (durationMs >= 10U) {
        Delay_Ms(10U);
        durationMs -= 10U;
    }
    while (durationMs-- != 0U)
        Delay_Ms(1U);
}

static const char *MP3Test_StatusText(CH32_MP3_Status status)
{
    switch (status) {
    case CH32_MP3_OK:                 return "PLAYBACK OK";
    case CH32_MP3_FILE_ERROR:         return "FILE ERROR";
    case CH32_MP3_DECODER_ERROR:      return "DECODER ERROR";
    case CH32_MP3_UNSUPPORTED_STEREO: return "STEREO REJECTED";
    case CH32_MP3_UNSUPPORTED_RATE:   return "RATE REJECTED";
    case CH32_MP3_FRAME_TOO_LARGE:    return "FRAME TOO LARGE";
    case CH32_MP3_AUDIO_TIMEOUT:      return "DMA TIMEOUT";
    case CH32_MP3_SD_READ_ERROR:      return "SD READ ERROR";
    case CH32_MP3_DECRYPT_ERROR:      return "ENC INVALID";
    case CH32_MP3_UNSUPPORTED_FORMAT: return "FORMAT REJECT";
    case CH32_MP3_CORRUPT_FILE:       return "FILE CORRUPT";
    case CH32_MP3_CLOCK_ERROR:        return "AUDIO CLOCK ERR";
    default:                          return "UNKNOWN ERROR";
    }
}

static void MP3Test_ShowStreamInfo(void)
{
    CH32_MP3_StreamInfo info;
    const char *formatText;

    CH32_MP3_GetStreamInfo(&info);
    if (info.sampleRate == 0U)
        return;

    LCD_Clear();
    LCD_PrintNum((uint16_t)info.sampleRate);
    LCD_WriteString("HZ ");
    LCD_PrintNum(info.bitsPerSample);
    LCD_WriteString("BIT");
    LCD_SetCursor(1, 0);
    LCD_WriteString(info.channels == 1U ? "MONO " : "STEREO ");
    LCD_PrintNum((uint16_t)(info.bitrate / 1000U));
    LCD_WriteString("KBPS");
    App_WaitMs(1500U);

    if (info.mpegVersion == CH32_MP3_MPEG1)
        formatText = "MPEG1 LAYER3";
    else if (info.mpegVersion == CH32_MP3_MPEG2)
        formatText = "MPEG2 LAYER3";
    else
        formatText = "MPEG2.5 LAYER3";
    LCD_Clear();
    LCD_WriteString(formatText);
    LCD_SetCursor(1, 0);
    LCD_WriteString("F:");
    LCD_PrintNum(info.frameBytes);
    LCD_WriteString(" S:");
    LCD_PrintNum(info.samplesPerFrame);
    App_WaitMs(1500U);
}

static void MP3Test_Play(const char *path)
{
    CH32_MP3_Status status;

    LCD_Clear();
    LCD_WriteString("PLAYING:");
    LCD_SetCursor(1, 0);
    LCD_WriteString(path);
    Relay_Set(1);
    App_WaitMs(300U);
    status = CH32_MP3_PlayFile(path, HAPPYBELL_MP3_TEST_VOLUME);
    Relay_Set(0);
    LCD_Clear();
    LCD_WriteString(MP3Test_StatusText(status));
    LCD_SetCursor(1, 0);
    LCD_WriteString("P:");
    LCD_PrintNum(CH32_MP3_GetDecodedPeak());
    LCD_WriteString(" D:");
    LCD_PrintNum(CH32_MP3_GetCompletedDMASlots());
    App_WaitMs(1200U);

    LCD_Clear();
    LCD_WriteString(MP3Test_StatusText(status));
    LCD_SetCursor(1, 0);
    LCD_WriteString("UNDERFLOW:");
    LCD_PrintNum((uint16_t)CH32_MP3_GetUnderflowCount());
    App_WaitMs(1200U);
    if (status == CH32_MP3_OK)
        MP3Test_ShowStreamInfo();
}

int main(void)
{
    uint8_t key;
    char encryptedPath[32];
    Storage_FindResult findResult;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    LCD_Init();
    KEY_Init();
    RelayLight_Init();

    LCD_Clear();
    LCD_WriteString("ENC+MP3 V10");
    LCD_SetCursor(1, 0);
    LCD_WriteString("AUTO GAIN VOL255");
    App_WaitMs(200U);

    if (SystemCoreClock != 96000000U) {
        LCD_Clear();
        LCD_WriteString("CLOCK ERROR");
        LCD_SetCursor(1, 0);
        LCD_WriteString("NOT 96MHZ");
        while (1)
            App_WaitMs(100U);
    }

    LCD_Clear();
    LCD_WriteString("FIXED POINT TEST");
    LCD_SetCursor(1, 0);
    if (!CH32_MP3_ArithmeticSelfTest()) {
        LCD_WriteString("ARITH FAIL");
        while (1)
            App_WaitMs(100U);
    }
    LCD_WriteString("ARITH PASS");
    App_WaitMs(800U);
    Relay_Set(0);

    LCD_Clear();
    LCD_WriteString("MOUNTING SD");
    LCD_SetCursor(1, 0);
    LCD_WriteString("PLEASE WAIT");

    if (!Storage_Mount()) {
        LCD_Clear();
        LCD_WriteString("SD CARD ERROR");
        while (1)
            App_WaitMs(100U);
    }

    LCD_Clear();
    LCD_WriteString("SD MOUNTED");
    LCD_SetCursor(1, 0);
    LCD_WriteString("CHECKING FILE");

    findResult = Storage_FindFirstFileByExtension(
        HAPPYBELL_MP3_TEST_ENC_FOLDER, ".enc", encryptedPath,
        sizeof(encryptedPath));
    if (findResult != STORAGE_FIND_OK) {
        LCD_Clear();
        if (findResult == STORAGE_FIND_NOT_FOUND)
            LCD_WriteString("NO ENC FILE");
        else if (findResult == STORAGE_FIND_PATH_TOO_LONG)
            LCD_WriteString("ENC PATH LONG");
        else
            LCD_WriteString("SD DIR ERROR");
        LCD_SetCursor(1, 0);
        LCD_WriteString(HAPPYBELL_MP3_TEST_ENC_FOLDER);
        while (1)
            App_WaitMs(100U);
    }

    LCD_Clear();
    LCD_WriteString("ENC FOUND");
    LCD_SetCursor(1, 0);
    LCD_WriteString(encryptedPath);
    App_WaitMs(300U);

    MP3Test_Play(encryptedPath);

    while (1) {
        LCD_Clear();
        LCD_WriteString("UP:ENC DOWN:MP3");
        LCD_SetCursor(1, 0);
        LCD_WriteString(encryptedPath);
        do {
            key = KEY_GetKey();
            App_WaitMs(20U);
        } while (key == KEY_NONE);
        if (key & KEY_UP)
            MP3Test_Play(encryptedPath);
        else if (key & KEY_DOWN) {
            if (Storage_FileExists(HAPPYBELL_MP3_TEST_PLAIN_PATH))
                MP3Test_Play(HAPPYBELL_MP3_TEST_PLAIN_PATH);
            else {
                LCD_Clear();
                LCD_WriteString("PLAIN MISSING");
                LCD_SetCursor(1, 0);
                LCD_WriteString(HAPPYBELL_MP3_TEST_PLAIN_PATH);
                App_WaitMs(1200U);
            }
        }
    }
}

#endif
