#include "player.h"
#include "ch32_mp3_player.h"
#include "app_runtime.h"
#include "mydef.h"
#include "keypad.h"
#include "debug.h"
#include <string.h>

#define PLAYER_VOLUME     255
#define PATH_MAX_LEN      64
static char currentPath[PATH_MAX_LEN];
static volatile uint8_t playing;
static uint8_t pending;
static uint8_t stopKeyMask;
static uint16_t keyPollMs;
static uint32_t stopTime;
static uint32_t previousKeyPoll;

void Player_Init(void)
{
    playing = 0;
    pending = 0;
}

void Player_SetWait(uint8_t stopKeys, uint32_t endTime, uint16_t pollMs)
{
    stopKeyMask = stopKeys;
    stopTime = endTime;
    keyPollMs = pollMs;
    previousKeyPoll = millis();
}

uint8_t Player_Play(const char *path)
{
    if (playing)
        Player_Stop();
    strncpy(currentPath, path, PATH_MAX_LEN - 1);
    currentPath[PATH_MAX_LEN - 1] = 0;
    playing = 1;
    pending = 1;
    return 1;
}

void Player_Service(void)
{
    if (!pending)
        return;
    pending = 0;
    CH32_MP3_PlayFile(currentPath, PLAYER_VOLUME);
    playing = 0;
}

void Player_Stop(void)
{
    if (!playing)
        return;
    if (pending) {
        pending = 0;
        playing = 0;
        return;
    }
    CH32_MP3_RequestStop();
}

uint8_t Player_IsPlaying(void)
{
    return playing;
}

void Player_AudioWaitOneMs(void)
{
    uint32_t now;
    uint8_t key;

    Delay_Ms(1);
    Light_Service();
    now = millis();
    if (stopTime && now > stopTime)
        CH32_MP3_RequestStop();
    if (!keyPollMs || (uint32_t)(now - previousKeyPoll) < keyPollMs)
        return;
    previousKeyPoll = now;
    key = KEY_PollEvent();
    if (key & stopKeyMask) {
        KEY_QueueEvent(key);
        CH32_MP3_RequestStop();
    }
}
