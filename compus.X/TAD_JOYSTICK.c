#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_JOYSTICK.h"

#define JOY_LOW  86
#define JOY_HIGH 170
#define JOY_LAST_MASK    0x0F
#define JOY_PENDING_MASK 0xF0
#define JOY_GET_LAST() ((unsigned char)(joyPacked & JOY_LAST_MASK))
#define JOY_SET_LAST(value) (joyPacked = (unsigned char)((joyPacked & JOY_PENDING_MASK) | ((value) & JOY_LAST_MASK)))
#define JOY_GET_PENDING() ((unsigned char)((joyPacked >> 4) & JOY_LAST_MASK))
#define JOY_SET_PENDING(value) (joyPacked = (unsigned char)((joyPacked & JOY_LAST_MASK) | (((value) & JOY_LAST_MASK) << 4)))

static unsigned char adcX;
static unsigned char adcY;
static unsigned char joyPacked;

void Joystick_Init(void)
{
    CONFIG_JOYSTICK;
    adcX = 128;
    adcY = 128;
    joyPacked = 0;
}

void motorJoystick(void)
{
    static unsigned char state = 0;
    unsigned char newDirection;

    switch (state) {
        case 0:
            if (ADC_Start(JOYSTICK_X_CHANNEL)) state++;
            break;
        case 1:
            if (ADC_IsDone()) {
                adcX = ADC_Read();
                state++;
            }
            break;
        case 2:
            if (ADC_Start(JOYSTICK_Y_CHANNEL)) state++;
            break;
        case 3:
            if (ADC_IsDone()) {
                adcY = ADC_Read();
                state++;
            }
            break;
        default:
            if (JOY_GET_PENDING() == JOY_NONE) {
                newDirection = JOY_NONE;
                if (adcX > JOY_HIGH) newDirection = JOY_RIGHT;
                else if (adcX < JOY_LOW) newDirection = JOY_LEFT;
                else if (adcY > JOY_HIGH) newDirection = JOY_DOWN;
                else if (adcY < JOY_LOW) newDirection = JOY_UP;

                if (newDirection != JOY_GET_LAST()) {
                    JOY_SET_LAST(newDirection);
                    if (newDirection != JOY_NONE) JOY_SET_PENDING(newDirection);
                }
            }
            state = 0;
            break;
    }
}

unsigned char Joystick_GetEvent(void)
{
    unsigned char e = JOY_GET_PENDING();
    JOY_SET_PENDING(JOY_NONE);
    return e;
}
