#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_FARM.h"
#include "TAD_LDR.h"
#include "TAD_TIMER.h"

#define LDR_ADC_CHANNEL 3
#define LDR_CHANGE_THRESHOLD 20
#define LDR_TIMEOUT_MS 5000

static unsigned char timerHandle;
static unsigned char baseLight;

void LDR_Init(void)
{
    TI_NewTimer(&timerHandle);
}

void motorLDR(void)
{
    static unsigned char state = 0;

    if (Farm_IsRestRequestPending() == 0) {
        state = 0;
        return;
    }
    if (state == 0) {
        TI_ResetTics(timerHandle);
        state = 1;
    }
    if (TI_GetTics(timerHandle) >= LDR_TIMEOUT_MS) {
        Farm_NotifyRestTimeout();
        state = 0;
    } else if (state == 1) {
        if (ADC_Start(LDR_ADC_CHANNEL)) state = 2;
    } else if (state == 2 && ADC_IsDone()) {
        baseLight = ADC_Read();
        state = 3;
    } else if (state == 3) {
        if (ADC_Start(LDR_ADC_CHANNEL)) state = 4;
    } else if (state == 4 && ADC_IsDone()) {
        unsigned char light = ADC_Read();
        if (light > baseLight) light = (unsigned char)(light - baseLight);
        else light = (unsigned char)(baseLight - light);
        if (light > LDR_CHANGE_THRESHOLD) {
            Farm_NotifyRestSuccess();
            state = 0;
        } else {
            state = 3;
        }
    }
}
