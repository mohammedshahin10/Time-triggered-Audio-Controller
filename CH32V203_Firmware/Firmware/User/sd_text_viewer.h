#ifndef SD_TEXT_VIEWER_H
#define SD_TEXT_VIEWER_H

#include "ch32v20x.h"

typedef enum
{
    SD_TEXT_VIEWER_OK = 0,
    SD_TEXT_VIEWER_MOUNT_ERROR,
    SD_TEXT_VIEWER_FILE_NOT_FOUND,
    SD_TEXT_VIEWER_READ_ERROR
} SD_TextViewerStatus;

/*
 * Blocking 16x2 startup diagnostic used only by the diagnostic profile.
 *
 * UP   -> next page
 * DOWN -> previous page
 * MENU -> close file and return
 */
SD_TextViewerStatus SD_TextViewer_Run(const char *path);

#endif
