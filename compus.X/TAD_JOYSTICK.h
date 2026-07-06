#ifndef TAD_JOYSTICK_H
#define TAD_JOYSTICK_H

#define JOY_NONE  0
#define JOY_UP    1
#define JOY_DOWN  2
#define JOY_LEFT  3
#define JOY_RIGHT 4

#define CONFIG_JOYSTICK TRISAbits.TRISA0 = 1; TRISAbits.TRISA1 = 1
#define JOYSTICK_X_CHANNEL 0
#define JOYSTICK_Y_CHANNEL 1

void Joystick_Init(void);
void motorJoystick(void);
unsigned char Joystick_GetEvent(void);

#endif
