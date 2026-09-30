#ifndef __RTC_H
#define __RTC_H

#include "ch32v20x.h"

/*
 * DS1307 I2C RTC
 *
 * I2C1:
 *   PB6 -> SCL
 *   PB7 -> SDA
 *
 * DS1307 7-bit address:
 *   0x68
 */

#define RTC_I2C_ADDRESS    0x68

typedef enum
{
    RTC_OK = 0,
    RTC_ERROR,
    RTC_NO_RESPONSE,
    RTC_READ_ERROR,
    RTC_WRITE_ERROR
} RTC_Status;

typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;

} RTC_Time;

/*
 * Initialize I2C1 on PB6/PB7.
 */
void RTC_Init(void);

/*
 * Check whether DS1307 responds.
 */
RTC_Status RTC_Begin(void);

/*
 * Read complete time/date from DS1307.
 */
RTC_Status RTC_ReadTime(RTC_Time *time);

/*
 * Write complete time/date to DS1307.
 */
RTC_Status RTC_SetTime(RTC_Time *time);

/*
 * Read one DS1307 register.
 */
RTC_Status RTC_ReadRegister(uint8_t reg, uint8_t *data);

/*
 * Write one DS1307 register.
 */
RTC_Status RTC_WriteRegister(uint8_t reg, uint8_t data);

/*
 * BCD conversion.
 */
uint8_t RTC_BCDToDec(uint8_t bcd);
uint8_t RTC_DecToBCD(uint8_t dec);

#endif