#ifndef CH32_MP3_PLAYER_H
#define CH32_MP3_PLAYER_H

#include <stdint.h>

typedef enum {
    CH32_MP3_OK = 0,
    CH32_MP3_FILE_ERROR,
    CH32_MP3_DECODER_ERROR,
    CH32_MP3_UNSUPPORTED_STEREO,
    CH32_MP3_UNSUPPORTED_RATE,
    CH32_MP3_FRAME_TOO_LARGE,
    CH32_MP3_AUDIO_TIMEOUT,
    CH32_MP3_SD_READ_ERROR,
    CH32_MP3_DECRYPT_ERROR,
    CH32_MP3_UNSUPPORTED_FORMAT,
    CH32_MP3_CORRUPT_FILE,
    CH32_MP3_CLOCK_ERROR
} CH32_MP3_Status;

#define CH32_MP3_MPEG1  0U
#define CH32_MP3_MPEG2  1U
#define CH32_MP3_MPEG25 2U

typedef struct {
    uint32_t sampleRate;
    uint32_t bitrate;
    uint16_t frameBytes;
    uint16_t samplesPerFrame;
    uint8_t bitsPerSample;
    uint8_t channels;
    uint8_t layer;
    uint8_t mpegVersion;
    uint8_t encrypted;
} CH32_MP3_StreamInfo;

/* Blocking; run from the bare-metal foreground or the full profile's audio
 * task. Plays .mp3 or XOR-obfuscated .enc. The SD volume must already be
 * mounted through storage.c. */
CH32_MP3_Status CH32_MP3_PlayFile(const char *path, uint8_t volume);

/* Verifies the signed fixed-point multiply primitives used by Helix. */
uint8_t CH32_MP3_ArithmeticSelfTest(void);

/* Ends the current PlayFile early; safe from another task. */
void CH32_MP3_RequestStop(void);

void CH32_MP3_Stop(void);
uint32_t CH32_MP3_GetUnderflowCount(void);
uint16_t CH32_MP3_GetDecodedPeak(void);
uint16_t CH32_MP3_GetCompletedDMASlots(void);
void CH32_MP3_GetStreamInfo(CH32_MP3_StreamInfo *info);

#endif
