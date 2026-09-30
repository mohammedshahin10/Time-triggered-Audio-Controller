#include "text_util.h"

static uint8_t is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\v' || c == '\f';
}

static uint8_t is_digit(char c)
{
    return c >= '0' && c <= '9';
}

void Text_Init(TextBuilder *b, char *buffer, uint32_t size)
{
    b->write = buffer;
    b->last = size ? buffer + size - 1 : buffer;
    if (size)
        *buffer = 0;
}

void Text_AppendChar(TextBuilder *b, char c)
{
    if (b->write < b->last) {
        *b->write++ = c;
        *b->write = 0;
    }
}

void Text_Append(TextBuilder *b, const char *text)
{
    while (*text)
        Text_AppendChar(b, *text++);
}

void Text_AppendN(TextBuilder *b, const char *text, uint32_t count)
{
    while (count-- && *text)
        Text_AppendChar(b, *text++);
}

void Text_AppendUInt(TextBuilder *b, uint32_t value, uint8_t min_width)
{
    char digits[10];
    uint8_t count = 0;

    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value);
    while (count < min_width)
        digits[count++] = '0';
    while (count)
        Text_AppendChar(b, digits[--count]);
}

static const char *parse_signed(const char *p, int *value, uint8_t *valid)
{
    uint32_t result = 0;
    int sign = 1;

    while (is_space(*p))
        p++;
    if (*p == '+' || *p == '-') {
        if (*p++ == '-')
            sign = -1;
    }
    *valid = is_digit(*p);
    while (is_digit(*p)) {
        result = result * 10U + (uint8_t)(*p - '0');
        p++;
    }
    *value = sign * (int)result;
    return p;
}

int Text_ParseInt(const char *text)
{
    int value;
    uint8_t valid;
    parse_signed(text, &value, &valid);
    return valid ? value : 0;
}

float Text_ParseFloat(const char *text)
{
    const char *p = text;
    float value = 0.0f;
    float fraction = 0.1f;
    int exponent = 0;
    int sign = 1;
    int exponent_sign = 1;
    uint8_t digits = 0;

    while (is_space(*p))
        p++;
    if (*p == '+' || *p == '-') {
        if (*p++ == '-')
            sign = -1;
    }
    while (is_digit(*p)) {
        value = value * 10.0f + (float)(*p++ - '0');
        digits = 1;
    }
    if (*p == '.') {
        p++;
        while (is_digit(*p)) {
            value += (float)(*p++ - '0') * fraction;
            fraction *= 0.1f;
            digits = 1;
        }
    }
    if (!digits)
        return 0.0f;
    if (*p == 'e' || *p == 'E') {
        const char *e = p + 1;
        if (*e == '+' || *e == '-') {
            if (*e++ == '-')
                exponent_sign = -1;
        }
        if (is_digit(*e)) {
            p = e;
            while (is_digit(*p)) {
                if (exponent < 10000)
                    exponent = exponent * 10 + (*p - '0');
                p++;
            }
            exponent *= exponent_sign;
        }
    }
    if (exponent) {
        float scale = 10.0f;
        uint32_t power = (uint32_t)(exponent < 0 ? -exponent : exponent);
        float factor = 1.0f;
        while (power) {
            if (power & 1U)
                factor *= scale;
            scale *= scale;
            power >>= 1;
        }
        value = exponent < 0 ? value / factor : value * factor;
    }
    return sign < 0 ? -value : value;
}

static uint8_t is_separator(char c, const char *separators)
{
    while (*separators)
        if (c == *separators++)
            return 1;
    return 0;
}

int Text_ParseIntList(const char *text, int *values, int count,
                      const char *separators)
{
    int parsed = 0;
    while (parsed < count) {
        uint8_t valid;
        text = parse_signed(text, &values[parsed], &valid);
        if (!valid)
            break;
        parsed++;
        if (parsed == count || !is_separator(*text, separators))
            break;
        text++;
    }
    return parsed;
}
