#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_FARM.h"
#include "TAD_LCD.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define MESSAGE_TIME 3000

static unsigned char timerHandle;
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

static void putSpeciesName(unsigned char species)
{
    if (species == FARM_COW) putText("VACA");
    else if (species == FARM_HORSE) putText("CAVALL");
    else if (species == FARM_PIG) putText("PORC");
    else putText("GALLINA");
}

static void putProductName(unsigned char species)
{
    if (species == FARM_COW) putText("LLET");
    else if (species == FARM_HORSE) putText("PINZELL");
    else if (species == FARM_PIG) putText("PERNIL");
    else putText("OUS");
}

static void showNotification(const FarmNotification *n)
{
    startLine(0);
    if (n->kind == FARM_NOTIF_ANIMAL) putSpeciesName(n->species);
    else putProductName(n->species);
    putChar(' ');
    put2(n->number);
    fillLine();
    startLine(1);
    fillLine();
}

static void showIdle(void)
{
    if (SerialTime_IsConfigured() == 0) {
        writeLine(0, "H");
    } else if (Farm_IsConfigured() == 0) {
        writeLine(0, "I");
    } else {
        writeLine(0, Farm_GetName());
        startLine(1);
        put2(SerialTime_GetDay());
        putChar('/');
        put2(SerialTime_GetMonth());
        putChar(' ');
        put2(SerialTime_GetHour());
        putChar(':');
        put2(SerialTime_GetMinute());
        fillLine();
        return;
    }
    startLine(1);
    fillLine();
}

void Display_Init(void)
{
    TI_NewTimer(&timerHandle);
}

void motorDisplay(void)
{
    static unsigned char state = 0;
    FarmNotification n;

    if (state == 0) {
        if (Farm_GetNotification(&n)) {
            showNotification(&n);
            TI_ResetTics(timerHandle);
            state++;
        } else {
            showIdle();
        }
    } else if (TI_GetTics(timerHandle) >= MESSAGE_TIME) {
        state = 0;
    }
}
