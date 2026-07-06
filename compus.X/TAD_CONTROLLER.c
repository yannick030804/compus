#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_CONTROLLER.h"
#include "TAD_FARM.h"
#include "TAD_HEARTBEAT.h"
#include "TAD_JOYSTICK.h"
#include "TAD_LDR.h"
#include "TAD_SERIAL_JAVA.h"
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

static unsigned char timerHandle;
static const char *txLine;
static char txBuf[24];
static char nameBuf[17];

static unsigned char isDigit(char c)
{
    return (unsigned char)(c >= '0' && c <= '9');
}

static unsigned char appendNum(unsigned char k, unsigned char value)
{
    unsigned char d = 0;
    unsigned char started = 0;

    if (value >= 100) {
        while (value >= 100) {
            value = (unsigned char)(value - 100);
            d++;
        }
        txBuf[k++] = (char)('0' + d);
        started = 1;
        d = 0;
    }
    if (value >= 10 || started) {
        while (value >= 10) {
            value = (unsigned char)(value - 10);
            d++;
        }
        txBuf[k++] = (char)('0' + d);
    }
    txBuf[k++] = (char)('0' + value);
    return k;
}

static void endLine(unsigned char k)
{
    txBuf[k++] = '\r';
    txBuf[k++] = '\n';
    txBuf[k] = '\0';
    txLine = txBuf;
}

static unsigned char parseNum(const char *s, unsigned char *i, unsigned char *value, char end)
{
    unsigned char n = 0;
    unsigned char ok = 0;

    while (isDigit(s[*i])) {
        n = (unsigned char)((n << 3) + (n << 1) + (unsigned char)(s[*i] - '0'));
        (*i)++;
        ok = 1;
    }
    if (ok == 0 || s[*i] != end) return 0;
    *value = n;
    return 1;
}

static unsigned char parseInit(const char *s)
{
    unsigned char i = (s[1] == ':') ? 2 : 1;
    unsigned char n = 0;
    unsigned char cow, horse, pig, chicken;

    while (s[i] != '$' && s[i] != '\0') {
        if (n < 16) nameBuf[n++] = s[i];
        i++;
    }
    if (n == 0 || s[i] != '$') return 0;
    nameBuf[n] = '\0';
    i++;
    if (!parseNum(s, &i, &cow, '$')) return 0;
    i++;
    if (!parseNum(s, &i, &horse, '$')) return 0;
    i++;
    if (!parseNum(s, &i, &pig, '$')) return 0;
    i++;
    if (!parseNum(s, &i, &chicken, '\0')) return 0;
    if (cow < 1 || cow > 60 || horse < 1 || horse > 60 || pig < 1 || pig > 60 || chicken < 1 || chicken > 60) return 0;
    Farm_SetConfig(nameBuf, cow, horse, pig, chicken);
    return 1;
}

static unsigned char parseSleep(const char *s)
{
    unsigned char i = (s[1] == ':') ? 2 : 1;
    unsigned char species;
    unsigned char number;

    if (s[i] == 'V' && s[i + 1] == 'A' && s[i + 2] == 'C' && s[i + 3] == 'A' && s[i + 4] == '$') {
        species = FARM_COW;
        i = (unsigned char)(i + 5);
    } else if (s[i] == 'P' && s[i + 1] == 'O' && s[i + 2] == 'R' && s[i + 3] == 'C' && s[i + 4] == '$') {
        species = FARM_PIG;
        i = (unsigned char)(i + 5);
    } else if (s[i] == 'C' && s[i + 1] == 'A' && s[i + 2] == 'V' && s[i + 3] == 'A' &&
               s[i + 4] == 'L' && s[i + 5] == 'L' && s[i + 6] == '$') {
        species = FARM_HORSE;
        i = (unsigned char)(i + 7);
    } else if (s[i] == 'G' && s[i + 1] == 'A' && s[i + 2] == 'L' && s[i + 3] == 'L' &&
               s[i + 4] == 'I' && s[i + 5] == 'N' && s[i + 6] == 'A' && s[i + 7] == '$') {
        species = FARM_CHICKEN;
        i = (unsigned char)(i + 8);
    } else {
        return 0;
    }
    if (!parseNum(s, &i, &number, '\0') || number == 0) return 0;
    Farm_RequestSleep(species, number);
    return 1;
}

static unsigned char appendSpecies(unsigned char k, unsigned char s)
{
    if (s == FARM_COW) {
        txBuf[k++] = 'V'; txBuf[k++] = 'A'; txBuf[k++] = 'C'; txBuf[k++] = 'A';
    } else if (s == FARM_PIG) {
        txBuf[k++] = 'P'; txBuf[k++] = 'O'; txBuf[k++] = 'R'; txBuf[k++] = 'C';
    } else if (s == FARM_HORSE) {
        txBuf[k++] = 'C'; txBuf[k++] = 'A'; txBuf[k++] = 'V'; txBuf[k++] = 'A'; txBuf[k++] = 'L'; txBuf[k++] = 'L';
    } else {
        txBuf[k++] = 'G'; txBuf[k++] = 'A'; txBuf[k++] = 'L'; txBuf[k++] = 'L'; txBuf[k++] = 'I'; txBuf[k++] = 'N'; txBuf[k++] = 'A';
    }
    return k;
}

