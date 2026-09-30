#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>
#include "ff.h"

uint8_t Storage_Mount(void);
void Storage_Unmount(void);
uint8_t Storage_FileExists(const char *path);
uint8_t Storage_FolderExists(const char *path);

typedef enum {
    STORAGE_FIND_OK = 0,
    STORAGE_FIND_NOT_FOUND,
    STORAGE_FIND_IO_ERROR,
    STORAGE_FIND_PATH_TOO_LONG
} Storage_FindResult;

/* Finds the alphabetically first matching short filename in one folder. */
Storage_FindResult Storage_FindFirstFileByExtension(const char *folder,
    const char *extension, char *path, uint32_t pathSize);

/* Reads one line, strips CR/LF. Returns length, 0 on EOF+empty. */
uint32_t Storage_ReadLine(FIL *f, char *buf, uint32_t size);

#endif
