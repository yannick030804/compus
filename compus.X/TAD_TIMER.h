#ifndef TAD_TIMER_H
#define TAD_TIMER_H

#define TI_TRUE  1
#define TI_FALSE 0

void TI_Init(void);
void RSI_Timer0(void);
unsigned char TI_NewTimer(unsigned char *timerHandle);
void TI_ResetTics(unsigned char timerHandle);
unsigned int TI_GetTics(unsigned char timerHandle);

#endif
