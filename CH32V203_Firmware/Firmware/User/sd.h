#ifndef __SD_H
#define __SD_H

#include "ch32v20x.h"
#include "sd_spi.h"


/*
 * ============================================================
 * SD CARD TYPES
 * ============================================================
 */

#define SD_TYPE_UNKNOWN    0
#define SD_TYPE_SDSC      1
#define SD_TYPE_SDHC      2


/*
 * ============================================================
 * SD STATUS
 * ============================================================
 */

typedef enum
{
    SD_OK = 0,
    SD_ERROR,
    SD_TIMEOUT,
    SD_NO_RESPONSE

} SD_Status;


/*
 * ============================================================
 * FUNCTIONS
 * ============================================================
 */

SD_Status SD_Init(void);

SD_Status SD_ReadBlock(
    uint32_t block,
    uint8_t *buffer
);

SD_Status SD_WriteBlock(
    uint32_t block,
    const uint8_t *buffer
);

uint8_t SD_GetCardType(void);

#endif