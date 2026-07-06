#include <xc.h>
#include "TAD_LCD.h"
#include "TAD_TIMER.h"

#define LCD_RS_UP()   LATDbits.LATD6 = 1
#define LCD_RS_DOWN() LATDbits.LATD6 = 0
#define LCD_E_UP()    LATDbits.LATD1 = 1
#define LCD_E_DOWN()  LATDbits.LATD1 = 0

static char frame[32];
static unsigned char cursor;
static unsigned char scan;
static unsigned char timerHandle;

static void sendByte(unsigned char value, unsigned char data)
{
    if (data) LCD_RS_UP(); else LCD_RS_DOWN();
    LCD_E_UP();
    LATD = (unsigned char)((LATD & 0xC3) | ((value >> 2) & 0x3C));
    LCD_E_DOWN();
    LCD_E_UP();
    LATD = (unsigned char)((LATD & 0xC3) | ((value << 2) & 0x3C));
    LCD_E_DOWN();
}

static void firstCommand(unsigned char value)
{
    LCD_RS_DOWN();
    LCD_E_UP();
    LATDbits.LATD2 = value & 1;
    LATDbits.LATD3 = (value >> 1) & 1;
    LCD_E_DOWN();
}

static void waitMs(unsigned char ms)
{
    TI_ResetTics(timerHandle);
    while (TI_GetTics(timerHandle) < ms) {
    }
}

void LCD_Init(void)
{
    unsigned char i;

    CONFIG_LCD;
    TI_NewTimer(&timerHandle);
    LCD_RS_DOWN();
    LCD_E_DOWN();
    for (i = 0; i < 32; i++) frame[i] = ' ';
    cursor = 0;
    scan = 0;

    waitMs(100);
    firstCommand(3);
    waitMs(15);
    firstCommand(3);
    waitMs(5);
    firstCommand(3);
    waitMs(5);
    firstCommand(2);
    waitMs(1);
    sendByte(0x28, 0);
    waitMs(2);
    sendByte(0x08, 0);
    waitMs(2);
    sendByte(0x01, 0);
    waitMs(3);
    sendByte(0x06, 0);
    waitMs(2);
    sendByte(0x0C, 0);
}

void LCD_GotoXY(unsigned char column, unsigned char row)
{
    cursor = (unsigned char)(column + (row ? 16 : 0));
}

void LCD_PutChar(char c)
{
    if (cursor < 32) frame[cursor++] = c;
}

void motorLCD(void)
{
    static unsigned char state = 0;

    if (TI_GetTics(timerHandle) < 1) return;
    TI_ResetTics(timerHandle);

    if (state == 0) {
        sendByte(0x80, 0);
        scan = 0;
        state = 1;
    } else if (state == 1) {
        sendByte(frame[scan++], 1);
        if (scan == 16) state = 2;
    } else if (state == 2) {
        sendByte(0xC0, 0);
        state = 3;
    } else {
        sendByte(frame[scan++], 1);
        if (scan == 32) state = 0;
    }
}
