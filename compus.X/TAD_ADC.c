#include <xc.h>
#include "TAD_ADC.h"

static unsigned char adcX = 128;
static unsigned char adcY = 128;
static unsigned char adcLight = 255;
static unsigned char channelIndex;

static unsigned char getChannel(void)
{
    if (channelIndex == 0) return 0;
    if (channelIndex == 1) return 1;
    return 3;
}

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
            ADCON0 = (unsigned char)((ADCON0 & 0xC3) | (getChannel() << 2));
            ADCON0bits.GO = 1;
            state = 1;
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

unsigned char ADC_GetX(void) { return adcX; }
unsigned char ADC_GetY(void) { return adcY; }
unsigned char ADC_GetLight(void) { return adcLight; }
