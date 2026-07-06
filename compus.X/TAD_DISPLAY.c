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

static void putNum(unsigned char value)
{
    unsigned char d = 0;
    unsigned char started = 0;

    if (value >= 100) {
        while (value >= 100) {
            value = (unsigned char)(value - 100);
            d++;
        }
        putChar((char)('0' + d));
        started = 1;
        d = 0;
    }
    if (value >= 10 || started) {
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            d++;
        }
        putChar((char)('0' + d));
    }
    putChar((char)('0' + value));
}

static void showNotification(const FarmNotification *n)
{
    startLine(0);
    putText(n->kind == FARM_NOTIF_ANIMAL ? "Nou Animal" : "Nou Producte");
    fillLine();
    startLine(1);
    if (n->species == FARM_COW) {
        putText(n->kind ? "Llet" : "Vaca");
    } else if (n->species == FARM_PIG) {
        putText(n->kind ? "Pernil" : "Porc");
    } else if (n->species == FARM_HORSE) {
        putText(n->kind ? "Pinzells" : "Cavall");
    } else {
        putText(n->kind ? "Ous" : "Gallina");
    }
    putChar(':');
    putChar(' ');
    putNum(n->number);
    fillLine();
}

static void showIdle(void)
{
    if (Farm_IsConfigured() == 0) {
        startLine(0);
        putText("Esperando Init");
        fillLine();
        startLine(1);
        fillLine();
    } else if (SerialTime_IsConfigured() == 0) {
        startLine(0);
        putText(Farm_GetName());
        fillLine();
        startLine(1);
        putText("Falta Hora");
        fillLine();
    } else {
        startLine(0);
        putText(Farm_GetName());
        fillLine();
        startLine(1);
        put2(SerialTime_GetDay());
        putChar('/');
        put2(SerialTime_GetMonth());
        putText("/2026");
        fillLine();
    }
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
