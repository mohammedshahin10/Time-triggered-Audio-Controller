#ifndef __TEXT_UTIL_H
#define __TEXT_UTIL_H

#include <stdint.h>

typedef struct {
    char *write;
    char *last;
} TextBuilder;

void Text_Init(TextBuilder *b, char *buffer, uint32_t size);
void Text_AppendChar(TextBuilder *b, char c);
void Text_Append(TextBuilder *b, const char *text);
void Text_AppendN(TextBuilder *b, const char *text, uint32_t count);
void Text_AppendUInt(TextBuilder *b, uint32_t value, uint8_t min_width);

int Text_ParseInt(const char *text);
float Text_ParseFloat(const char *text);
int Text_ParseIntList(const char *text, int *values, int count,
                      const char *separators);

#endif
