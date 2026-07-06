#include <xc.h>
#include "TAD_EEPROM.h"

static unsigned char busy;

static void startWrite(void)
{
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;
    di();
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    ei();
}

void motorEEPROM(void)
{
    if (busy == 0) return;
    if (EECON1bits.WR == 1) return;
    if (busy == 1) {
        startWrite();
        busy = 2;
    } else {
        EECON1bits.WREN = 0;
        busy = 0;
    }
}

unsigned char EEPROM_ReadByte(unsigned char addr)
{
    EEADR = addr;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

unsigned char EEPROM_StartByteWrite(unsigned char addr, unsigned char data)
{
    if (busy != 0) return 0;
    EEADR = addr;
    EEDATA = data;
    busy = 1;
    return 1;
}

unsigned char EEPROM_IsBusy(void)
{
    return (unsigned char)(busy != 0);
}
