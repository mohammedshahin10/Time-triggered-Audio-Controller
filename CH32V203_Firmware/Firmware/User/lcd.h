#ifndef __LCD_H
#define __LCD_H

#include "ch32v20x.h"

/*
 * ============================================================
 * CH32V203G8R6 PCB
 * 16x2 LCD - 4 BIT MODE
 * ============================================================
 *
 * LCD D4 -> PB10  (pin 18)
 * LCD D5 -> PB11  (pin 19)
 * LCD D6 -> PB12 (pin 20)
 * LCD RS -> PB13 (pin 21)
 * LCD EN -> PB14 (pin 22)
 * LCD D7 -> PB8  (pin 24)
 *
 * LCD RW -> GND
 *
 * ============================================================
 */

void LCD_Init(void);

void LCD_Clear(void);

void LCD_SetCursor(
    uint8_t row,
    uint8_t col
);

void LCD_WriteChar(char c);

void LCD_WriteString(
    const char *str
);

void LCD_WriteCommand(
    uint8_t command
);

void LCD_CreateChar(
    uint8_t location,
    const uint8_t charmap[8]
);

void LCD_CursorOn(void);
void LCD_CursorOff(void);
void LCD_BlinkOn(void);
void LCD_BlinkOff(void);
void LCD_PrintNum(uint16_t value);
void LCD_PutsAt(uint8_t row, uint8_t col, const char *str);

#endif
