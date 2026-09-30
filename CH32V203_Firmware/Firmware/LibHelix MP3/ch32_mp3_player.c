#include "ch32_mp3_player.h"
#include "ch32v20x.h"
#include "ff.h"
#include "mp3dec.h"
#include "assembly.h"
#include <string.h>
#include "happybell_build.h"
#include "ch32v20x_it.h"

#if HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_FULL
#include "player.h"
#define Audio_WaitOneMs() Player_AudioWaitOneMs()
#else
#include "debug.h"
#define Audio_WaitOneMs() Delay_Ms(1U)
#endif

/*
 * ------------------------------------------------------------------------
 * DMA-driven audio output (PA8/TIM1_CH1 PWM carrier -> external LPF -> AUX_OUT)
 * ------------------------------------------------------------------------
 *
 * TIM1_CH1 still generates the fixed 8-bit PWM carrier (ARR 255, nominally
 * 375 kHz at a 96 MHz timer clock) that feeds the schematic AUX net and the
 * board's analog low-pass filter stage to AUX_OUT, exactly as before.
 *
 * TIM2 still free-runs at the decoded sample rate (see Audio_Init()), but
 * instead of firing a per-sample CPU interrupt it now issues a DMA request
 * on every update event. DMA1 Channel2 moves each prepared sample directly
 * from a decoded PCM slot into TIM1->CH1CVR with no CPU involvement per
 * sample; the CPU is only interrupted once per 576-sample slot (DMA
 * Transfer-Complete), to hand off to the next ready slot or fall back to a
 * small circularly-looped silence buffer so the carrier never stalls.
 *
 * Volume scaling and the 0..255 PWM-duty bias that used to run per-sample
 * inside the old TIM2 ISR now run once per decoded granule, in
 * Audio_PrepareSlot(), before that row is marked ready for DMA. All deployed
 * audio receives bounded peak-aware gain, with immediate gain reduction and a
 * gradual interpolated rise so quiet assets become louder without wraparound.
 *
 * WCH's CH32FV2x/V3x reference manual DMA request table confirms TIM2_UP on
 * DMA1 Channel2. That channel also carries SPI1_RX and TIM1_CH1, so it must
 * not be reassigned to an SD-card SPI DMA path without re-checking ownership.
 */

#define INPUT_BUFFER_SIZE       768U
#define INPUT_REFILL_LEVEL      (INPUT_BUFFER_SIZE - 95U)
#define PCM_SAMPLES_PER_SLOT    576U
#define NO_SLOT                 0xffU
#define SILENCE_LEN             32U   /* small circular fill while no slot is ready */
#define PWM_MID                 128U  /* silence / idle duty (8-bit PWM midpoint) */
#define DMA_WAIT_TIMEOUT_MS     500U
#define AUDIO_TIMER_CLOCK_HZ    96000000U
#define MAX_CONSECUTIVE_BAD_FRAMES 8U
#define AUDIO_GAIN_MIN_Q8       254U  /* attenuates full-scale PCM below PWM rails */
#define AUDIO_GAIN_MAX_Q8       8192U /* 32x / +30 dB for very quiet source material */
#define AUDIO_GAIN_RISE_Q8      256U  /* +1x/block recovery, interpolated across samples */
#define AUDIO_GAIN_TARGET       32512U /* 99.2% PCM scale; duty remains within 1..254 */
#define AUDIO_GAIN_MIN_PEAK     512U  /* do not chase digital silence/background noise */

static unsigned char inputBuffer[INPUT_BUFFER_SIZE];
/* Holds raw decoded PCM until Audio_PrepareSlot() converts it in place to
 * ready-to-DMA 0..255 PWM-duty values (same buffer, no extra RAM). */
static short pcmBuffer[2U * PCM_SAMPLES_PER_SLOT];
static short silenceSample;
static volatile uint16_t slotCount[2];
static volatile uint8_t slotState[2]; /* 0 free, 1 ready, 2 playing */
static volatile uint8_t playingSlot = NO_SLOT;
static volatile uint8_t silenceActive;
static volatile uint8_t outputVolume;
static volatile uint32_t underflowCount;
static volatile uint16_t decodedPeak;
static volatile uint16_t completedDMASlots;
static volatile uint8_t gapAfterPlayback;
static volatile uint8_t stopRequest;
static volatile uint32_t sampleClockBaseTicks;
static volatile uint32_t sampleClockRemainder;
static volatile uint32_t sampleClockDivisor;
static volatile uint32_t sampleClockAccumulator;
static CH32_MP3_StreamInfo streamInfo;
static uint16_t audioGainQ8;

