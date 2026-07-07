#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_LCD.h"
#include "TAD_LDR.h"

static unsigned char column;

static void startLine(unsigned char row)
{
    LCD_GotoXY(0, row);
    column = 0;
}

static void putChar(char c)
{
    if (column < 16) {
        LCD_PutChar(c);
        column++;
    }
}

static void putText(const char *text)
{
    while (column < 16 && *text != '\0') putChar(*text++);
}

static void fillLine(void)
{
    while (column < 16) putChar(' ');
}

static void writeLine(unsigned char row, const char *text)
{
    startLine(row);
    putText(text);
    fillLine();
}

static void put2(unsigned char value)
{
    unsigned char tens = 0;
    while (value >= 10) {
        value = (unsigned char)(value - 10);
        tens++;
    }
    putChar((char)('0' + tens));
    putChar((char)('0' + value));
}

static void put3(unsigned char value)
{
    unsigned char hundreds = 0;

    if (value >= 100) {
        hundreds++;
        value = (unsigned char)(value - 100);
        if (value >= 100) {
            hundreds++;
            value = (unsigned char)(value - 100);
        }
    }
    putChar((char)('0' + hundreds));
    put2(value);
}

static void showIdle(void)
{
    writeLine(0, "LDR");
    startLine(1);
    put3(LDR_GetValue());
    fillLine();
}

void Display_Init(void)
{
}

void motorDisplay(void)
{
    showIdle();
}
