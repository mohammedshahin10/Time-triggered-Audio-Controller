#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdint.h>

/* Persistent config store in the last 2 KB of internal flash: eight
 * 256-byte fast pages written round-robin (wear spread x8, newest record
 * = highest sequence). Linker FLASH region is shrunk to 62 KB to keep
 * these pages free. Layout follows esp_bell_idf mydef.h; 240 bytes
 * limits rotating-group ids to 1..94 (MEM_GROUP_AT + id*2 < 240). */

#define EEPROM_SIZE 240

uint8_t  EEPROM_Begin(void);
uint8_t  EEPROM_Read8(uint16_t addr);
uint16_t EEPROM_Read16(uint16_t addr);
uint8_t  EEPROM_ReadBool(uint16_t bit);
void     EEPROM_Write8(uint16_t addr, uint8_t v);
void     EEPROM_Write16(uint16_t addr, uint16_t v);
void     EEPROM_WriteBool(uint16_t bit, uint8_t v);
void     EEPROM_Commit(void);

#endif
