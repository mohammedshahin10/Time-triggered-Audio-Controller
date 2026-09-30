#include "sd_text_viewer.h"
#include "ff.h"
#include "keypad.h"
#include "lcd.h"
#include "storage.h"

#define LCD_COLUMNS   16U
#define LCD_PAGE_SIZE (LCD_COLUMNS * 2U)

static void LCD_WriteFixedLine(uint8_t row, const char *text)
{
    uint8_t column = 0;

    LCD_SetCursor(row, 0);
    while (column < LCD_COLUMNS && text[column] != '\0')
        LCD_WriteChar(text[column++]);
    while (column++ < LCD_COLUMNS)
        LCD_WriteChar(' ');
}

static void DisplayMessage(const char *line1, const char *line2)
{
    LCD_WriteFixedLine(0, line1);
    LCD_WriteFixedLine(1, line2);
}

static void WaitForMenu(void)
{
    while (KEY_GetKey() != KEY_MENU)
        Delay_Ms(20);
}

static FRESULT ReadDisplayPage(FIL *file, char *page, FSIZE_t offset,
                               FSIZE_t *nextOffset, uint8_t *hasData)
{
    FRESULT result;
    UINT bytesRead;
    uint8_t cell;
    uint8_t value;

    for (cell = 0; cell < LCD_PAGE_SIZE; cell++)
        page[cell] = ' ';
    cell = 0;
    *hasData = 0;

    result = f_lseek(file, offset);
    if (result != FR_OK)
        return result;

    while (cell < LCD_PAGE_SIZE) {
        result = f_read(file, &value, 1, &bytesRead);
        if (result != FR_OK)
            return result;
        if (bytesRead == 0)
            break;

        *hasData = 1;
        if (value == '\r')
            continue;
        if (value == '\n') {
            cell = (uint8_t)(((cell / LCD_COLUMNS) + 1U) * LCD_COLUMNS);
            continue;
        }
        if (value == '\t')
            value = ' ';
        else if (value < 0x20U || value > 0x7eU)
            value = '?';

        page[cell++] = (char)value;
    }

    *nextOffset = f_tell(file);
    return FR_OK;
}

static void DisplayCurrentPage(const char *page)
{
    uint8_t column;

    LCD_SetCursor(0, 0);
    for (column = 0; column < LCD_COLUMNS; column++)
        LCD_WriteChar(page[column]);

    LCD_SetCursor(1, 0);
    for (column = LCD_COLUMNS; column < LCD_PAGE_SIZE; column++)
        LCD_WriteChar(page[column]);
}

static FRESULT FindPreviousPage(FIL *file, char *page, FSIZE_t currentOffset,
                                FSIZE_t *previousOffset)
{
    FRESULT result;
    FSIZE_t scanOffset = 0;
    FSIZE_t nextOffset;
    FSIZE_t lastOffset = 0;
    uint8_t hasData;

    while (scanOffset < currentOffset) {
        lastOffset = scanOffset;
        result = ReadDisplayPage(file, page, scanOffset, &nextOffset, &hasData);
        if (result != FR_OK)
            return result;
        if (!hasData || nextOffset <= scanOffset || nextOffset >= currentOffset)
            break;
        scanOffset = nextOffset;
    }

    *previousOffset = lastOffset;
    return FR_OK;
}

SD_TextViewerStatus SD_TextViewer_Run(const char *path)
{
    FIL file;
    char page[LCD_PAGE_SIZE];
    FRESULT result;
    FSIZE_t currentOffset = 0;
    FSIZE_t nextOffset = 0;
    FSIZE_t previousOffset;
    uint8_t hasData;
    uint8_t key;

    if (!Storage_Mount()) {
        DisplayMessage("SD MOUNT ERROR", "MENU TO EXIT");
        WaitForMenu();
        return SD_TEXT_VIEWER_MOUNT_ERROR;
    }

    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        if (result == FR_NO_FILE || result == FR_NO_PATH) {
            DisplayMessage("FILE MISSING", "MENU TO EXIT");
            WaitForMenu();
            return SD_TEXT_VIEWER_FILE_NOT_FOUND;
        }
        DisplayMessage("FILE OPEN ERROR", "MENU TO EXIT");
        WaitForMenu();
        return SD_TEXT_VIEWER_READ_ERROR;
    }

    while (1) {
        result = ReadDisplayPage(&file, page, currentOffset, &nextOffset,
                                 &hasData);
        if (result != FR_OK) {
            DisplayMessage("FILE READ ERROR", "MENU TO EXIT");
            WaitForMenu();
            f_close(&file);
            return SD_TEXT_VIEWER_READ_ERROR;
        }
        if (!hasData && currentOffset == 0) {
            DisplayMessage("FILE EMPTY", "MENU TO EXIT");
            WaitForMenu();
            f_close(&file);
            return SD_TEXT_VIEWER_OK;
        }

        DisplayCurrentPage(page);
        do {
            key = KEY_GetKey();
            Delay_Ms(20);
        } while (key == KEY_NONE);

        if (key == KEY_MENU)
            break;
        if (key == KEY_UP && nextOffset < f_size(&file)) {
            currentOffset = nextOffset;
        } else if (key == KEY_DOWN && currentOffset > 0) {
            result = FindPreviousPage(&file, page, currentOffset,
                                      &previousOffset);
            if (result != FR_OK) {
                DisplayMessage("FILE READ ERROR", "MENU TO EXIT");
                WaitForMenu();
                f_close(&file);
                return SD_TEXT_VIEWER_READ_ERROR;
            }
            currentOffset = previousOffset;
        }
    }

    f_close(&file);
    return SD_TEXT_VIEWER_OK;
}
