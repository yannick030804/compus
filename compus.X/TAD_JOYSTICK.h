#ifndef TAD_JOYSTICK_H
#define TAD_JOYSTICK_H

#define JOY_NONE  0
#define JOY_UP    1
#define JOY_DOWN  2
#define JOY_LEFT  3
#define JOY_RIGHT 4

void Joystick_Init(void);
void motorJoystick(void);
unsigned char Joystick_GetEvent(void);

#endif
