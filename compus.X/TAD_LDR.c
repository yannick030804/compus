#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_LDR.h"

#define LDR_DARK_LIMIT 80

static unsigned char covered;

void LDR_Init(void)
{
    covered = 0;
}

void motorLDR(void)
{
    covered = (unsigned char)(ADC_GetLight() < LDR_DARK_LIMIT);
}

unsigned char LDR_IsCovered(void)
{
    return covered;
}
