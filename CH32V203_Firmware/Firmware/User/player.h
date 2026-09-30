#ifndef __PLAYER_H
#define __PLAYER_H

#include <stdint.h>

/* Async front end over the blocking CH32_MP3_PlayFile(): Player_Play()
 * queues a path to the audio task, other tasks poll/stop. */
void Player_Init(void);
uint8_t Player_Play(const char *path);
void Player_Stop(void);
uint8_t Player_IsPlaying(void);
void Player_Service(void);
void Player_SetWait(uint8_t stopKeys, uint32_t endTime, uint16_t pollMs);
void Player_AudioWaitOneMs(void);

#endif
