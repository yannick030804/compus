#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static unsigned char event;

void Button_Init(void)
{
    CONFIG_BUTTON;
    TI_NewTimer(&timerHandle);
    event = 0;
}

void motorButton(void)
{
    static unsigned char state = 0;

    if (state == 0) {
        if (BUTTON_PRESSED) {
            TI_ResetTics(timerHandle);
            state++;
        }
    } else if (state == 1) {
        if (TI_GetTics(timerHandle) >= 30) {
            if (BUTTON_PRESSED) {
                event = 1;
                state++;
            } else {
                state = 0;
            }
        }
    } else if (!BUTTON_PRESSED) {
        state = 0;
    }
}

unsigned char getButton(void)
{
    if (event == 0) return 0;
    event = 0;
    return 1;
}