static void buildProducts(void)
{
    unsigned char k = 2;

    txBuf[0] = 'P';
    txBuf[1] = ':';
    k = appendNum(k, Farm_GetProductTotal(FARM_COW));
    txBuf[k++] = '$';
    k = appendNum(k, Farm_GetProductTotal(FARM_PIG));
    txBuf[k++] = '$';
    k = appendNum(k, Farm_GetProductTotal(FARM_CHICKEN));
    txBuf[k++] = '$';
    k = appendNum(k, Farm_GetProductTotal(FARM_HORSE));
    endLine(k);
}

static void buildAnimal(unsigned char animalIndex)
{
    unsigned char s, n, c, k = 2;

    Farm_GetAnimal(animalIndex, &s, &n, &c);
    txBuf[0] = 'A';
    txBuf[1] = ':';
    k = appendSpecies(k, s);
    txBuf[k++] = '$';
    k = appendNum(k, n);
    txBuf[k++] = '$';
    if (c) {
        txBuf[k++] = 'S'; txBuf[k++] = 'L'; txBuf[k++] = 'E'; txBuf[k++] = 'E'; txBuf[k++] = 'P';
    } else {
        txBuf[k++] = 'A'; txBuf[k++] = 'W'; txBuf[k++] = 'A'; txBuf[k++] = 'K'; txBuf[k++] = 'E';
    }
    endLine(k);
}

static unsigned char processLine(const char *line, unsigned char *animalIndex)
{
    unsigned char i;

    if (line[0] == 'I') {
        parseInit(line);
    } else if (line[0] == 'G' && line[1] == '\0') {
        buildProducts();
        return 1;
    } else if (line[0] == 'A' && line[1] == '\0') {
        *animalIndex = 0;
        return 2;
    } else if (line[0] == 'R' && line[1] == '\0') {
        Farm_Reset();
        SerialTime_Reset();
        Heartbeat_SetRebellion(0);
    } else if (line[0] == 'B' && line[1] == '\0') {
        Farm_SetRebellion(1);
        Heartbeat_SetRebellion(1);
    } else if (line[0] == 'P' && line[1] == '\0') {
        Farm_SetRebellion(0);
        Heartbeat_SetRebellion(0);
    } else if (line[0] == 'C') {
        i = (line[1] == ':') ? 2 : 1;
        if (isDigit(line[i]) && line[i + 1] == '\0') Farm_Consume((unsigned char)(line[i] - '0'));
    } else if (line[0] == 'S') {
        if (parseSleep(line)) return 4;
        txLine = "N\r\n";
        return 1;
    }
    return 0;
}

static unsigned char processInputs(void)
{
    unsigned char e;

    if (getButton()) {
        txLine = "S\r\n";
        return 1;
    }
    e = Joystick_GetEvent();
    if (e == JOY_UP) txLine = "U\r\n";
    else if (e == JOY_DOWN) txLine = "D\r\n";
    else if (e == JOY_LEFT) txLine = "L\r\n";
    else if (e == JOY_RIGHT) txLine = "R\r\n";
    else return 0;
    return 1;
}

void Controller_Init(void)
{
    TI_NewTimer(&timerHandle);
    txLine = 0;
}

void motorController(void)
{
    static unsigned char state = 0;
    static unsigned char animalIndex = 0;
    const char *line;

    Farm_SetCurrentDate(SerialTime_IsConfigured(), SerialTime_GetDay(), SerialTime_GetMonth(),
                        SerialTime_GetHour(), SerialTime_GetMinute(), SerialTime_GetSecond());

    if (state == 0) {
        line = SJ_GetLine();
        if (line != 0) state = processLine(line, &animalIndex);
        else state = processInputs();
    } else if (state == 1) {
        if (SJ_PutString(txLine)) state = 0;
    } else if (state == 2) {
        if (animalIndex < Farm_GetAnimalCount()) {
            buildAnimal(animalIndex);
            state = 3;
        } else {
            txLine = "F\r\n";
            state = 1;
        }
    } else if (state == 3) {
        if (SJ_PutString(txLine)) {
            animalIndex++;
            state = 2;
        }
    } else if (state == 4) {
        if (Farm_IsSleepSearchDone()) {
            if (Farm_IsSleepSearchFound()) {
                TI_ResetTics(timerHandle);
                state = 5;
            } else {
                txLine = "N\r\n";
                state = 1;
            }
        }
    } else {
        if (LDR_IsCovered()) {
            Farm_ApplySleep();
            txLine = "Y\r\n";
            state = 1;
        } else if (TI_GetTics(timerHandle) >= 5000) {
            txLine = "N\r\n";
            state = 1;
        }
    }
}
