#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_LDR.h"

#define LDR_DARK_LIMIT 80

unsigned char ldrCovered;

void LDR_Init(void)
{
    ldrCovered = 0;
}

void motorLDR(void)
{
    ldrCovered = (unsigned char)(ADC_GetLight() < LDR_DARK_LIMIT);
}
