#ifndef TAD_ADC_H
#define TAD_ADC_H

void ADC_Init(void);
void motorADC(void);

extern unsigned char adcX;
extern unsigned char adcY;
extern unsigned char adcLight;

#define ADC_GetX()     (adcX)
#define ADC_GetY()     (adcY)
#define ADC_GetLight() (adcLight)

#endif
