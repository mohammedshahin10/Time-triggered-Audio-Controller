#include "sd.h"


/*
 * ============================================================
 * SD COMMANDS
 * ============================================================
 */

#define CMD0        0
#define CMD8        8
#define CMD16       16
#define CMD17       17
#define CMD24       24
#define CMD55       55
#define CMD58       58

#define ACMD41      41


/*
 * ============================================================
 * R1 RESPONSES
 * ============================================================
 */

#define SD_R1_IDLE       0x01
#define SD_R1_READY      0x00
#define SD_R1_ILLEGAL    0x04


/*
 * ============================================================
 * CARD TYPE
 * ============================================================
 */

static uint8_t SD_CardType =
    SD_TYPE_UNKNOWN;


/*
 * ============================================================
 * SEND COMMAND
 * ============================================================
 */
static uint8_t SD_SendCommand(
    uint8_t cmd,
    uint32_t arg,
    uint8_t crc
)
{
    uint8_t response;
    uint8_t i;


    /*
     * Wait until card is ready.
     */
    for(i = 0; i < 100; i++)
    {
        if(
            SD_SPI_TransferByte(0xFF)
            == 0xFF
        )
        {
            break;
        }
    }


    /*
     * Command byte
     */
    SD_SPI_TransferByte(
        0x40 | cmd
    );


    /*
     * Argument
     */
    SD_SPI_TransferByte(
        (uint8_t)(arg >> 24)
    );

    SD_SPI_TransferByte(
        (uint8_t)(arg >> 16)
    );

    SD_SPI_TransferByte(
        (uint8_t)(arg >> 8)
    );

    SD_SPI_TransferByte(
        (uint8_t)arg
    );


    /*
     * CRC
     */
    SD_SPI_TransferByte(crc);


    /*
     * Read R1
     */
    for(i = 0; i < 10; i++)
    {
        response =
            SD_SPI_TransferByte(0xFF);

        if(
            (response & 0x80) == 0
        )
        {
            return response;
        }
    }


    return 0xFF;
}


/*
 * ============================================================
 * SD INIT
 * ============================================================
 */
SD_Status SD_Init(void)
{
    uint8_t response;
    uint8_t ocr[4];
    uint8_t i;

    uint32_t timeout;


    SD_CardType =
        SD_TYPE_UNKNOWN;


    /*
     * --------------------------------------------------------
     * Initialize SPI
     * --------------------------------------------------------
     */
    SD_SPI_Init();


    /*
     * --------------------------------------------------------
     * Card deselected
     * --------------------------------------------------------
     */
    SD_CS_HIGH();


    /*
     * 80 startup clocks
     */
    for(i = 0; i < 10; i++)
    {
        SD_SPI_TransferByte(0xFF);
    }


    /*
     * --------------------------------------------------------
     * CMD0
     * --------------------------------------------------------
     */
    SD_CS_LOW();

    response = SD_SendCommand(
        CMD0,
        0x00000000,
        0x95
    );


    if(response != SD_R1_IDLE)
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_ERROR;
    }


    /*
     * --------------------------------------------------------
     * CMD8
     * --------------------------------------------------------
     */
    response = SD_SendCommand(
        CMD8,
        0x000001AA,
        0x87
    );


    /*
     * ========================================================
     * SD VERSION 2
     * ========================================================
     */
    if(response == SD_R1_IDLE)
    {
        /*
         * Read R7 response
         */
        ocr[0] =
            SD_SPI_TransferByte(0xFF);

        ocr[1] =
            SD_SPI_TransferByte(0xFF);

        ocr[2] =
            SD_SPI_TransferByte(0xFF);

        ocr[3] =
            SD_SPI_TransferByte(0xFF);


        /*
         * Check voltage pattern
         */
        if(
            ocr[2] != 0x01 ||
            ocr[3] != 0xAA
        )
        {
            SD_CS_HIGH();

            SD_SPI_TransferByte(0xFF);

            return SD_ERROR;
        }


        /*
         * ----------------------------------------------------
         * ACMD41
         * ----------------------------------------------------
         */
        timeout = 10000;

        while(timeout--)
        {
            /*
             * CMD55
             */
            response = SD_SendCommand(
                CMD55,
                0x00000000,
                0x01
            );


            if(
                response != SD_R1_IDLE &&
                response != SD_R1_READY
            )
            {
                SD_CS_HIGH();

                SD_SPI_TransferByte(0xFF);

                return SD_ERROR;
            }


            /*
             * ACMD41
             *
             * HCS = 1
             */
            response = SD_SendCommand(
                ACMD41,
                0x40000000,
                0x01
            );


            /*
             * Card ready
             */
            if(response == SD_R1_READY)
            {
                break;
            }


            /*
             * Still initializing
             */
            if(response != SD_R1_IDLE)
            {
                SD_CS_HIGH();

                SD_SPI_TransferByte(0xFF);

                return SD_ERROR;
            }


            Delay_Ms(1);
        }


        /*
         * Timeout
         */
        if(response != SD_R1_READY)
        {
            SD_CS_HIGH();

            SD_SPI_TransferByte(0xFF);

            return SD_TIMEOUT;
        }


        /*
         * ----------------------------------------------------
         * CMD58
         * ----------------------------------------------------
         */
        response = SD_SendCommand(
            CMD58,
            0x00000000,
            0xFF
        );


        if(response != SD_R1_READY)
        {
            SD_CS_HIGH();

            SD_SPI_TransferByte(0xFF);

            return SD_ERROR;
        }


        /*
         * Read OCR
         */
        ocr[0] =
            SD_SPI_TransferByte(0xFF);

        ocr[1] =
            SD_SPI_TransferByte(0xFF);

        ocr[2] =
            SD_SPI_TransferByte(0xFF);

        ocr[3] =
            SD_SPI_TransferByte(0xFF);


        /*
         * CCS bit
         *
         * Bit 30
         */
        if(ocr[0] & 0x40)
        {
            SD_CardType =
                SD_TYPE_SDHC;
        }
        else
        {
            SD_CardType =
                SD_TYPE_SDSC;
        }
    }


    /*
     * ========================================================
     * SD VERSION 1
     * ========================================================
     */
    else if(response == SD_R1_ILLEGAL)
    {
        timeout = 10000;

        while(timeout--)
        {
            /*
             * CMD55
             */
            response = SD_SendCommand(
                CMD55,
                0x00000000,
                0x01
            );


            if(
                response != SD_R1_IDLE &&
                response != SD_R1_READY
            )
            {
                SD_CS_HIGH();

                SD_SPI_TransferByte(0xFF);

                return SD_ERROR;
            }


            /*
             * ACMD41
             */
            response = SD_SendCommand(
                ACMD41,
                0x00000000,
                0x01
            );


            if(response == SD_R1_READY)
            {
                break;
            }


            if(response != SD_R1_IDLE)
            {
                SD_CS_HIGH();

                SD_SPI_TransferByte(0xFF);

                return SD_ERROR;
            }


            Delay_Ms(1);
        }


        if(response != SD_R1_READY)
        {
            SD_CS_HIGH();

            SD_SPI_TransferByte(0xFF);

            return SD_TIMEOUT;
        }


        SD_CardType =
            SD_TYPE_SDSC;
    }


    /*
     * ========================================================
     * INVALID CMD8 RESPONSE
     * ========================================================
     */
    else
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_ERROR;
    }


    /*
     * --------------------------------------------------------
     * SDSC:
     * Set block size to 512 bytes
     * --------------------------------------------------------
     */
    if(
        SD_CardType ==
        SD_TYPE_SDSC
    )
    {
        response = SD_SendCommand(
            CMD16,
            512,
            0xFF
        );


        if(response != SD_R1_READY)
        {
            SD_CS_HIGH();

            SD_SPI_TransferByte(0xFF);

            return SD_ERROR;
        }
    }


    /*
     * --------------------------------------------------------
     * Deselect
     * --------------------------------------------------------
     */
    SD_CS_HIGH();

    SD_SPI_TransferByte(0xFF);


    return SD_OK;
}


