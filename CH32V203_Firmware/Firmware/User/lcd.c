#include "lcd.h"
#include "debug.h"

/* LCD pin mapping */
#define LCD_RS_PORT    GPIOB
#define LCD_RS_PIN     GPIO_Pin_13

#define LCD_EN_PORT    GPIOB
#define LCD_EN_PIN     GPIO_Pin_14

#define LCD_D4_PORT    GPIOB
#define LCD_D4_PIN     GPIO_Pin_10

#define LCD_D5_PORT    GPIOB
#define LCD_D5_PIN     GPIO_Pin_11

#define LCD_D6_PORT    GPIOB
#define LCD_D6_PIN     GPIO_Pin_12

#define LCD_D7_PORT    GPIOB
#define LCD_D7_PIN     GPIO_Pin_8


static void LCD_RS(uint8_t state)
{
    if(state)
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);
    else
        GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN);
}


static void LCD_EnablePulse(void)
{
    GPIO_SetBits(LCD_EN_PORT, LCD_EN_PIN);

    Delay_Us(2);

    GPIO_ResetBits(LCD_EN_PORT, LCD_EN_PIN);

    Delay_Us(2);
}


static void LCD_Send4Bits(uint8_t data)
{
    /* D4 -> PB10 */
    if(data & 0x01)
        GPIO_SetBits(LCD_D4_PORT, LCD_D4_PIN);
    else
        GPIO_ResetBits(LCD_D4_PORT, LCD_D4_PIN);

    /* D5 -> PB11 */
    if(data & 0x02)
        GPIO_SetBits(LCD_D5_PORT, LCD_D5_PIN);
    else
        GPIO_ResetBits(LCD_D5_PORT, LCD_D5_PIN);

    /* D6 -> PB12 */
    if(data & 0x04)
        GPIO_SetBits(LCD_D6_PORT, LCD_D6_PIN);
    else
        GPIO_ResetBits(LCD_D6_PORT, LCD_D6_PIN);

    /* D7 -> PB8 */
    if(data & 0x08)
        GPIO_SetBits(LCD_D7_PORT, LCD_D7_PIN);
    else
        GPIO_ResetBits(LCD_D7_PORT, LCD_D7_PIN);

    LCD_EnablePulse();
}


void LCD_WriteCommand(uint8_t command)
{
    LCD_RS(0);

    LCD_Send4Bits(command >> 4);
    LCD_Send4Bits(command & 0x0F);

    if(command == 0x01 || command == 0x02)
        Delay_Ms(2);
    else
        Delay_Us(50);
}


void LCD_WriteChar(char c)
{
    LCD_RS(1);

    LCD_Send4Bits(((uint8_t)c) >> 4);
    LCD_Send4Bits(((uint8_t)c) & 0x0F);

    Delay_Us(50);
}


void LCD_WriteString(const char *str)
{
    while(*str)
    {
        LCD_WriteChar(*str);
        str++;
    }
}


void LCD_Clear(void)
{
    LCD_WriteCommand(0x01);
    Delay_Ms(2);
}


void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if(row == 0)
        address = 0x00 + col;
    else
        address = 0x40 + col;

    LCD_WriteCommand(0x80 | address);
}


void LCD_CreateChar(uint8_t location, const uint8_t charmap[8])
{
    uint8_t i;

    location &= 0x07U;
    LCD_WriteCommand((uint8_t)(0x40U | (location << 3)));
    for(i = 0U; i < 8U; i++)
        LCD_WriteChar((char)charmap[i]);
    LCD_WriteCommand(0x80U);
}


void LCD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOB clock */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOB,
        ENABLE
    );

    /*
     * LCD:
     *
     * PB8  -> D7
     * PB10 -> D4
     * PB11 -> D5
     * PB12 -> D6
     * PB13 -> RS
     * PB14 -> EN
     */
    GPIO_InitStructure.GPIO_Pin =
          GPIO_Pin_8
        | GPIO_Pin_10
        | GPIO_Pin_11
        | GPIO_Pin_12
        | GPIO_Pin_13
        | GPIO_Pin_14;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* All LCD outputs LOW */
    GPIO_ResetBits(
        GPIOB,
          GPIO_Pin_8
        | GPIO_Pin_10
        | GPIO_Pin_11
        | GPIO_Pin_12
        | GPIO_Pin_13
        | GPIO_Pin_14
    );

    /* LCD power-up delay */
    Delay_Ms(20);

    /* HD44780 4-bit initialization */

    LCD_RS(0);

    LCD_Send4Bits(0x03);
    Delay_Ms(5);

    LCD_Send4Bits(0x03);
    Delay_Us(150);

    LCD_Send4Bits(0x03);
    Delay_Us(150);

    LCD_Send4Bits(0x02);
    Delay_Us(150);

    /* 4-bit, 2-line, 5x8 font */
    LCD_WriteCommand(0x28);

    /* Display ON, cursor OFF, blink OFF */
    LCD_WriteCommand(0x0C);

    /* Entry mode */
    LCD_WriteCommand(0x06);

    /* Clear */
    LCD_WriteCommand(0x01);
    Delay_Ms(2);

    LCD_SetCursor(0, 0);
}

/* --- extensions for the bell application (menu/scheduling UI) --- */

static uint8_t dispCtrl = 0x0C;   /* display on, cursor off, blink off */

void LCD_CursorOn(void)  { dispCtrl |= 0x02;  LCD_WriteCommand(dispCtrl); }
void LCD_CursorOff(void) { dispCtrl &= ~0x02; LCD_WriteCommand(dispCtrl); }
void LCD_BlinkOn(void)   { dispCtrl |= 0x01;  LCD_WriteCommand(dispCtrl); }
void LCD_BlinkOff(void)  { dispCtrl &= ~0x01; LCD_WriteCommand(dispCtrl); }

void LCD_PrintNum(uint16_t value)
{
    char buf[6];
    uint8_t i = sizeof(buf) - 1;
    buf[i] = 0;
    if (value == 0)
        buf[--i] = '0';
    else
        while (value && i) { buf[--i] = '0' + value % 10; value /= 10; }
    LCD_WriteString(&buf[i]);
}

/* Clears from col to end of row, then writes str at col. */
void LCD_PutsAt(uint8_t row, uint8_t col, const char *str)
{
    uint8_t n;
    LCD_SetCursor(row, col);
    for (n = col; n < 16; n++)
        LCD_WriteChar(' ');
    LCD_SetCursor(row, col);
    LCD_WriteString(str);
}
