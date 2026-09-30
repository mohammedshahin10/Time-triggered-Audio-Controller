#ifndef RELAY_LIGHT_H
#define RELAY_LIGHT_H

#include <stdint.h>

/* RLY_IN (amplifier relay) = PA3, LGT_IN (light output) = PB0.
 * See README.md / REQUIREMENTS.md pin tables. This module owns the GPIO;
 * the application tasks own amplifier state and scheduled light timing. */

void RelayLight_Init(void);
void Relay_Set(uint8_t on);
void Light_Set(uint8_t on);

#endif /* RELAY_LIGHT_H */
