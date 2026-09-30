/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v20x_it.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/29
 * Description        : Main Interrupt Service Routines.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ch32v20x_it.h"

void NMI_Handler(void)
  __attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible));
void HardFault_Handler(void)
  __attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible));

#define HARDFAULT_DIAG_MAGIC 0x48464C54UL /* "HFLT" */

typedef struct {
  uint32_t magic;
  uint8_t activeStage;
  uint8_t faultStage;
  uint16_t reserved;
} HardFaultDiagRecord;

static volatile HardFaultDiagRecord hardFaultDiag
  __attribute__((section(".noinit"), aligned(4)));

void FaultDiag_SetStage(uint8_t stage)
{
  hardFaultDiag.activeStage = stage;
}

uint8_t FaultDiag_TakeHardFaultStage(void)
{
  uint8_t stage = FAULT_STAGE_NONE;

  if (hardFaultDiag.magic == HARDFAULT_DIAG_MAGIC)
    stage = hardFaultDiag.faultStage;
  hardFaultDiag.magic = 0;
  hardFaultDiag.activeStage = FAULT_STAGE_NONE;
  hardFaultDiag.faultStage = FAULT_STAGE_NONE;
  return stage;
}

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
  hardFaultDiag.faultStage = hardFaultDiag.activeStage;
  hardFaultDiag.magic = HARDFAULT_DIAG_MAGIC;
  NVIC_SystemReset();
  while (1)
  {
  }
}
