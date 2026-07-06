#ifndef TAD_SERIAL_JAVA_H
#define TAD_SERIAL_JAVA_H

#define CONFIG_SERIAL_JAVA TRISCbits.TRISC6 = 0; TRISCbits.TRISC7 = 1

void SerialJava_Init(void);
const char *SJ_GetLine(void);
unsigned char SJ_PutString(const char *str);

#endif
