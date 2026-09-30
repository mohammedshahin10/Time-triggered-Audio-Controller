#include "happybell_build.h"
#include "ch32v20x.h"

void TIM3_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible));

#if HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_FULL

#include "app_runtime.h"
#include "player.h"
#include "mydef.h"

static volatile uint32_t milliseconds;

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        milliseconds++;
    }
}

void AppRuntime_Init(void)
{
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef nvic;

    milliseconds = 0;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_DeInit(TIM3);
    timer.TIM_Prescaler = 95U;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = 999U;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &timer);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    nvic.NVIC_IRQChannel = TIM3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 7;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    TIM_Cmd(TIM3, ENABLE);
}

uint32_t millis(void)
{
    return milliseconds;
}

void App_DelayMs(uint32_t delayMs)
{
    uint32_t start = millis();

    do {
        Player_Service();
        Light_Service();
    } while ((uint32_t)(millis() - start) < delayMs);
}

#else

void TIM3_IRQHandler(void)
{
    while (1) {
    }
}

#endif
