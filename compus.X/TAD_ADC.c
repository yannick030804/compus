#include <xc.h>
#include "TAD_ADC.h"

unsigned char adcX = 128;
unsigned char adcY = 128;
unsigned char adcLight = 255;
static unsigned char channelIndex;

void ADC_Init(void)
{
    ADCON2 = 0x35;
    ADCON0 = 0x01;
    channelIndex = 0;
}

void motorADC(void)
{
    static unsigned char state = 0;
    unsigned char value;

    if (state == 0) {
        if (ADCON0bits.GO == 0) {
            value = channelIndex;
            if (value == 2) value = 3;
            ADCON0 = (unsigned char)((ADCON0 & 0xC3) | (value << 2));
            ADCON0bits.GO = 1;
            state++;
        }
    } else if (ADCON0bits.GO == 0) {
        value = ADRESH;
        if (channelIndex == 0) adcX = value;
        else if (channelIndex == 1) adcY = value;
        else adcLight = value;
        channelIndex++;
        if (channelIndex >= 3) channelIndex = 0;
        state = 0;
    }
}
