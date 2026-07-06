#ifndef TAD_HEARTBEAT_H
#define TAD_HEARTBEAT_H

#define CONFIG_HEARTBEAT TRISAbits.TRISA4 = 0
#define HEARTBEAT_LED LATAbits.LATA4

void Heartbeat_Init(void);
void motorHeartbeat(void);
void Heartbeat_SetRebellion(unsigned char active);

#endif