/* .enc is the complete MP3 file XORed from byte zero with this repeating key.
 * There is no IV or wrapper header. Use the absolute file offset for the key
 * phase so seeks and partial FatFs reads cannot desynchronize decryption. */
static const uint8_t xorKey[4]
    __attribute__((section(".rodata.mp3_xor_key"))) = { 'B', 'K', '2', '6' };

static void DecryptBuffer(uint8_t *data, uint32_t len, FSIZE_t fileOffset)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= xorKey[((uint32_t)fileOffset + i) & 3U];
    }
}

static uint8_t IsEncryptedPath(const char *path)
{
    const char *ext = strrchr(path, '.');

    if (!ext || ext[1] == '\0' || ext[2] == '\0' || ext[3] == '\0' ||
        ext[4] != '\0')
        return 0;
    return (ext[1] == 'e' || ext[1] == 'E') &&
           (ext[2] == 'n' || ext[2] == 'N') &&
           (ext[3] == 'c' || ext[3] == 'C');
}

static uint8_t HasLayer3Header(const uint8_t *data, uint32_t len)
{
    uint8_t version;
    uint8_t layer;
    uint8_t bitrate;
    uint8_t sampleRate;

    if (!data || len < 4U || data[0] != 0xffU || (data[1] & 0xe0U) != 0xe0U)
        return 0;
    version = (data[1] >> 3) & 3U;
    layer = (data[1] >> 1) & 3U;
    bitrate = data[2] >> 4;
    sampleRate = (data[2] >> 2) & 3U;
    return version == 3U && layer == 1U &&
           (bitrate == 5U || bitrate == 9U) && sampleRate == 2U &&
           ((data[3] >> 6) & 3U) == 3U;
}

/* Validate the decrypted preamble and skip ID3v2 before streaming. The
 * encrypted files on the mounted card carry a normal ID3v2.4 tag inside the
 * XOR stream, followed directly by MPEG Layer III frames. */
static CH32_MP3_Status PrepareAudioFile(FIL *file, uint8_t encrypted)
{
    UINT bytesRead;
    FRESULT readResult;
    FSIZE_t tagEnd;
    FSIZE_t audioStart = 0;
    uint8_t index;
    uint8_t id3Found = 0;

    if (f_lseek(file, 0) != FR_OK)
        return CH32_MP3_SD_READ_ERROR;
    readResult = f_read(file, inputBuffer, 10U, &bytesRead);
    if (readResult != FR_OK)
        return CH32_MP3_SD_READ_ERROR;
    if (bytesRead < 4U)
        return encrypted ? CH32_MP3_DECRYPT_ERROR : CH32_MP3_CORRUPT_FILE;
    if (encrypted)
        DecryptBuffer(inputBuffer, bytesRead, 0U);

    if (bytesRead == 10 && inputBuffer[0] == 'I' && inputBuffer[1] == 'D' &&
        inputBuffer[2] == '3') {
        id3Found = 1;
        for (index = 6U; index < 10U; index++) {
            if (inputBuffer[index] & 0x80U)
                return CH32_MP3_CORRUPT_FILE;
        }
        tagEnd = 10U +
            ((FSIZE_t)(inputBuffer[6] & 0x7fU) << 21) +
            ((FSIZE_t)(inputBuffer[7] & 0x7fU) << 14) +
            ((FSIZE_t)(inputBuffer[8] & 0x7fU) << 7) +
            (FSIZE_t)(inputBuffer[9] & 0x7fU);
        if (inputBuffer[5] & 0x10U)
            tagEnd += 10U;
        if (tagEnd > f_size(file))
            return CH32_MP3_CORRUPT_FILE;
        audioStart = tagEnd;
    } else if (encrypted && !HasLayer3Header(inputBuffer, bytesRead)) {
        return CH32_MP3_DECRYPT_ERROR;
    }

    if (f_lseek(file, audioStart) != FR_OK)
        return CH32_MP3_SD_READ_ERROR;
    if (encrypted || id3Found) {
        readResult = f_read(file, inputBuffer, 4U, &bytesRead);
        if (readResult != FR_OK)
            return CH32_MP3_SD_READ_ERROR;
        if (bytesRead != 4U)
            return CH32_MP3_CORRUPT_FILE;
        if (encrypted)
            DecryptBuffer(inputBuffer, bytesRead, audioStart);
        if (!HasLayer3Header(inputBuffer, bytesRead))
            return id3Found ? CH32_MP3_UNSUPPORTED_FORMAT :
                   CH32_MP3_DECRYPT_ERROR;
    }
    return f_lseek(file, audioStart) == FR_OK ?
           CH32_MP3_OK : CH32_MP3_SD_READ_ERROR;
}

