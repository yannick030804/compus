#include <xc.h>
#include "TAD_ADC.h"

static unsigned char busy;

void ADC_Init(void)
{
    ADCON2 = 0x35;
    ADCON0 = 0x01;
    busy = 0;
}

unsigned char ADC_Start(unsigned char channel)
{
    if (busy && ADCON0bits.GO) return 0;
    busy = 0;
    ADCON0 = (unsigned char)((ADCON0 & 0xC3) | (channel << 2));
    ADCON0bits.GO = 1;
    busy = 1;
    return 1;
}

unsigned char ADC_IsDone(void)
{
    return (unsigned char)(!ADCON0bits.GO);
}

unsigned char ADC_Read(void)
{
    busy = 0;
    return ADRESH;
}
