#include "rtc.h"

/*
 * ============================================================
 * DS1307 RTC
 *
 * CH32V203G8R6 custom PCB
 *
 * PB6 -> I2C1_SCL
 * PB7 -> I2C1_SDA
 *
 * DS1307 address = 0x68
 * ============================================================
 */


/*
 * ------------------------------------------------------------
 * BCD conversion
 * ------------------------------------------------------------
 */

uint8_t RTC_BCDToDec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}


uint8_t RTC_DecToBCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}


/*
 * ------------------------------------------------------------
 * I2C initialization
 * ------------------------------------------------------------
 */

void RTC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    /*
     * Enable GPIOB clock.
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOB,
        ENABLE
    );

    /*
     * Enable I2C1 clock.
     */
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_I2C1,
        ENABLE
    );

    /*
     * --------------------------------------------------------
     * PB6 -> I2C1_SCL
     * PB7 -> I2C1_SDA
     *
     * I2C requires open-drain outputs.
     * --------------------------------------------------------
     */

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_6 |
        GPIO_Pin_7;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_OD;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);


    /*
     * --------------------------------------------------------
     * I2C configuration
     * --------------------------------------------------------
     */

    I2C_InitStructure.I2C_ClockSpeed =
        100000;

    I2C_InitStructure.I2C_Mode =
        I2C_Mode_I2C;

    I2C_InitStructure.I2C_DutyCycle =
        I2C_DutyCycle_2;

    I2C_InitStructure.I2C_OwnAddress1 =
        0x00;

    I2C_InitStructure.I2C_Ack =
        I2C_Ack_Enable;

    I2C_InitStructure.I2C_AcknowledgedAddress =
        I2C_AcknowledgedAddress_7bit;

    I2C_Init(
        I2C1,
        &I2C_InitStructure
    );


    /*
     * Enable I2C1.
     */
    I2C_Cmd(I2C1, ENABLE);

}


/*
 * ------------------------------------------------------------
 * I2C STOP
 * ------------------------------------------------------------
 */

static void RTC_Stop(void)
{
    I2C_GenerateSTOP(
        I2C1,
        ENABLE
    );
}


/*
 * ------------------------------------------------------------
 * Check DS1307 response
 * ------------------------------------------------------------
 */

RTC_Status RTC_Begin(void)
{
    uint32_t timeout = 100000;


    /*
     * START condition.
     */
    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );


    /*
     * Wait for START condition.
     */
    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_NO_RESPONSE;
        }
    }


    /*
     * Send DS1307 address + WRITE.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Transmitter
    );


    /*
     * Wait for ACK.
     */
    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_NO_RESPONSE;
        }
    }


    /*
     * DS1307 responded.
     */
    RTC_Stop();

    return RTC_OK;
}


/*
 * ------------------------------------------------------------
 * Write one DS1307 register
 * ------------------------------------------------------------
 */