static uint16_t Layer3FrameBytes(const MP3FrameInfo *info,
                                 const uint8_t *header)
{
    uint32_t bytes;
    if (!info || !header ||
        (info->bitrate != 64000 && info->bitrate != 128000))
        return 0;
    bytes = (144U * (uint32_t)info->bitrate) / 32000U;
    bytes += (header[2] >> 1) & 1U;
    return bytes <= 0xffffU ? (uint16_t)bytes : 0U;
}

typedef enum {
    AUDIO_INIT_OK = 0,
    AUDIO_INIT_UNSUPPORTED_RATE,
    AUDIO_INIT_CLOCK_ERROR
} Audio_InitStatus;

static void DMA_StartSilence(void)
{
    if (silenceActive)
        return; /* already looping on silence; nothing to reprogram */

    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA1_Channel2->MADDR = (uint32_t)&silenceSample;
    DMA1_Channel2->CNTR = SILENCE_LEN;
    DMA1_Channel2->CFGR = (DMA1_Channel2->CFGR | DMA_Mode_Circular) &
                          ~DMA_MemoryInc_Enable;
    playingSlot = NO_SLOT;
    silenceActive = 1;
    DMA_Cmd(DMA1_Channel2, ENABLE);
}

static void DMA_StartSlot(uint8_t slot)
{
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA1_Channel2->MADDR =
        (uint32_t)&pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT];
    DMA1_Channel2->CNTR = slotCount[slot];
    DMA1_Channel2->CFGR = (DMA1_Channel2->CFGR & ~DMA_Mode_Circular) |
                          DMA_MemoryInc_Enable; /* one-shot: TC fires at slot end */
    slotState[slot] = 2;
    playingSlot = slot;
    silenceActive = 0;
    DMA_Cmd(DMA1_Channel2, ENABLE);
}

/* Arms the next ready slot, or falls back to silence. Only takes effect
 * while the carrier is idle (playingSlot == NO_SLOT) -- while a real slot
 * is playing, the DMA Transfer-Complete interrupt is what calls this next.
 * cameFromRealSlot marks a fresh underflow (gapAfterPlayback) only when a
 * real slot just finished and nothing else was ready in time; it must NOT
 * be set on every completion, or a normal back-to-back handoff between two
 * already-ready slots would be miscounted as an underflow. */
static void Audio_StartReadySlot(uint8_t cameFromRealSlot)
{
    uint8_t slot;

    if (playingSlot != NO_SLOT)
        return;

    for (slot = 0; slot < 2; slot++) {
        if (slotState[slot] == 1) {
            if (gapAfterPlayback) {
                underflowCount++;
                gapAfterPlayback = 0;
            }
            DMA_StartSlot(slot);
            return;
        }
    }

    if (cameFromRealSlot)
        gapAfterPlayback = 1;
    DMA_StartSilence();
}

/* A loud slot reduces gain before it is handed to DMA, while quiet slots raise
 * gain gradually. Starting at the ceiling is safe because the complete slot is
 * peak-scanned before any sample reaches DMA. With the 32x Q8 ceiling, the
 * worst intermediate magnitude is 32768 * 8192 = 268,435,456, safely inside
 * int32_t. Final saturation remains a hard guard against corrupt PCM or
 * arithmetic edge cases. */
