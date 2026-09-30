/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v20x_it.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/06/06
 * Description        : This file contains the headers of the interrupt handlers.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __CH32V20x_IT_H
#define __CH32V20x_IT_H

#include "debug.h"

enum {
    FAULT_STAGE_NONE = 0,
    FAULT_STAGE_MP3_FILE = 1,
    FAULT_STAGE_MP3_READ = 2,
    FAULT_STAGE_MP3_SYNC = 3,
    FAULT_STAGE_MP3_DECODE = 4,
    FAULT_STAGE_AUDIO_INIT = 5,
    FAULT_STAGE_PCM_PREPARE = 6,
    FAULT_STAGE_DMA_OUTPUT = 7,
    FAULT_STAGE_DEC_HEADER = 8,
    FAULT_STAGE_DEC_SIDEINFO = 9,
    FAULT_STAGE_DEC_MAINDATA = 10,
    FAULT_STAGE_DEC_SCALEFACT = 11,
    FAULT_STAGE_DEC_HUFFMAN = 12,
    FAULT_STAGE_DEC_DEQUANT = 13,
    FAULT_STAGE_DEC_IMDCT = 14,
    FAULT_STAGE_DEC_SUBBAND = 15
};

void FaultDiag_SetStage(uint8_t stage);
uint8_t FaultDiag_TakeHardFaultStage(void);

#endif /* __CH32V20x_IT_H */
