#include "sd_spi.h"

/*
 * ============================================================
 * SD CARD SPI DRIVER
 * CH32V203G8R6
 * ============================================================
 *
 * SPI1:
 *
 * PA5 -> SCK
 * PA6 -> MISO
 * PA7 -> MOSI
 * PA4 -> CS
 *
 * SPI Mode 0
 *
 * CPOL = 0
 * CPHA = 0
 *
 * ============================================================
 */


void SD_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;


    /*
     * --------------------------------------------------------
     * Enable GPIOA and SPI1 clocks
     * --------------------------------------------------------
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_SPI1,
        ENABLE
    );


    /*
     * --------------------------------------------------------
     * PA5 -> SCK
     * PA7 -> MOSI
     *
     * Alternate function push-pull
     * --------------------------------------------------------
     */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_5 |
        GPIO_Pin_7;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /*
     * --------------------------------------------------------
     * PA6 -> MISO
     * --------------------------------------------------------
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA,&GPIO_InitStructure);


    /*
     * --------------------------------------------------------
     * PA4 -> SD CS
     *
     * Manual GPIO control
     * --------------------------------------------------------
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_Init(
        GPIOA,
        &GPIO_InitStructure
    );


    /*
     * --------------------------------------------------------
     * Deselect card
     * --------------------------------------------------------
     */
    SD_CS_HIGH();


    /*
     * --------------------------------------------------------
     * SPI configuration
     * --------------------------------------------------------
     */
    SPI_InitStructure.SPI_Direction =
        SPI_Direction_2Lines_FullDuplex;

    SPI_InitStructure.SPI_Mode =
        SPI_Mode_Master;

    SPI_InitStructure.SPI_DataSize =
        SPI_DataSize_8b;


    /*
     * SPI MODE 0
     */
    SPI_InitStructure.SPI_CPOL =
        SPI_CPOL_Low;

    SPI_InitStructure.SPI_CPHA =
        SPI_CPHA_1Edge;


    /*
     * Software NSS
     */
    SPI_InitStructure.SPI_NSS =
        SPI_NSS_Soft;


    /*
     * Slow SPI speed during initialization.
     */
    SPI_InitStructure.SPI_BaudRatePrescaler =
        SPI_BaudRatePrescaler_256;


    /*
     * MSB first
     */
    SPI_InitStructure.SPI_FirstBit =
        SPI_FirstBit_MSB;


    /*
     * CRC
     */
    SPI_InitStructure.SPI_CRCPolynomial =
        7;


    /*
     * Initialize SPI1
     */
    SPI_Init(
        SPI1,
        &SPI_InitStructure
    );


    /*
     * Enable SPI1
     */
    SPI_Cmd(
        SPI1,
        ENABLE
    );


    /*
     * --------------------------------------------------------
     * SD startup clocks
     *
     * 10 bytes �� 8 clocks = 80 clocks
     *
     * CS remains HIGH.
     * --------------------------------------------------------
     */
    {
        uint8_t i;

        SD_CS_HIGH();

        for(i = 0; i < 10; i++)
        {
            SD_SPI_TransferByte(0xFF);
        }
    }


    SD_CS_HIGH();
}


/*
 * ============================================================
 * SPI TRANSFER BYTE
 * ============================================================
 */
uint8_t SD_SPI_TransferByte(uint8_t data)
{
    uint32_t timeout;


    /*
     * Wait for TX empty
     */
    timeout = 60000;

    while(
        SPI_I2S_GetFlagStatus(
            SPI1,
            SPI_I2S_FLAG_TXE
        ) == RESET
    )
    {
        timeout--;

        if(timeout == 0)
        {
            return 0xFF;
        }
    }


    /*
     * Send byte
     */
    SPI_I2S_SendData(
        SPI1,
        data
    );


    /*
     * Wait for RX
     */
    timeout = 60000;

    while(
        SPI_I2S_GetFlagStatus(
            SPI1,
            SPI_I2S_FLAG_RXNE
        ) == RESET
    )
    {
        timeout--;

        if(timeout == 0)
        {
            return 0xFF;
        }
    }


    /*
     * Return received byte
     */
    return (
        uint8_t
    )SPI_I2S_ReceiveData(SPI1);
}