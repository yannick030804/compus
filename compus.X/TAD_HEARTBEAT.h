#ifndef TAD_HEARTBEAT_H
#define TAD_HEARTBEAT_H

#define CONFIG_HEARTBEAT TRISAbits.TRISA4 = 0
#define HEARTBEAT_LED LATAbits.LATA4

extern unsigned char heartbeatRebellion;
#define Heartbeat_SetRebellion(active) (heartbeatRebellion = (active))

void Heartbeat_Init(void);
void motorHeartbeat(void);

#endif
