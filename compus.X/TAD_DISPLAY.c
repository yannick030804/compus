#include <xc.h>
#include "TAD_DISPLAY.h"
#include "TAD_FARM.h"
#include "TAD_LCD.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define MESSAGE_TIME 3000

static unsigned char timerHandle;
static char line[17];

static void clearLine(void)
{
    unsigned char i;
    for (i = 0; i < 16; i++) line[i] = ' ';
    line[16] = '\0';
}

static void writeLine(unsigned char row, const char *text)
{
    unsigned char i = 0;

    LCD_GotoXY(0, row);
    while (i < 16 && text[i] != '\0') {
        LCD_PutChar(text[i]);
        i++;
    }
    while (i < 16) {
        LCD_PutChar(' ');
        i++;
    }
}

static void put2(unsigned char pos, unsigned char value)
{
    unsigned char tens = 0;
    while (value >= 10) {
        value = (unsigned char)(value - 10);
        tens++;
    }
    line[pos] = (char)('0' + tens);
    line[pos + 1] = (char)('0' + value);
}

static unsigned char putNum(unsigned char pos, unsigned char value)
{
    unsigned char d = 0;
    unsigned char started = 0;

    if (value >= 100) {
        while (value >= 100) {
            value = (unsigned char)(value - 100);
            d++;
        }
        line[pos++] = (char)('0' + d);
        started = 1;
        d = 0;
    }
    if (value >= 10 || started) {
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            d++;
        }
        line[pos++] = (char)('0' + d);
    }
    line[pos++] = (char)('0' + value);
    return pos;
}

static unsigned char putSpecies(unsigned char pos, unsigned char species)
{
    if (species == FARM_COW) {
        line[pos++] = 'V'; line[pos++] = 'a'; line[pos++] = 'c'; line[pos++] = 'a';
    } else if (species == FARM_PIG) {
        line[pos++] = 'P'; line[pos++] = 'o'; line[pos++] = 'r'; line[pos++] = 'c';
    } else if (species == FARM_HORSE) {
        line[pos++] = 'C'; line[pos++] = 'a'; line[pos++] = 'v'; line[pos++] = 'a'; line[pos++] = 'l'; line[pos++] = 'l';
    } else {
        line[pos++] = 'G'; line[pos++] = 'a'; line[pos++] = 'l'; line[pos++] = 'l'; line[pos++] = 'i'; line[pos++] = 'n'; line[pos++] = 'a';
    }
    return pos;
}

static unsigned char putProduct(unsigned char pos, unsigned char species)
{
    if (species == FARM_COW) {
        line[pos++] = 'L'; line[pos++] = 'l'; line[pos++] = 'e'; line[pos++] = 't';
    } else if (species == FARM_PIG) {
        line[pos++] = 'P'; line[pos++] = 'e'; line[pos++] = 'r'; line[pos++] = 'n'; line[pos++] = 'i'; line[pos++] = 'l';
    } else if (species == FARM_HORSE) {
        line[pos++] = 'P'; line[pos++] = 'i'; line[pos++] = 'n'; line[pos++] = 'z'; line[pos++] = 'e'; line[pos++] = 'l'; line[pos++] = 'l'; line[pos++] = 's';
    } else {
        line[pos++] = 'O'; line[pos++] = 'u'; line[pos++] = 's';
    }
    return pos;
}

static void showNotification(const FarmNotification *n)
{
    unsigned char pos;

    if (n->kind == FARM_NOTIF_ANIMAL) writeLine(0, "Nou Animal");
    else writeLine(0, "Nou Producte");

    clearLine();
    if (n->kind == FARM_NOTIF_ANIMAL) pos = putSpecies(0, n->species);
    else pos = putProduct(0, n->species);
    line[pos++] = ':';
    line[pos++] = ' ';
    pos = putNum(pos, n->number);
    line[pos] = '\0';
    writeLine(1, line);
}

static void showIdle(void)
{
    if (Farm_IsConfigured() == 0) {
        writeLine(0, "Esperando Init");
        writeLine(1, "");
    } else if (SerialTime_IsConfigured() == 0) {
        writeLine(0, Farm_GetName());
        writeLine(1, "Falta Hora");
    } else {
        writeLine(0, Farm_GetName());
        clearLine();
        put2(0, SerialTime_GetDay());
        line[2] = '/';
        put2(3, SerialTime_GetMonth());
        line[5] = '/';
        line[6] = '2';
        line[7] = '0';
        line[8] = '2';
        line[9] = '6';
        line[10] = '\0';
        writeLine(1, line);
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
            state = 1;
        } else {
            showIdle();
        }
    } else if (TI_GetTics(timerHandle) >= MESSAGE_TIME) {
        state = 0;
    }
}
