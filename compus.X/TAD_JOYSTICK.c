#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_JOYSTICK.h"

#define JOY_LOW  70
#define JOY_HIGH 185

static unsigned char event;
static unsigned char armed;

void Joystick_Init(void)
{
    event = 0;
    armed = 1;
}

void motorJoystick(void)
{
    unsigned char x = ADC_GetX();
    unsigned char y = ADC_GetY();
    unsigned char dir = JOY_NONE;

    if (x < JOY_LOW) dir = JOY_LEFT;
    else if (x > JOY_HIGH) dir = JOY_RIGHT;
    else if (y < JOY_LOW) dir = JOY_DOWN;
    else if (y > JOY_HIGH) dir = JOY_UP;

    if (dir == JOY_NONE) {
        armed = 1;
    } else if (armed == 1) {
        event = dir;
        armed = 0;
    }
}

unsigned char Joystick_GetEvent(void)
{
    unsigned char e = event;
    event = 0;
    return e;
}
