#ifndef TAD_BUTTON_H
#define TAD_BUTTON_H

#define CONFIG_BUTTON TRISBbits.TRISB0 = 1
#define BUTTON_PRESSED (PORTBbits.RB0 == 0)

void Button_Init(void);
void motorButton(void);
unsigned char getButton(void);

#endif
