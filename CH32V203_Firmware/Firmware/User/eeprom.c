#include "eeprom.h"
#include "ch32v20x.h"
#include <string.h>

/* 8 fast pages (256B each) in the last 2 KB of flash, round-robin:
 * each commit erases the next page and writes one full record into it,
 * newest = highest sequence number. FLASH_ErasePage would take 4 KB,
 * so fast page erase/program is used instead. */
#define STORE_BASE   0x0800F800U
#define PAGE_BYTES   256U
#define SLOT_COUNT   8U
#define REC_MAGIC    0xBEE7U

typedef struct {
    uint16_t magic;
    uint16_t seq;
    uint16_t sum;
    uint16_t pad;
    uint8_t data[EEPROM_SIZE];
} Record;   /* 248 bytes, fits one fast page */

static union {
    Record rec;
    uint32_t words[PAGE_BYTES / 4];
} page;

#define buffer (page.rec.data)

static uint16_t seq;
static uint32_t liveSlot;

static uint32_t SlotAddr(uint32_t i)
{
    return STORE_BASE + i * PAGE_BYTES;
}

static uint16_t Checksum(const uint8_t *d)
{
    uint16_t s = 0;
    uint16_t i;
    for (i = 0; i < EEPROM_SIZE; i++)
        s += d[i];
    return s;
}

uint8_t EEPROM_Begin(void)
{
    int32_t bestSeq = -1;
    uint32_t i;

    memset(buffer, 0xFF, EEPROM_SIZE);
    seq = 0;
    liveSlot = SLOT_COUNT - 1;   /* first commit lands in slot 0 */

    for (i = 0; i < SLOT_COUNT; i++) {
        const Record *r = (const Record *)SlotAddr(i);
        if (r->magic != REC_MAGIC || r->sum != Checksum(r->data))
            continue;
        if ((int32_t)r->seq > bestSeq) {
            bestSeq = r->seq;
            liveSlot = i;
        }
    }
    if (bestSeq >= 0) {
        memcpy(buffer, ((const Record *)SlotAddr(liveSlot))->data, EEPROM_SIZE);
        seq = (uint16_t)bestSeq;
    }
    return 1;
}

void EEPROM_Commit(void)
{
    uint32_t slot = (liveSlot + 1) % SLOT_COUNT;
    uint32_t i;

    seq++;
    page.rec.magic = REC_MAGIC;
    page.rec.seq = seq;
    page.rec.sum = Checksum(buffer);
    page.rec.pad = 0;
    for (i = sizeof(Record); i < PAGE_BYTES; i++)
        ((uint8_t *)&page)[i] = 0xFF;

    FLASH_Unlock();
    FLASH_Unlock_Fast();
    FLASH_ErasePage_Fast(SlotAddr(slot));
    FLASH_ProgramPage_Fast(SlotAddr(slot), page.words);
    FLASH_Lock_Fast();
    FLASH_Lock();

    liveSlot = slot;
}

uint8_t EEPROM_Read8(uint16_t addr)
{
    return (addr < EEPROM_SIZE) ? buffer[addr] : 0xFF;
}

uint16_t EEPROM_Read16(uint16_t addr)
{
    if (addr + 1 >= EEPROM_SIZE)
        return 0xFFFF;
    return (uint16_t)(buffer[addr] | (buffer[addr + 1] << 8));
}

uint8_t EEPROM_ReadBool(uint16_t bit)
{
    if (bit > 100)
        return 0;
    return (buffer[bit >> 3] >> (bit & 7)) & 1;
}

void EEPROM_Write8(uint16_t addr, uint8_t v)
{
    if (addr >= EEPROM_SIZE)
        return;
    buffer[addr] = v;
    EEPROM_Commit();
}

void EEPROM_Write16(uint16_t addr, uint16_t v)
{
    if (addr + 1 >= EEPROM_SIZE)
        return;
    buffer[addr] = (uint8_t)v;
    buffer[addr + 1] = (uint8_t)(v >> 8);
    EEPROM_Commit();
}

void EEPROM_WriteBool(uint16_t bit, uint8_t v)
{
    if (bit > 100)
        return;
    if (v)
        buffer[bit >> 3] |= 1 << (bit & 7);
    else
        buffer[bit >> 3] &= ~(1 << (bit & 7));
    EEPROM_Commit();
}
