#include <xc.h>
#include "TAD_ADC.h"
#include "TAD_FARM.h"
#include "TAD_LDR.h"
#include "TAD_TIMER.h"

#define LDR_ADC_CHANNEL 3
#define LDR_COVER_THRESHOLD 100
#define LDR_TIMEOUT_MS 5000

static unsigned char timerHandle;
static unsigned char ldrCovered;

void LDR_Init(void)
{
    TI_NewTimer(&timerHandle);
    ldrCovered = 0;
}

void motorLDR(void)
{
    static unsigned char state = 0;

    switch (state) {
        case 0:
            if (Farm_IsRestRequestPending()) {
                TI_ResetTics(timerHandle);
                state++;
            }
            break;
        case 1:
            if (Farm_IsRestRequestPending() == 0) {
                state = 0;
            } else if (ADC_Start(LDR_ADC_CHANNEL)) {
                state++;
            }
            break;
        default:
            if (Farm_IsRestRequestPending() == 0) {
                state = 0;
            } else if (ADC_IsDone()) {
                ldrCovered = (unsigned char)(ADC_Read() < LDR_COVER_THRESHOLD);
                if (ldrCovered) {
                    Farm_NotifyRestSuccess();
                    state = 0;
                } else if (TI_GetTics(timerHandle) >= LDR_TIMEOUT_MS) {
                    Farm_NotifyRestTimeout();
                    state = 0;
                } else {
                    state = 1;
                }
            }
            break;
    }
}

unsigned char LDR_IsCovered(void)
{
    return ldrCovered;
}
