#ifndef __SD_SPI_H
#define __SD_SPI_H

#include "ch32v20x.h"
#include "debug.h"

/*
 * ============================================================
 * SD CARD SPI PINS - CH32V203G8R6
 * ============================================================
 *
 * PA4 -> CS
 * PA5 -> SCK
 * PA6 -> MISO / DO
 * PA7 -> MOSI / DI
 *
 * ============================================================
 */

#define SD_CS_PORT    GPIOA
#define SD_CS_PIN     GPIO_Pin_4


#define SD_CS_LOW() \
    GPIO_ResetBits(SD_CS_PORT, SD_CS_PIN)


#define SD_CS_HIGH() \
    GPIO_SetBits(SD_CS_PORT, SD_CS_PIN)


void SD_SPI_Init(void);

uint8_t SD_SPI_TransferByte(uint8_t data);

#endif