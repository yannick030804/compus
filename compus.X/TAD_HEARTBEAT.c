#include <xc.h>
#include "TAD_HEARTBEAT.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static unsigned char rebellion;
static unsigned char pwm;
static unsigned char duty;
static unsigned char up;

void Heartbeat_Init(void)
{
    CONFIG_HEARTBEAT;
    TI_NewTimer(&timerHandle);
    rebellion = 0;
    pwm = 0;
    duty = 1;
    up = 1;
    HEARTBEAT_LED = 0;
}

void Heartbeat_SetRebellion(unsigned char active)
{
    rebellion = active;
}

void motorHeartbeat(void)
{
    if (rebellion) {
        HEARTBEAT_LED = 0;
        return;
    }
    if (TI_GetTics(timerHandle) < 5) return;
    TI_ResetTics(timerHandle);

    pwm++;
    if (pwm >= 20) {
        pwm = 0;
        if (up) {
            duty++;
            if (duty >= 18) up = 0;
        } else {
            duty--;
            if (duty <= 1) up = 1;
        }
    }
    HEARTBEAT_LED = (unsigned char)(pwm < duty);
}
