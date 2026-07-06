#include <xc.h>
#include "TAD_SERIAL_TIME.h"
#include "TAD_TIMER.h"

#define ST_LINE_LEN 14
#define DIGIT_AT(i) ((unsigned char)(rxLine[(i)] - '0'))
#define TWO_AT(i) ((unsigned char)((DIGIT_AT(i) << 3) + (DIGIT_AT(i) << 1) + DIGIT_AT((unsigned char)((i) + 1))))

static unsigned char timerHandle;
static volatile unsigned char rxByte;
static volatile unsigned char rxReady;
static unsigned char rxActive;
static unsigned char rxBit;
static unsigned char rxShift;
static unsigned char rxPos;
static unsigned char rxNext;
static unsigned char txActive;
static unsigned char txBit;
static unsigned char txShift;
static unsigned char txPos;
static unsigned char txNext;
static volatile unsigned char txEcho;
static const char * volatile txPtr;
static char rxLine[ST_LINE_LEN];
static unsigned char rxLen;

unsigned char serialTimeConfigured;
unsigned char serialTimeDay;
unsigned char serialTimeMonth;
unsigned char serialTimeHour;
unsigned char serialTimeMinute;
unsigned char serialTimeSecond;

static unsigned char parseDate(void)
{
    unsigned char d, m, h, mi, s;
    unsigned char i;

    if (rxLen != ST_LINE_LEN) return 0;
    if (rxLine[2] != '/' || rxLine[5] != ' ' || rxLine[8] != ':' || rxLine[11] != ':') return 0;
    for (i = 0; i < ST_LINE_LEN; i++) {
        if (i != 2 && i != 5 && i != 8 && i != 11 && (rxLine[i] < '0' || rxLine[i] > '9')) return 0;
    }

    d = TWO_AT(0);
    m = TWO_AT(3);
    h = TWO_AT(6);
    mi = TWO_AT(9);
    s = TWO_AT(12);
    if (d < 1 || d > 31 || m < 1 || m > 12 || h > 23 || mi > 59 || s > 59) return 0;

    serialTimeDay = d;
    serialTimeMonth = m;
    serialTimeHour = h;
    serialTimeMinute = mi;
    serialTimeSecond = s;
    serialTimeConfigured = 1;
    TI_ResetTics(timerHandle);
    return 1;
}

static void incClock(void)
{
    serialTimeSecond++;
    if (serialTimeSecond <= 59) return;
    serialTimeSecond = 0;
    serialTimeMinute++;
    if (serialTimeMinute <= 59) return;
    serialTimeMinute = 0;
    serialTimeHour++;
    if (serialTimeHour <= 23) return;
    serialTimeHour = 0;
    serialTimeDay++;
    if (serialTimeDay <= 31) return;
    serialTimeDay = 1;
    serialTimeMonth++;
    if (serialTimeMonth <= 12) return;
    serialTimeMonth = 1;
}

static void txTick(void)
{
    unsigned char b = 0;

    if (txActive == 0) {
        if (txEcho != 0) {
            b = txEcho;
            txEcho = 0;
        } else if (txPtr != 0) {
            if (*txPtr != '\0') b = *txPtr++;
            else txPtr = 0;
        }
        if (b != 0) {
            txShift = b;
            txBit = 0;
            txPos = 0;
            txNext = 3;
            txActive = 1;
        }
        return;
    }

    txPos = (unsigned char)(txPos + 3);
    if (txPos < txNext) return;
    txNext = (unsigned char)(txNext + 10);

    if (txBit == 0) LATBbits.LATB1 = 0;
    else if (txBit <= 8) {
        LATBbits.LATB1 = txShift & 1;
        txShift >>= 1;
    } else if (txBit == 9) LATBbits.LATB1 = 1;
    else {
        txActive = 0;
        return;
    }
    txBit++;
}

void SerialTime_Init(void)
{
    CONFIG_SERIAL_TIME;
    TI_NewTimer(&timerHandle);
    SerialTime_Reset();
    INTCON2bits.INTEDG2 = 0;
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT2IE = 1;
}

void SerialTime_Reset(void)
{
    serialTimeConfigured = 0;
    serialTimeDay = 1;
    serialTimeMonth = 1;
    serialTimeHour = 0;
    serialTimeMinute = 0;
    serialTimeSecond = 0;
    TI_ResetTics(timerHandle);
}

void motorSerialTime(void)
{
    unsigned char c;

    if (serialTimeConfigured && TI_GetTics(timerHandle) >= 1000) {
        TI_ResetTics(timerHandle);
        incClock();
    }

    if (rxReady == 0) return;
    c = rxByte;
    rxReady = 0;

    if (c == '\r' || c == '\n') {
        if (rxLen == 0) return;
        if (parseDate()) txPtr = "\r\nDate and time correct\r\n";
        else txPtr = "\r\nPlease input a correct date\r\n";
        rxLen = 0;
    } else {
        txEcho = c;
        if (rxLen < ST_LINE_LEN) rxLine[rxLen++] = (char)c;
        else rxLen = 0;
    }
}

void SerialTime_StartBitISR(void)
{
    INTCON3bits.INT2IE = 0;
    rxShift = 0;
    rxBit = 0;
    rxPos = 0;
    rxNext = 15;
    rxActive = 1;
}

void SerialTime_TickISR(void)
{
    if (rxActive) {
        rxPos = (unsigned char)(rxPos + 3);
        if (rxPos >= rxNext) {
            rxNext = (unsigned char)(rxNext + 10);
            if (rxBit < 8) {
                rxShift >>= 1;
                if (PORTBbits.RB2) rxShift |= 0x80;
                rxBit++;
            } else {
                if (PORTBbits.RB2) {
                    rxByte = rxShift;
                    rxReady = 1;
                }
                rxActive = 0;
                INTCON3bits.INT2IF = 0;
                INTCON3bits.INT2IE = 1;
            }
        }
    }
    txTick();
}
