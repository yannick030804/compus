#ifndef TAD_LDR_H
#define TAD_LDR_H

#define LDR_Init()
void motorLDR(void);

extern unsigned char ldrCovered;
#define LDR_IsCovered() (ldrCovered)

#endif
