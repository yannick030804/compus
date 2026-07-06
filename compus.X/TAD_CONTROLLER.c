#include <xc.h>
#include "TAD_BUTTON.h"
#include "TAD_CONTROLLER.h"
#include "TAD_FARM.h"
#include "TAD_HEARTBEAT.h"
#include "TAD_JOYSTICK.h"
#include "TAD_SERIAL_JAVA.h"
#include "TAD_SERIAL_TIME.h"

static const char *txLine;
static char txBuf[24];
static char nameBuf[17];

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

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

static void setReply(char c)
{
    txBuf[0] = c;
    endLine(1);
}

static unsigned char appendText(unsigned char k, const char *text)
{
    while (*text != '\0') txBuf[k++] = *text++;
    return k;
}

static unsigned char parseNum(const char *s, unsigned char *i, unsigned char *value, char end)
{
    unsigned char n = 0;
    unsigned char start = *i;

    while (IS_DIGIT(s[*i])) {
        n = (unsigned char)((n << 3) + (n << 1) + (unsigned char)(s[*i] - '0'));
        (*i)++;
    }
    if (*i == start || s[*i] != end) return 0;
    *value = n;
    return 1;
}

static unsigned char parseInit(const char *s)
{
    unsigned char i = (s[1] == ':') ? 2 : 1;
    unsigned char n = 0;
    unsigned char cow, pig, horse, chicken;

    while (s[i] != '$' && s[i] != '\0') {
        if (n < 16) nameBuf[n++] = s[i];
        i++;
    }
    if (n == 0 || s[i] != '$') return 0;
    nameBuf[n] = '\0';
    i++;
    if (!parseNum(s, &i, &cow, '$')) return 0;
    i++;
    if (!parseNum(s, &i, &pig, '$')) return 0;
    i++;
    if (!parseNum(s, &i, &horse, '$')) return 0;
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

    if (s[i] == 'V') {
        species = FARM_COW;
    } else if (s[i] == 'P') {
        species = FARM_PIG;
    } else if (s[i] == 'C') {
        species = FARM_HORSE;
    } else if (s[i] == 'G') {
        species = FARM_CHICKEN;
    } else {
        return 0;
    }
    while (s[i] != '$') {
        if (s[i] == '\0') return 0;
        i++;
    }
    i++;
    if (!parseNum(s, &i, &number, '\0') || number == 0) return 0;
    Farm_RequestSleep(species, number);
    return 1;
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
    if (s == FARM_COW) {
        k = appendText(k, "VACA");
    } else if (s == FARM_PIG) {
        k = appendText(k, "PORC");
    } else if (s == FARM_HORSE) {
        k = appendText(k, "CAVALL");
    } else {
        k = appendText(k, "GALLINA");
    }
    txBuf[k++] = '$';
    k = appendNum(k, n);
    txBuf[k++] = '$';
    if (c) {
        k = appendText(k, "SLEEP");
    } else {
        k = appendText(k, "AWAKE");
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
        if (Farm_GetAnimalCount() == 0) return 0;
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
        if (IS_DIGIT(line[i]) && line[i + 1] == '\0') Farm_Consume((unsigned char)(line[i] - '0'));
    } else if (line[0] == 'S') {
        if (parseSleep(line)) return 4;
        setReply('N');
        return 1;
    }
    return 0;
}

static unsigned char processInputs(void)
{
    unsigned char e;

    if (getButton()) {
        setReply('S');
        return 1;
    }
    e = Joystick_GetEvent();
    if (e == JOY_UP) setReply('U');
    else if (e == JOY_DOWN) setReply('D');
    else if (e == JOY_LEFT) setReply('L');
    else if (e == JOY_RIGHT) setReply('R');
    else return 0;
    return 1;
}

void motorController(void)
{
    static unsigned char state = 0;
    static unsigned char animalIndex = 0;
    const char *line;

    if (SerialTime_IsConfigured() == 0) {
        line = SJ_GetLine();
        if (line != 0 && line[0] == 'I') parseInit(line);
        state = 0;
        return;
    }
    Farm_SetCurrentDate(1, SerialTime_GetDay(), SerialTime_GetMonth(),
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
            state++;
        } else {
            setReply('F');
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
                state++;
            } else {
                setReply('N');
                state = 1;
            }
        }
    } else {
        if (Farm_GetRestResult() == FARM_REST_SUCCESS) {
            setReply('Y');
            state = 1;
        } else if (Farm_GetRestResult() == FARM_REST_TIMEOUT) {
            setReply('N');
            state = 1;
        }
    }
}
