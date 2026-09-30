#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status of Disk Functions */
typedef BYTE DSTATUS;

/* Results of Disk Functions */
typedef enum
{
    RES_OK = 0,     /* 0: Successful */
    RES_ERROR,      /* 1: R/W Error */
    RES_WRPRT,      /* 2: Write Protected */
    RES_NOTRDY,     /* 3: Not Ready */
    RES_PARERR      /* 4: Invalid Parameter */
} DRESULT;


/* Disk Status Bits */
#define STA_NOINIT      0x01    /* Drive not initialized */
#define STA_NODISK      0x02    /* No medium in the drive */
#define STA_PROTECT     0x04    /* Write protected */


/* Command code for disk_ioctrl() */

#define CTRL_SYNC           0
#define GET_SECTOR_COUNT    1
#define GET_SECTOR_SIZE     2
#define GET_BLOCK_SIZE      3
#define CTRL_TRIM           4

#define CTRL_POWER          5
#define CTRL_LOCK           6
#define CTRL_UNLOCK         7
#define CTRL_EJECT          8
#define CTRL_FORMAT         9

#define MMC_GET_TYPE        10
#define MMC_GET_CSD         11
#define MMC_GET_CID         12
#define MMC_GET_OCR         13
#define MMC_GET_SDSTAT      14

#define ATA_GET_REV         20
#define ATA_GET_MODEL       21
#define ATA_GET_SN          22


/* Physical drive number */
#define DEV_SD              0


/* Disk functions */

DSTATUS disk_initialize(BYTE pdrv);

DSTATUS disk_status(BYTE pdrv);

DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count
);

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count
);

#endif

DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
);

#ifdef __cplusplus
}
#endif

#endif /* _DISKIO_DEFINED */