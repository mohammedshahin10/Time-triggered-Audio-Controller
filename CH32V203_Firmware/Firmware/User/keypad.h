#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "ch32v20x.h"

/*
 * ============================================================
 * CH32V203G8R6 CUSTOM-PCB KEYPAD
 * ============================================================
 *
 * MENU -> PA0
 * UP   -> PA1
 * DOWN -> PA2
 *
 * Buttons are connected between GPIO and GND.
 * Internal pull-ups are enabled.
 *
 * Released = HIGH
 * Pressed  = LOW
 *
 * ============================================================
 */

/* Key definitions - same logic as original keys.hpp */
#define KEY_NONE    0
#define KEY_MENU    1
#define KEY_UP      2
#define KEY_DOWN    4


/*
 * Initialize keypad GPIOs.
 */
void KEY_Init(void);


/*
 * Get a newly pressed key.
 *
 * Returns:
 *
 * KEY_NONE
 * KEY_MENU
 * KEY_UP
 * KEY_DOWN
 *
 * Same edge-detection behavior as original getKey().
 */
uint8_t KEY_GetKey(void);
uint8_t KEY_PollEvent(void);
void KEY_QueueEvent(uint8_t key);


/*
 * Wait for a key press.
 *
 * timeout = milliseconds
 *
 * Returns KEY_NONE if timeout expires.
 */
uint8_t KEY_WaitKey(uint32_t timeout);

#endif
