#include "relay_light.h"
#include "ch32v20x.h"

/* RLY_IN=PA3 (amp relay), LGT_IN=PB0 (light), push-pull outputs.
 * HIGH = on (esp_bell_idf convention; PCB driver-stage polarity
 * unconfirmed -- verify on hardware). Plain GPIO writes; settle
 * delays live in setRelay() in main.c. */

void RelayLight_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

    gpio.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &gpio);
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, Bit_RESET);

    gpio.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &gpio);
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
}

void Relay_Set(uint8_t on)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, on ? Bit_SET : Bit_RESET);
}

void Light_Set(uint8_t on)
{
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, on ? Bit_SET : Bit_RESET);
}
