#include <xc.h>
#include "TAD_TIMER.h"

#define T0CON_CONFIG 0x88
#define TMR0_RELOAD  63036
#define NUM_TIMERS   6

static volatile unsigned int ticks;
static unsigned int starts[NUM_TIMERS];
static unsigned char nextTimer;

void RSI_Timer0(void)
{
    TMR0 = TMR0_RELOAD;
    INTCONbits.TMR0IF = 0;
    ticks++;
}

void TI_Init(void)
{
    T0CON = T0CON_CONFIG;
    TMR0 = TMR0_RELOAD;
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

unsigned char TI_NewTimer(unsigned char *timerHandle)
{
    *timerHandle = nextTimer;
    starts[nextTimer] = ticks;
    nextTimer++;
    return TI_TRUE;
}

void TI_ResetTics(unsigned char timerHandle)
{
    di();
    starts[timerHandle] = ticks;
    ei();
}

unsigned int TI_GetTics(unsigned char timerHandle)
{
    unsigned int now;

    di();
    now = ticks;
    ei();
    return (unsigned int)(now - starts[timerHandle]);
}
