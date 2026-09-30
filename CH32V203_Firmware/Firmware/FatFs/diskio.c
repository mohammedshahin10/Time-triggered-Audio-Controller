#include "diskio.h"
#include "sd.h"
#include "sd_spi.h"
/*
 * Physical drive 0 = SD card
 */

static DSTATUS Stat = STA_NOINIT;


/*
 * Initialize SD card
 */
DSTATUS disk_initialize(BYTE pdrv)
{
    if(pdrv != 0)
    {
        return STA_NOINIT;
    }
    SD_SPI_Init();
    
    if(SD_Init() == SD_OK)
    {
        Stat = 0;
    }
    else
    {
        Stat = STA_NOINIT;
    }

    return Stat;
}


/*
 * Return SD card status
 */
DSTATUS disk_status(BYTE pdrv)
{
    if(pdrv != 0)
    {
        return STA_NOINIT;
    }

    return Stat;
}


/*
 * Read sectors from SD card
 *
 * FatFs sector number == SD block number for SDHC.
 */
DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count)
{
    UINT i;

    if(pdrv != 0)
    {
        return RES_PARERR;
    }

    if(Stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    for(i = 0; i < count; i++)
    {
        if(SD_ReadBlock(
                (uint32_t)(sector + i),
                buff + (i * 512)) != SD_OK)
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}


/*
 * Write support
 *
 * Currently disabled because:
 *
 * #define FF_FS_READONLY 1
 *
 */
#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count)
{
    UINT i;

    if(pdrv != 0)
    {
        return RES_PARERR;
    }

    if(Stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    for(i = 0; i < count; i++)
    {
        if(SD_WriteBlock(
                (uint32_t)(sector + i),
                buff + (i * 512)) != SD_OK)
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#endif


/*
 * Control functions
 */
DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff)
{
    if(pdrv != 0)
    {
        return RES_PARERR;
    }

    if(Stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    switch(cmd)
    {
        case CTRL_SYNC:
            /*
             * SD_ReadBlock() waits for the card response,
             * so there is nothing else to synchronize.
             */
            return RES_OK;


        case GET_SECTOR_SIZE:
            /*
             * SD card sector size = 512 bytes
             */
            *(WORD *)buff = 512;
            return RES_OK;


        case GET_BLOCK_SIZE:
            /*
             * Return 1 sector as the minimum block size.
             */
            *(DWORD *)buff = 1;
            return RES_OK;


        case GET_SECTOR_COUNT:
            /*
             * Not required for our initial f_mount()
             * + f_open() + f_read() test.
             *
             * We will add proper CSD capacity detection
             * later.
             */
            return RES_PARERR;


        default:
            return RES_PARERR;
    }
}