static void Audio_PrepareSlot(uint8_t slot, uint16_t count)
{
    uint16_t i;
    uint16_t blockPeak = 0U;
    uint16_t desiredGainQ8;
    uint16_t nextGainQ8;
    uint16_t startGainQ8;
    uint16_t endGainQ8;
    uint16_t gainQ8;
    uint32_t gainAccumulatorQ16;
    uint32_t gainStepQ16 = 0U;
    int32_t s;
    int32_t magnitude;

    for (i = 0; i < count; i++) {
        s = pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT + i];
        magnitude = (s < 0) ? -s : s;
        if ((uint32_t)magnitude > blockPeak)
            blockPeak = (uint16_t)magnitude;
        if ((uint32_t)magnitude > decodedPeak)
            decodedPeak = (uint16_t)magnitude;
    }

    startGainQ8 = audioGainQ8;
    if (blockPeak >= AUDIO_GAIN_MIN_PEAK) {
        desiredGainQ8 = (uint16_t)(((uint32_t)AUDIO_GAIN_TARGET << 8) /
                                  blockPeak);
        if (desiredGainQ8 < AUDIO_GAIN_MIN_Q8)
            desiredGainQ8 = AUDIO_GAIN_MIN_Q8;
        else if (desiredGainQ8 > AUDIO_GAIN_MAX_Q8)
            desiredGainQ8 = AUDIO_GAIN_MAX_Q8;

        if (desiredGainQ8 <= audioGainQ8) {
            audioGainQ8 = desiredGainQ8;
            startGainQ8 = desiredGainQ8;
        } else {
            nextGainQ8 = audioGainQ8 + AUDIO_GAIN_RISE_Q8;
            audioGainQ8 = (nextGainQ8 < desiredGainQ8) ?
                          nextGainQ8 : desiredGainQ8;
        }
    }
    endGainQ8 = audioGainQ8;
    if (endGainQ8 > startGainQ8 && count != 0U)
        gainStepQ16 = ((uint32_t)(endGainQ8 - startGainQ8) << 16) /
                      count;
    gainAccumulatorQ16 = (uint32_t)startGainQ8 << 16;

    for (i = 0; i < count; i++) {
        s = pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT + i];
        gainQ8 = (uint16_t)(gainAccumulatorQ16 >> 16);
        s = (s * (int32_t)gainQ8) >> 8;
        s = (s * (int32_t)outputVolume) >> 16;
        if (s > 127)
            s = 127;
        else if (s < -128)
            s = -128;
        pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT + i] =
            (short)(s + (int32_t)PWM_MID);
        gainAccumulatorQ16 += gainStepQ16;
    }
}

static Audio_InitStatus Audio_Init(uint32_t sampleRate)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef timer;
    TIM_OCInitTypeDef pwm;
    NVIC_InitTypeDef nvic;
    DMA_InitTypeDef dma;
    uint32_t samplePeriod;

    if (sampleRate < 8000U || sampleRate > 48000U)
        return AUDIO_INIT_UNSUPPORTED_RATE;
    if (SystemCoreClock != AUDIO_TIMER_CLOCK_HZ ||
        (RCC->CFGR0 & RCC_PPRE1) != RCC_PPRE1_DIV2)
        return AUDIO_INIT_CLOCK_ERROR;
    samplePeriod = AUDIO_TIMER_CLOCK_HZ / sampleRate;
    if (samplePeriod == 0U || samplePeriod > 65536U ||
        (samplePeriod == 65536U &&
         (AUDIO_TIMER_CLOCK_HZ % sampleRate) != 0U))
        return AUDIO_INIT_UNSUPPORTED_RATE;
    sampleClockBaseTicks = samplePeriod;
    sampleClockRemainder = AUDIO_TIMER_CLOCK_HZ % sampleRate;
    sampleClockDivisor = sampleRate;
    sampleClockAccumulator = 0U;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* TIM1 is an advanced timer. TIM_OC1Init() consumes complementary-output
     * and idle-state members as well as the four main PWM members, so the
     * whole structure must be initialized deterministically. Reset all three
     * audio peripherals before each file playback. */
    TIM_DeInit(TIM1);
    TIM_DeInit(TIM2);
    DMA_DeInit(DMA1_Channel2);
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, DISABLE);

    gpio.GPIO_Pin = GPIO_Pin_8; /* schematic AUX: PA8/TIM1_CH1 */
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    timer.TIM_Prescaler = 0;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = 255;     /* 375 kHz, 8-bit PWM at 96 MHz */
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &timer);

    TIM_OCStructInit(&pwm);
    pwm.TIM_OCMode = TIM_OCMode_PWM1;
    pwm.TIM_OutputState = TIM_OutputState_Enable;
    pwm.TIM_Pulse = PWM_MID;
    pwm.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &pwm);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    timer.TIM_Period = (uint16_t)(samplePeriod - 1U);
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_ARRPreloadConfig(TIM2, DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_DMACmd(TIM2, TIM_DMA_Update, ENABLE);

    if (sampleClockRemainder != 0U) {
        nvic.NVIC_IRQChannel = TIM2_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority = 0;
        nvic.NVIC_IRQChannelSubPriority = 0;
        nvic.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic);
        TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    }

    silenceSample = (short)PWM_MID;

    dma.DMA_PeripheralBaseAddr = (uint32_t)&TIM1->CH1CVR;
    dma.DMA_MemoryBaseAddr = (uint32_t)&silenceSample;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = SILENCE_LEN;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Disable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &dma);
    DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

    nvic.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    playingSlot = NO_SLOT;
    silenceActive = 1;
    DMA_Cmd(DMA1_Channel2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    return AUDIO_INIT_OK;
}