RTC_Status RTC_WriteRegister(
    uint8_t reg,
    uint8_t data
)
{
    uint32_t timeout;


    /*
     * START
     */
    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Address + WRITE
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Transmitter
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Register address.
     */
    I2C_SendData(
        I2C1,
        reg
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_TRANSMITTING
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Register data.
     */
    I2C_SendData(
        I2C1,
        data
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_TRANSMITTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * STOP
     */
    RTC_Stop();

    return RTC_OK;
}


/*
 * ------------------------------------------------------------
 * Read one DS1307 register
 * ------------------------------------------------------------
 */

RTC_Status RTC_ReadRegister(
    uint8_t reg,
    uint8_t *data
)
{
    uint32_t timeout;


    /*
     * --------------------------------------------------------
     * First transaction:
     *
     * Tell DS1307 which register we want.
     * --------------------------------------------------------
     */

    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Address + WRITE.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Transmitter
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Register address.
     */
    I2C_SendData(
        I2C1,
        reg
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_TRANSMITTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * --------------------------------------------------------
     * Repeated START
     * --------------------------------------------------------
     */

    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Address + READ.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Receiver
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * We only need one byte.
     *
     * Disable ACK before receiving final byte.
     */
    I2C_AcknowledgeConfig(
        I2C1,
        DISABLE
    );

    /*
     * Wait for received byte.
     */
    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_RECEIVED
        )
    )
    {
        if(timeout-- == 0)
        {
            I2C_AcknowledgeConfig(
                I2C1,
                ENABLE
            );

            RTC_Stop();

            return RTC_READ_ERROR;
        }
    }


    /*
     * Read data.
     */
    *data = I2C_ReceiveData(I2C1);


    /*
     * STOP.
     */
    RTC_Stop();


    /*
     * Re-enable ACK.
     */
    I2C_AcknowledgeConfig(
        I2C1,
        ENABLE
    );


    return RTC_OK;
}


/*
 * ------------------------------------------------------------
 * Read complete date/time
 *
 * DS1307 registers:
 *
 * 00 seconds
 * 01 minutes
 * 02 hours
 * 03 day
 * 04 date
 * 05 month
 * 06 year
 * ------------------------------------------------------------
 */

RTC_Status RTC_ReadTime(
    RTC_Time *time
)
{
    uint8_t buffer[7];
    uint8_t i;

    uint32_t timeout;

    if (time == NULL)
        return RTC_READ_ERROR;


    /*
     * Start transaction.
     */
    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Address + WRITE.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Transmitter
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Start at seconds register.
     */
    I2C_SendData(
        I2C1,
        0x00
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_TRANSMITTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Repeated START.
     */
    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Address + READ.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Receiver
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_READ_ERROR;
        }
    }


    /*
     * Read seven registers.
     */
    for(i = 0; i < 7; i++)
    {
        if(i == 6)
        {
            /*
             * Last byte:
             * send NACK.
             */
            I2C_AcknowledgeConfig(
                I2C1,
                DISABLE
            );
        }
        else
        {
            I2C_AcknowledgeConfig(
                I2C1,
                ENABLE
            );
        }


        timeout = 100000;

        while(
            !I2C_CheckEvent(
                I2C1,
                I2C_EVENT_MASTER_BYTE_RECEIVED
            )
        )
        {
            if(timeout-- == 0)
            {
                I2C_AcknowledgeConfig(
                    I2C1,
                    ENABLE
                );

                RTC_Stop();

                return RTC_READ_ERROR;
            }
        }


        buffer[i] =
            I2C_ReceiveData(I2C1);
    }


    /*
     * STOP.
     */
    RTC_Stop();


    /*
     * Re-enable ACK.
     */
    I2C_AcknowledgeConfig(
        I2C1,
        ENABLE
    );


    /*
     * Convert BCD to decimal.
     */

    time->seconds =
        RTC_BCDToDec(buffer[0] & 0x7F);

    time->minutes =
        RTC_BCDToDec(buffer[1] & 0x7F);

    /*
     * DS1307 hour register.
     *
     * Handle 24-hour mode.
     */
    time->hours =
        RTC_BCDToDec(buffer[2] & 0x3F);

    time->day =
        RTC_BCDToDec(buffer[3] & 0x07);

    time->date =
        RTC_BCDToDec(buffer[4] & 0x3F);

    time->month =
        RTC_BCDToDec(buffer[5] & 0x1F);

    time->year =
        RTC_BCDToDec(buffer[6]);


    return RTC_OK;
}


/*
 * ------------------------------------------------------------
 * Set complete date/time
 * ------------------------------------------------------------
 */

RTC_Status RTC_SetTime(
    RTC_Time *time
)
{
    uint8_t buffer[7];

    uint8_t i;

    uint32_t timeout;

    if (time == NULL)
        return RTC_WRITE_ERROR;


    buffer[0] = RTC_DecToBCD(time->seconds);
    buffer[1] = RTC_DecToBCD(time->minutes);
    buffer[2] = RTC_DecToBCD(time->hours);
    buffer[3] = RTC_DecToBCD(time->day);
    buffer[4] = RTC_DecToBCD(time->date);
    buffer[5] = RTC_DecToBCD(time->month);
    buffer[6] = RTC_DecToBCD(time->year);


    /*
     * START.
     */
    I2C_GenerateSTART(
        I2C1,
        ENABLE
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_MODE_SELECT
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Address + WRITE.
     */
    I2C_Send7bitAddress(
        I2C1,
        RTC_I2C_ADDRESS << 1,
        I2C_Direction_Transmitter
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Start at seconds register.
     */
    I2C_SendData(
        I2C1,
        0x00
    );

    timeout = 100000;

    while(
        !I2C_CheckEvent(
            I2C1,
            I2C_EVENT_MASTER_BYTE_TRANSMITTED
        )
    )
    {
        if(timeout-- == 0)
        {
            RTC_Stop();
            return RTC_WRITE_ERROR;
        }
    }


    /*
     * Write seven registers.
     */
    for(i = 0; i < 7; i++)
    {
        I2C_SendData(
            I2C1,
            buffer[i]
        );

        timeout = 100000;

        while(
            !I2C_CheckEvent(
                I2C1,
                I2C_EVENT_MASTER_BYTE_TRANSMITTED
            )
        )
        {
            if(timeout-- == 0)
            {
                RTC_Stop();
                return RTC_WRITE_ERROR;
            }
        }
    }


    /*
     * STOP.
     */
    RTC_Stop();

    return RTC_OK;
}
