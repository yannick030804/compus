#ifndef TAD_LDR_H
#define TAD_LDR_H

extern unsigned char ldrValue;
#define LDR_GetValue() (ldrValue)

void LDR_Init(void);
void motorLDR(void);

#endif