/*
 * ============================================================
 * GET CARD TYPE
 * ============================================================
 */
uint8_t SD_GetCardType(void)
{
    return SD_CardType;
}


/*
 * ============================================================
 * READ BLOCK
 * ============================================================
 */
SD_Status SD_ReadBlock(
    uint32_t block,
    uint8_t *buffer
)
{
    uint8_t response;
    uint16_t i;
    uint32_t timeout;


    /*
     * SDSC uses byte addressing
     */
    if(
        SD_CardType ==
        SD_TYPE_SDSC
    )
    {
        block *= 512;
    }


    /*
     * Select
     */
    SD_CS_LOW();


    /*
     * CMD17
     */
    response = SD_SendCommand(
        CMD17,
        block,
        0xFF
    );


    if(response != SD_R1_READY)
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_ERROR;
    }


    /*
     * Wait for data token
     */
    timeout = 1000000;

    while(timeout--)
    {
        response =
            SD_SPI_TransferByte(0xFF);

        if(response == 0xFE)
        {
            break;
        }
    }


    if(timeout == 0)
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_TIMEOUT;
    }


    /*
     * Read 512 bytes
     */
    for(i = 0; i < 512; i++)
    {
        buffer[i] =
            SD_SPI_TransferByte(0xFF);
    }


    /*
     * CRC
     */
    SD_SPI_TransferByte(0xFF);
    SD_SPI_TransferByte(0xFF);


    /*
     * Deselect
     */
    SD_CS_HIGH();

    SD_SPI_TransferByte(0xFF);


    return SD_OK;
}


/*
 * ============================================================
 * WRITE BLOCK
 * ============================================================
 */
SD_Status SD_WriteBlock(
    uint32_t block,
    const uint8_t *buffer
)
{
    uint8_t response;
    uint16_t i;
    uint32_t timeout;


    /*
     * SDSC uses byte addressing
     */
    if(
        SD_CardType ==
        SD_TYPE_SDSC
    )
    {
        block *= 512;
    }


    /*
     * Select
     */
    SD_CS_LOW();


    /*
     * CMD24
     */
    response = SD_SendCommand(
        CMD24,
        block,
        0xFF
    );


    if(response != SD_R1_READY)
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_ERROR;
    }


    /*
     * Start token
     */
    SD_SPI_TransferByte(0xFE);


    /*
     * Send 512 bytes
     */
    for(i = 0; i < 512; i++)
    {
        SD_SPI_TransferByte(
            buffer[i]
        );
    }


    /*
     * Dummy CRC
     */
    SD_SPI_TransferByte(0xFF);
    SD_SPI_TransferByte(0xFF);


    /*
     * Data response
     */
    response =
        SD_SPI_TransferByte(0xFF);


    if(
        (response & 0x1F) != 0x05
    )
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_ERROR;
    }


    /*
     * Wait for card to finish
     */
    timeout = 1000000;

    while(timeout--)
    {
        if(
            SD_SPI_TransferByte(0xFF)
            == 0xFF
        )
        {
            break;
        }
    }


    if(timeout == 0)
    {
        SD_CS_HIGH();

        SD_SPI_TransferByte(0xFF);

        return SD_TIMEOUT;
    }


    /*
     * Deselect
     */
    SD_CS_HIGH();

    SD_SPI_TransferByte(0xFF);


    return SD_OK;
}