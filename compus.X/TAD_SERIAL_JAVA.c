#include <xc.h>
#include "TAD_SERIAL_JAVA.h"

static char rxLine[32];
static unsigned char rxLen;
static unsigned char txIndex;

void SerialJava_Init(void)
{
    CONFIG_SERIAL_JAVA;
    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 64;
    BAUDCON = 0x00;
    rxLen = 0;
    txIndex = 0;
}

const char *SJ_GetLine(void)
{
    unsigned char c;

    if (RCSTAbits.OERR) {
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }
    if (PIR1bits.RCIF == 0) return 0;
    c = RCREG;
    if (RCSTAbits.FERR) return 0;

    if (c == '\r' || c == '\n') {
        if (rxLen == 0) return 0;
        rxLine[rxLen] = '\0';
        rxLen = 0;
        return rxLine;
    }
    if (rxLen < sizeof(rxLine) - 1) {
        rxLine[rxLen++] = (char)c;
    } else {
        rxLen = 0;
    }
    return 0;
}

unsigned char SJ_PutString(const char *str)
{
    if (str[txIndex] == '\0') {
        txIndex = 0;
        return 1;
    }
    if (PIR1bits.TXIF) {
        TXREG = str[txIndex++];
    }
    return 0;
}