/* MPEG-1 contains two 576-sample granules per frame. Queue each row as soon
 * as Helix synthesizes it so the following row can decode while DMA plays the
 * preceding one. This avoids a forced silence gap at every frame boundary and
 * uses the existing two contiguous rows instead of adding another RAM buffer. */
static int Audio_GranuleOutputHook(void *context, int granule, short *output,
                                  int outputSamps, int event)
{
    uint16_t waitMs;
    uint8_t slot;

    (void)context;
    if (granule < 0 || granule > 1 || outputSamps <= 0 ||
        outputSamps > (int)PCM_SAMPLES_PER_SLOT)
        return -1;
    slot = (uint8_t)granule;
    if (output != &pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT])
        return -1;

    if (event == MP3_GRANULE_OUTPUT_BEFORE) {
        waitMs = 0;
        while (slotState[slot] != 0U && !stopRequest) {
            if (waitMs++ >= DMA_WAIT_TIMEOUT_MS)
                return -1;
            Audio_WaitOneMs();
        }
        return stopRequest ? -1 : 0;
    }
    if (event != MP3_GRANULE_OUTPUT_AFTER)
        return -1;

    FaultDiag_SetStage(FAULT_STAGE_PCM_PREPARE);
    Audio_PrepareSlot(slot, (uint16_t)outputSamps);
    slotCount[slot] = (uint16_t)outputSamps;
    slotState[slot] = 1;
    FaultDiag_SetStage(FAULT_STAGE_DMA_OUTPUT);
    Audio_StartReadySlot(0);
    return 0;
}

uint8_t CH32_MP3_ArithmeticSelfTest(void)
{
    const int a = -123456789;
    const int b = 987654321;

    if (MULSHIFT32(a, b) != -28389653)
        return 0;
    if (MADD64(0, a, b) != -121932631112635269LL)
        return 0;
    return 1;
}

/* Rates in the 44.1 kHz family do not divide the 96 MHz timer clock evenly.
 * Dither TIM2 between adjacent integer periods with a phase accumulator. The
 * long-term update rate is exact; 32/48 kHz-family assets have zero remainder
 * and never enable this per-sample interrupt. */
void TIM2_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible));
void TIM2_IRQHandler(void)
{
    uint32_t ticks;

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == RESET)
        return;
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    if (sampleClockRemainder == 0U)
        return;
    ticks = sampleClockBaseTicks;
    sampleClockAccumulator += sampleClockRemainder;
    if (sampleClockAccumulator >= sampleClockDivisor) {
        sampleClockAccumulator -= sampleClockDivisor;
        ticks++;
    }
    TIM2->ATRLR = (uint16_t)(ticks - 1U);
}

void DMA1_Channel2_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible));
void DMA1_Channel2_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC2) == RESET)
        return;
    DMA_ClearITPendingBit(DMA1_IT_TC2);

    if (playingSlot != NO_SLOT) {
        /* A real slot just finished. If nothing else is ready,
         * Audio_StartReadySlot() falling through to silence counts as an
         * underflow (matches the previous ISR's gapAfterPlayback logic). */
        if (completedDMASlots != 0xffffU)
            completedDMASlots++;
        slotState[playingSlot] = 0;
        playingSlot = NO_SLOT;
        Audio_StartReadySlot(1);
        return;
    }

    /* Was already looping on silence (fires once per SILENCE_LEN-sample
     * wrap); check again for freshly-decoded data. No new gap here -- we
     * were already silent. */
    Audio_StartReadySlot(0);
}

