#include "storage.h"
#include <string.h>

static FATFS fs;
static uint8_t mounted;

uint8_t Storage_Mount(void)
{
    if (mounted)
        return 1;
    if (f_mount(&fs, "", 1) != FR_OK)
        return 0;
    mounted = 1;
    return 1;
}

void Storage_Unmount(void)
{
    if (!mounted)
        return;
    f_mount(0, "", 0);
    mounted = 0;
}

uint8_t Storage_FileExists(const char *path)
{
    FILINFO fi;
    return f_stat(path, &fi) == FR_OK && !(fi.fattrib & AM_DIR);
}

uint8_t Storage_FolderExists(const char *path)
{
    FILINFO fi;
    return f_stat(path, &fi) == FR_OK && (fi.fattrib & AM_DIR);
}

static char Storage_ToLower(char value)
{
    if (value >= 'A' && value <= 'Z')
        return (char)(value + ('a' - 'A'));
    return value;
}

static uint8_t Storage_HasExtension(const char *name, const char *extension)
{
    uint32_t nameLength;
    uint32_t extensionLength;
    uint32_t index;

    if (!name || !extension)
        return 0;
    nameLength = (uint32_t)strlen(name);
    extensionLength = (uint32_t)strlen(extension);
    if (extensionLength == 0U || nameLength <= extensionLength)
        return 0;
    for (index = 0; index < extensionLength; index++) {
        if (Storage_ToLower(name[nameLength - extensionLength + index]) !=
            Storage_ToLower(extension[index]))
            return 0;
    }
    return 1;
}

Storage_FindResult Storage_FindFirstFileByExtension(const char *folder,
    const char *extension, char *path, uint32_t pathSize)
{
    DIR directory;
    FILINFO entry;
    FRESULT result;
    char best[13];
    uint32_t folderLength;
    uint32_t nameLength;

    if (!folder || !extension || !path || pathSize == 0U)
        return STORAGE_FIND_IO_ERROR;
    best[0] = '\0';
    result = f_opendir(&directory, folder);
    if (result != FR_OK)
        return STORAGE_FIND_IO_ERROR;

    for (;;) {
        result = f_readdir(&directory, &entry);
        if (result != FR_OK) {
            f_closedir(&directory);
            return STORAGE_FIND_IO_ERROR;
        }
        if (entry.fname[0] == '\0')
            break;
        if ((entry.fattrib & AM_DIR) ||
            !Storage_HasExtension(entry.fname, extension))
            continue;
        if (best[0] == '\0' || strcmp(entry.fname, best) < 0) {
            strncpy(best, entry.fname, sizeof(best) - 1U);
            best[sizeof(best) - 1U] = '\0';
        }
    }
    f_closedir(&directory);
    if (best[0] == '\0')
        return STORAGE_FIND_NOT_FOUND;

    folderLength = (uint32_t)strlen(folder);
    nameLength = (uint32_t)strlen(best);
    if (folderLength + 1U + nameLength + 1U > pathSize)
        return STORAGE_FIND_PATH_TOO_LONG;
    memcpy(path, folder, folderLength);
    path[folderLength] = '/';
    memcpy(path + folderLength + 1U, best, nameLength + 1U);
    return STORAGE_FIND_OK;
}

uint32_t Storage_ReadLine(FIL *f, char *buf, uint32_t size)
{
    UINT n;
    char c;
    uint32_t i = 0;
    uint8_t got = 0;

    while (i < size - 1) {
        if (f_read(f, &c, 1, &n) != FR_OK || n == 0)
            break;
        got = 1;
        if (c == '\n')
            break;
        if (c != '\r')
            buf[i++] = c;
    }
    buf[i] = 0;
    return i ? i : (uint32_t)got;
}
