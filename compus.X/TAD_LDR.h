#ifndef TAD_LDR_H
#define TAD_LDR_H

void LDR_Init(void);
void motorLDR(void);

extern unsigned char ldrCovered;
#define LDR_IsCovered() (ldrCovered)

#endif