void CH32_MP3_Stop(void)
{
    DMA_Cmd(DMA1_Channel2, DISABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_DMACmd(TIM2, TIM_DMA_Update, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
    TIM_SetCompare1(TIM1, PWM_MID);
    TIM_Cmd(TIM1, DISABLE);
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    playingSlot = NO_SLOT;
    silenceActive = 0;
    slotState[0] = slotState[1] = 0;
}

uint32_t CH32_MP3_GetUnderflowCount(void)
{
    return underflowCount;
}

uint16_t CH32_MP3_GetDecodedPeak(void)
{
    return decodedPeak;
}

uint16_t CH32_MP3_GetCompletedDMASlots(void)
{
    return completedDMASlots;
}

void CH32_MP3_GetStreamInfo(CH32_MP3_StreamInfo *info)
{
    if (info)
        *info = streamInfo;
}

void CH32_MP3_RequestStop(void)
{
    stopRequest = 1;
}

/* Caller must have the SD volume mounted (see storage.c). */
CH32_MP3_Status CH32_MP3_PlayFile(const char *path, uint8_t volume)
{
    FIL file;
    HMP3Decoder decoder;
    MP3FrameInfo info;
    const unsigned char *readPtr = inputBuffer;
    const unsigned char *frameStart;
    UINT bytesRead;
    size_t bytesLeft = 0;
    size_t frameBytes;
    uint32_t decodedFrames = 0;
    uint8_t eof = 0;
    uint8_t audioStarted = 0;
    uint8_t slot = 0;
    uint8_t encrypted;
    uint8_t unsupportedStereoSeen = 0;
    uint8_t mpeg1Frame;
    uint8_t streamedGranules;
    uint8_t consecutiveBadFrames = 0;
    uint16_t currentFrameBytes;
    uint16_t dmaWaitMs;
    FSIZE_t readOffset;
    CH32_MP3_Status status = CH32_MP3_OK;
    Audio_InitStatus audioInitStatus;
    int sync;
    int result = 0;

    FaultDiag_SetStage(FAULT_STAGE_MP3_FILE);
    if (!path || f_open(&file, path, FA_READ) != FR_OK) {
        FaultDiag_SetStage(FAULT_STAGE_NONE);
        return CH32_MP3_FILE_ERROR;
    }

    encrypted = IsEncryptedPath(path);
    memset(&streamInfo, 0, sizeof(streamInfo));
    streamInfo.encrypted = encrypted;
    status = PrepareAudioFile(&file, encrypted);
    if (status != CH32_MP3_OK) {
        f_close(&file);
        FaultDiag_SetStage(FAULT_STAGE_NONE);
        return status;
    }
    decoder = MP3InitDecoder();
    if (!decoder) {
        f_close(&file);
        FaultDiag_SetStage(FAULT_STAGE_NONE);
        return CH32_MP3_DECODER_ERROR;
    }

    outputVolume = volume;
    /* Start short announcements at useful level. Audio_PrepareSlot() scans the
     * entire first block and reduces this before DMA if its peak is loud. */
    audioGainQ8 = AUDIO_GAIN_MAX_Q8;
    underflowCount = 0;
    decodedPeak = 0;
    completedDMASlots = 0;
    gapAfterPlayback = 0;
    stopRequest = 0;
    slotState[0] = slotState[1] = 0;
    playingSlot = NO_SLOT;

    while ((!eof || bytesLeft) && !stopRequest) {
        if (!eof && bytesLeft < INPUT_REFILL_LEVEL) {
            FaultDiag_SetStage(FAULT_STAGE_MP3_READ);
            memmove(inputBuffer, readPtr, bytesLeft);
            readOffset = f_tell(&file);
            if (f_read(&file, inputBuffer + bytesLeft,
                       INPUT_BUFFER_SIZE - bytesLeft, &bytesRead) != FR_OK) {
                status = CH32_MP3_SD_READ_ERROR;
                break;
            }
            if (encrypted)
                DecryptBuffer(inputBuffer + bytesLeft, bytesRead, readOffset);
            bytesLeft += bytesRead;
            readPtr = inputBuffer;
            if (bytesRead == 0)
                eof = 1;
        }
        if (bytesLeft < 4)
            break;

        FaultDiag_SetStage(FAULT_STAGE_MP3_SYNC);
        sync = MP3FindSyncWord(readPtr, (int)bytesLeft);
        if (sync < 0) {
            if (eof)
                break;
            if (bytesLeft > 3) {
                readPtr += bytesLeft - 3;
                bytesLeft = 3;
            }
            continue;
        }
        readPtr += sync;
        bytesLeft -= (size_t)sync;

        if (((readPtr[3] >> 6) & 3U) != 3U) {
            /* MP3FindSyncWord checks only the sync bits. Compressed payload or
             * trailing metadata can contain the same pattern, so an
             * unsupported-looking candidate must not terminate a file that
             * otherwise contains valid frames. Advance one byte and resume
             * sync scanning. */
            unsupportedStereoSeen = 1;
            readPtr++;
            bytesLeft--;
            continue;
        }
        if (MP3GetNextFrameInfo(decoder, &info,
                                (unsigned char *)readPtr) != ERR_MP3_NONE) {
            readPtr++;
            bytesLeft--;
            continue;
        }
        currentFrameBytes = Layer3FrameBytes(&info, readPtr);
        if (currentFrameBytes == 0U) {
            readPtr++;
            bytesLeft--;
            continue;
        }
        if (currentFrameBytes > INPUT_BUFFER_SIZE) {
            status = CH32_MP3_FRAME_TOO_LARGE;
            break;
        }
        /* MPEG-1 mono decodes 1152 samples across both contiguous slots. */
        mpeg1Frame = 1;
        dmaWaitMs = 0;
        if (mpeg1Frame) {
            /* The first frame needs both output rows. After audio starts, only
             * row 0 must be free here; the granule hook waits for row 1 just
             * before Helix writes it. */
            while (((audioStarted && slotState[0] != 0) ||
                    (!audioStarted &&
                     (slotState[0] != 0 || slotState[1] != 0))) &&
                   !stopRequest) {
                if (dmaWaitMs++ >= DMA_WAIT_TIMEOUT_MS) {
                    result = -106;
                    break;
                }
                Audio_WaitOneMs();
            }
            slot = 0;
        } else {
            while (slotState[slot] != 0 && !stopRequest) {
                if (dmaWaitMs++ >= DMA_WAIT_TIMEOUT_MS) {
                    result = -106;
                    break;
                }
                Audio_WaitOneMs();
            }
        }
        if (stopRequest || result == -106)
            break;
        frameStart = readPtr;
        frameBytes = bytesLeft;
        FaultDiag_SetStage(FAULT_STAGE_MP3_DECODE);
        streamedGranules = (mpeg1Frame && audioStarted);
        if (streamedGranules) {
            result = MP3DecodeWithGranuleHook(decoder, &readPtr, &bytesLeft,
                pcmBuffer, 0, Audio_GranuleOutputHook, 0);
        } else {
            result = MP3Decode(decoder, &readPtr, &bytesLeft,
                &pcmBuffer[(uint16_t)slot * PCM_SAMPLES_PER_SLOT], 0);
        }
        if (result == ERR_MP3_INDATA_UNDERFLOW && !eof) {
            readPtr = frameStart;
            bytesLeft = frameBytes;
            if (bytesLeft == INPUT_BUFFER_SIZE) {
                /* A verified frame for this build is smaller than the input
                 * buffer. A full-buffer underflow can therefore be a false
                 * sync header whose fields imply a much larger frame. Drop
                 * that candidate byte and resume scanning instead of
                 * misreporting the file as FRAME TOO LARGE. */
                readPtr++;
                bytesLeft--;
            }
            continue;
        }
        if (result == ERR_MP3_OUTPUT_ABORTED) {
            result = stopRequest ? 0 : -106;
            break;
        }
        if (result == ERR_MP3_MAINDATA_UNDERFLOW) {
            consecutiveBadFrames = 0;
            continue;
        }
        if (result != ERR_MP3_NONE) {
            if (audioStarted && ++consecutiveBadFrames >=
                MAX_CONSECUTIVE_BAD_FRAMES) {
                status = CH32_MP3_CORRUPT_FILE;
                break;
            }
            if (readPtr == frameStart && bytesLeft) {
                readPtr++;
                bytesLeft--;
            }
            continue;
        }

        MP3GetLastFrameInfo(decoder, &info);
        consecutiveBadFrames = 0;
        if (info.layer != 3 || info.version != MPEG1 ||
            info.bitsPerSample != 16 || info.nChans != 1 ||
            info.samprate != 32000 || info.outputSamps != 1152 ||
            (info.bitrate != 64000 && info.bitrate != 128000)) {
            status = CH32_MP3_UNSUPPORTED_FORMAT;
            break;
        }
        decodedFrames++;
        if (!audioStarted) {
            FaultDiag_SetStage(FAULT_STAGE_AUDIO_INIT);
            audioInitStatus = Audio_Init((uint32_t)info.samprate);
            if (audioInitStatus != AUDIO_INIT_OK) {
                status = audioInitStatus == AUDIO_INIT_CLOCK_ERROR ?
                         CH32_MP3_CLOCK_ERROR : CH32_MP3_UNSUPPORTED_RATE;
                break;
            }
            streamInfo.sampleRate = (uint32_t)info.samprate;
            streamInfo.bitrate = (uint32_t)info.bitrate;
            streamInfo.frameBytes = currentFrameBytes;
            streamInfo.samplesPerFrame = (uint16_t)info.outputSamps;
            streamInfo.bitsPerSample = (uint8_t)info.bitsPerSample;
            streamInfo.channels = (uint8_t)info.nChans;
            streamInfo.layer = (uint8_t)info.layer;
            streamInfo.mpegVersion = (uint8_t)info.version;
            audioStarted = 1;
        } else if ((uint32_t)info.samprate != streamInfo.sampleRate ||
                   (uint8_t)info.bitsPerSample != streamInfo.bitsPerSample ||
                   (uint8_t)info.nChans != streamInfo.channels ||
                   (uint8_t)info.layer != streamInfo.layer ||
                   (uint8_t)info.version != streamInfo.mpegVersion ||
                   (uint16_t)info.outputSamps != streamInfo.samplesPerFrame) {
            status = CH32_MP3_CORRUPT_FILE;
            break;
        }
        if (info.outputSamps == (int)(PCM_SAMPLES_PER_SLOT * 2U)) {
            if (!streamedGranules) {
                /* First frame: audio was not initialized when Helix produced
                 * its rows, so prepare and queue both now. Later frames are
                 * handed off granule-by-granule inside the decode call. */
                FaultDiag_SetStage(FAULT_STAGE_PCM_PREPARE);
                Audio_PrepareSlot(0, PCM_SAMPLES_PER_SLOT);
                Audio_PrepareSlot(1, PCM_SAMPLES_PER_SLOT);
                slotCount[0] = slotCount[1] = PCM_SAMPLES_PER_SLOT;
                slotState[0] = slotState[1] = 1;
                FaultDiag_SetStage(FAULT_STAGE_DMA_OUTPUT);
                Audio_StartReadySlot(0);
            }
            slot = 0;
        } else if (info.outputSamps > 0 &&
                   info.outputSamps <= (int)PCM_SAMPLES_PER_SLOT) {
            FaultDiag_SetStage(FAULT_STAGE_PCM_PREPARE);
            Audio_PrepareSlot(slot, (uint16_t)info.outputSamps);
            slotCount[slot] = (uint16_t)info.outputSamps;
            slotState[slot] = 1;
            FaultDiag_SetStage(FAULT_STAGE_DMA_OUTPUT);
            Audio_StartReadySlot(0);
            slot ^= 1U;
        } else {
            status = CH32_MP3_UNSUPPORTED_FORMAT;
            break;
        }
    }

    dmaWaitMs = 0;
    while (!stopRequest && (slotState[0] || slotState[1] || playingSlot != NO_SLOT)) {
        if (dmaWaitMs++ >= DMA_WAIT_TIMEOUT_MS) {
            result = -106;
            break;
        }
        Audio_WaitOneMs();
    }
    CH32_MP3_Stop();
    MP3FreeDecoder(decoder);
    f_close(&file);
    FaultDiag_SetStage(FAULT_STAGE_NONE);

    if (status != CH32_MP3_OK)
        return status;
    if (decodedFrames == 0 && unsupportedStereoSeen)
        return CH32_MP3_UNSUPPORTED_STEREO;
    if (result == -106)
        return CH32_MP3_AUDIO_TIMEOUT;
    if (decodedFrames == 0)
        return encrypted ? CH32_MP3_CORRUPT_FILE : CH32_MP3_DECODER_ERROR;
    return CH32_MP3_OK;
}
