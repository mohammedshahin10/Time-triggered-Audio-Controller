#include "keypad.h"
#include "happybell_build.h"
#include "debug.h"
#if HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_FULL
#include "app_runtime.h"
#include "player.h"
#endif


/*
 * ============================================================
 * Key pin definitions
 * ============================================================
 */

#define KEY_MENU_PORT   GPIOA
#define KEY_MENU_PIN    GPIO_Pin_0

#define KEY_UP_PORT     GPIOA
#define KEY_UP_PIN      GPIO_Pin_1

#define KEY_DOWN_PORT   GPIOA
#define KEY_DOWN_PIN    GPIO_Pin_2


/*
 * Last detected key.
 *
 * Original ESP32 code initialized this to -1.
 *
 * We use 0xFF here because uint8_t is used.
 */
static uint8_t lastKey = 0xFF;
static uint8_t queuedKey;


/*
 * ============================================================
 * KEY_Init
 * ============================================================
 *
 * PA0 = MENU
 * PA1 = UP
 * PA2 = DOWN
 *
 * Input pull-up.
 * ============================================================
 */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * Enable GPIOA clock.
     */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA,
        ENABLE
    );

    /*
     * Configure PA0, PA1 and PA2
     * as input pull-up.
     */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0 |
        GPIO_Pin_1 |
        GPIO_Pin_2;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * Same initial state as original:
     *
     * _lastKey = -1;
     */
    lastKey = 0xFF;
    queuedKey = KEY_NONE;
}


/*
 * ============================================================
 * KEY_GetKey
 * ============================================================
 *
 * Same logic as original Keys::getKey().
 *
 * Button priority is:
 *
 * MENU checked first
 * UP   checked second
 * DOWN checked last
 *
 * Therefore if multiple buttons are pressed simultaneously,
 * DOWN has the highest priority, then UP, then MENU.
 *
 * Holding a button does NOT repeatedly return the key.
 * A new key event occurs only when the key state changes.
 * ============================================================
 */
uint8_t KEY_PollEvent(void)
{
    uint8_t currentKey = KEY_NONE;


    /*
     * --------------------------------------------------------
     * MENU
     * --------------------------------------------------------
     */
    if(GPIO_ReadInputDataBit(
            KEY_MENU_PORT,
            KEY_MENU_PIN) == RESET)
    {
        currentKey = KEY_MENU;
    }


    /*
     * --------------------------------------------------------
     * UP
     * --------------------------------------------------------
     */
    if(GPIO_ReadInputDataBit(
            KEY_UP_PORT,
            KEY_UP_PIN) == RESET)
    {
        currentKey = KEY_UP;
    }


    /*
     * --------------------------------------------------------
     * DOWN
     * --------------------------------------------------------
     */
    if(GPIO_ReadInputDataBit(
            KEY_DOWN_PORT,
            KEY_DOWN_PIN) == RESET)
    {
        currentKey = KEY_DOWN;
    }


    /*
     * --------------------------------------------------------
     * Same key as last time?
     *
     * If yes, don't generate another event.
     *
     * This is the same behavior as:
     *
     * if(currentKey == _lastKey)
     *     return KEY_NONE;
     * --------------------------------------------------------
     */
    if(currentKey == lastKey)
    {
        return KEY_NONE;
    }


    /*
     * Save current state.
     */
    lastKey = currentKey;


    /*
     * Return newly detected key.
     */
    return currentKey;
}

void KEY_QueueEvent(uint8_t key)
{
    if (queuedKey == KEY_NONE)
        queuedKey = key;
}

uint8_t KEY_GetKey(void)
{
    uint8_t key;

    if (queuedKey != KEY_NONE) {
        key = queuedKey;
        queuedKey = KEY_NONE;
        return key;
    }
    return KEY_PollEvent();
}


/*
 * ============================================================
 * KEY_WaitKey
 * ============================================================
 *
 * Wait for a key press with timeout.
 *
 * Original behavior:
 *
 * while(!(key = getKey()))
 * {
 *     cooperative delay(100 ms);
 *
 *     if(timeout expired)
 *         return KEY_NONE;
 * }
 *
 * Polls every 100 ms while the cooperative services continue to run.
 * ============================================================
 */
uint8_t KEY_WaitKey(uint32_t timeout)
{
    uint32_t elapsed = 0;
    uint8_t key;


    while(1)
    {
        /*
         * Check keypad.
         */
        key = KEY_GetKey();


        /*
         * A key event occurred.
         */
        if(key != KEY_NONE)
        {
            return key;
        }


#if HAPPYBELL_BUILD_PROFILE == HAPPYBELL_PROFILE_FULL
        if (Player_IsPlaying())
            Player_SetWait(KEY_MENU | KEY_UP | KEY_DOWN, 0, 100U);
        App_DelayMs(100);
#else
        Delay_Ms(100);
#endif


        /*
         * Add 100 ms.
         */
        elapsed += 100;


        /*
         * Check timeout.
         */
        if(elapsed >= timeout)
        {
            return KEY_NONE;
        }
    }
}
