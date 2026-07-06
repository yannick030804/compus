#include <xc.h>
#include "TAD_EEPROM.h"
#include "TAD_FARM.h"

#define FARM_MAX_NAME 16
#define FARM_MAX_ANIMALS 24
#define FARM_SPECIES 4
#define FARM_MIN_PER_SPECIES 3
#define FARM_CRITICAL 0x80
#define FARM_SPECIES_MASK 0x03
#define FARM_QUEUE 3
#define EE_CLEAR 1
#define EE_SAVE  2
#define SPECIES_OF(i) ((unsigned char)(animals[(i)].info & FARM_SPECIES_MASK))
#define IS_CRITICAL(i) ((unsigned char)((animals[(i)].info & FARM_CRITICAL) != 0))
#define PRODUCT_TIME(s) ((s) == FARM_COW ? 47 : ((s) == FARM_HORSE ? 23 : ((s) == FARM_PIG ? 31 : 13)))

typedef struct {
    unsigned char info;
    unsigned char day;
    unsigned char month;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} Animal;

char farmName[FARM_MAX_NAME + 1];
static Animal animals[FARM_MAX_ANIMALS];
static unsigned char genTime[FARM_SPECIES];
static unsigned char genCount[FARM_SPECIES];
static unsigned char prodCount[FARM_SPECIES];
unsigned char products[FARM_SPECIES];
static unsigned char animalCount[FARM_SPECIES];
static unsigned char criticalCount[FARM_SPECIES];
unsigned char totalAnimals;
unsigned char configured;
unsigned char rebellion;
static unsigned char dateValid;
static unsigned char curDay, curMonth, curHour, curMinute, curSecond;
static unsigned char lastDay, lastMonth, lastHour, lastMinute, lastSecond;
static unsigned char timeSeen;
static FarmNotification queue[FARM_QUEUE];
static unsigned char queueHead, queueCount;
static unsigned char sleepReq;
unsigned char sleepDone;
unsigned char sleepFound;
unsigned char restPending;
unsigned char restResult;
static unsigned char sleepSpecies, sleepNumber, sleepIndex, sleepSeen;
static unsigned char selectedIndex;
static unsigned char dirty, eeMode, eeIndex, eeField, eeAddr;

static void recount(void)
{
    unsigned char i, s;

    for (i = 0; i < FARM_SPECIES; i++) {
        animalCount[i] = 0;
        criticalCount[i] = 0;
    }
    for (i = 0; i < totalAnimals; i++) {
        s = SPECIES_OF(i);
        if (s < FARM_SPECIES) {
            animalCount[s]++;
            if (IS_CRITICAL(i)) criticalCount[s]++;
        }
    }
}

static void clearRuntime(void)
{
    unsigned char i;

    configured = 0;
    rebellion = 0;
    queueHead = 0;
    queueCount = 0;
    sleepReq = 0;
    sleepDone = 0;
    sleepFound = 0;
    restPending = 0;
    restResult = FARM_REST_NONE;
    selectedIndex = 255;
    timeSeen = 0;
    dirty = 0;
    farmName[0] = '\0';
    for (i = 0; i < FARM_SPECIES; i++) {
        genTime[i] = 0;
        genCount[i] = 0;
        prodCount[i] = 0;
        products[i] = 0;
        animalCount[i] = 0;
        criticalCount[i] = 0;
    }
    totalAnimals = 0;
}

static unsigned char isNextDate(unsigned char d, unsigned char m)
{
    unsigned char last = 31;

    if (curMonth == m && curDay == (unsigned char)(d + 1)) return 1;
    if (m == 2) last = 28;
    else if (m == 4 || m == 6 || m == 9 || m == 11) last = 30;
    if (curDay == 1 && curMonth == (unsigned char)(m + 1) && d == last) return 1;
    if (curDay == 1 && curMonth == 1 && m == 12 && d == 31) return 1;
    return 0;
}

static unsigned char elapsedCritical(const Animal *a)
{
    unsigned char minutes;

    if (dateValid == 0) return 0;
    if (a->day == curDay && a->month == curMonth) {
        if (curHour == a->hour) {
            if (curMinute < a->minute) return 0;
            minutes = (unsigned char)(curMinute - a->minute);
        } else if (curHour == (unsigned char)(a->hour + 1)) {
            minutes = (unsigned char)(60 - a->minute + curMinute);
        } else {
            return (unsigned char)(curHour > a->hour);
        }
        if (minutes > 2) return 1;
        if (minutes < 2) return 0;
        return (unsigned char)(curSecond >= a->second);
    }
    if (isNextDate(a->day, a->month) && a->hour == 23 && curHour == 0) {
        minutes = (unsigned char)(60 - a->minute + curMinute);
        if (minutes > 2) return 1;
        if (minutes < 2) return 0;
        return (unsigned char)(curSecond >= a->second);
    }
    return 1;
}

static void pushNotif(unsigned char kind, unsigned char species, unsigned char number)
{
    unsigned char tail;

    if (queueCount >= FARM_QUEUE) return;
    tail = (unsigned char)(queueHead + queueCount);
    if (tail >= FARM_QUEUE) tail = (unsigned char)(tail - FARM_QUEUE);
    queue[tail].kind = kind;
    queue[tail].species = species;
    queue[tail].number = number;
    queueCount++;
}

static unsigned char canCreate(unsigned char species)
{
    unsigned char i, c, missing = 0;

    if (totalAnimals >= FARM_MAX_ANIMALS) return 0;
    for (i = 0; i < FARM_SPECIES; i++) {
        c = animalCount[i];
        if (i == species) c++;
        if (c < FARM_MIN_PER_SPECIES) missing = (unsigned char)(missing + FARM_MIN_PER_SPECIES - c);
    }
    return (unsigned char)((unsigned char)(totalAnimals + 1 + missing) <= FARM_MAX_ANIMALS);
}

static void createAnimal(unsigned char species)
{
    if (canCreate(species) == 0) return;
    animals[totalAnimals].info = species;
    animals[totalAnimals].day = curDay;
    animals[totalAnimals].month = curMonth;
    animals[totalAnimals].hour = curHour;
    animals[totalAnimals].minute = curMinute;
    animals[totalAnimals].second = curSecond;
    totalAnimals++;
    animalCount[species]++;
    pushNotif(FARM_NOTIF_ANIMAL, species, animalCount[species]);
    dirty = 1;
}

static void serviceEEPROM(void)
{
    unsigned char *p;

    if (EEPROM_IsBusy()) return;

    if (eeMode == EE_CLEAR) {
        if (EEPROM_StartByteWrite(0, 0)) {
            eeMode = 0;
            dirty = 0;
        }
        return;
    }

    if (eeMode == 0) {
        if (dirty == 0 || configured == 0) return;
        eeMode = EE_SAVE;
        eeIndex = 0;
        eeField = 0;
        eeAddr = 1;
    }

    if (eeIndex < totalAnimals) {
        p = (unsigned char *)&animals[eeIndex];
        if (EEPROM_StartByteWrite(eeAddr, p[eeField])) {
            eeAddr++;
            eeField++;
            if (eeField >= 6) {
                eeField = 0;
                eeIndex++;
            }
        }
    } else if (EEPROM_StartByteWrite(0, totalAnimals)) {
        eeMode = 0;
        dirty = 0;
    }
}

static void loadEEPROM(void)
{
    unsigned char i, f, count, addr = 1;
    unsigned char *p;

    count = EEPROM_ReadByte(0);
    if (count > FARM_MAX_ANIMALS) return;
    totalAnimals = count;
    for (i = 0; i < totalAnimals; i++) {
        p = (unsigned char *)&animals[i];
        for (f = 0; f < 6; f++) {
            p[f] = EEPROM_ReadByte(addr++);
        }
        animals[i].info &= FARM_SPECIES_MASK;
    }
    recount();
}

static void processSearch(void)
{
    if (sleepIndex < totalAnimals) {
        if (SPECIES_OF(sleepIndex) == sleepSpecies) {
            sleepSeen++;
            if (sleepSeen == sleepNumber) {
                selectedIndex = sleepIndex;
                sleepFound = 1;
                sleepDone = 1;
                sleepReq = 0;
                restPending = 1;
                restResult = FARM_REST_NONE;
                return;
            }
        }
        sleepIndex++;
    } else {
        sleepFound = 0;
        sleepDone = 1;
        sleepReq = 0;
    }
}

static unsigned char newSecond(void)
{
    if (timeSeen == 0) {
        lastDay = curDay;
        lastMonth = curMonth;
        lastHour = curHour;
        lastMinute = curMinute;
        lastSecond = curSecond;
        timeSeen = 1;
        return 0;
    }
    if (lastSecond == curSecond && lastMinute == curMinute && lastHour == curHour &&
        lastDay == curDay && lastMonth == curMonth) {
        return 0;
    }
    lastDay = curDay;
    lastMonth = curMonth;
    lastHour = curHour;
    lastMinute = curMinute;
    lastSecond = curSecond;
    return 1;
}

void Farm_Init(void)
{
    clearRuntime();
    loadEEPROM();
}

void Farm_Reset(void)
{
    clearRuntime();
    eeMode = EE_CLEAR;
}

void Farm_SetConfig(const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken)
{
    unsigned char i;

    for (i = 0; i < FARM_MAX_NAME && name[i] != '\0'; i++) farmName[i] = name[i];
    farmName[i] = '\0';
    genTime[FARM_COW] = cow;
    genTime[FARM_HORSE] = horse;
    genTime[FARM_PIG] = pig;
    genTime[FARM_CHICKEN] = chicken;
    for (i = 0; i < FARM_SPECIES; i++) {
        genCount[i] = 0;
        prodCount[i] = 0;
        products[i] = 0;
    }
    configured = 1;
    timeSeen = 0;
}

void Farm_SetCurrentDate(unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second)
{
    dateValid = valid;
    if (valid) {
        curDay = day;
        curMonth = month;
        curHour = hour;
        curMinute = minute;
        curSecond = second;
    }
}

void motorFarm(void)
{
    static unsigned char state = 0;
    static unsigned char index = 0;
    unsigned char awake;
    unsigned char s;

    serviceEEPROM();
    if (sleepReq) processSearch();
    if (configured == 0 || dateValid == 0) return;

    if (state == 0) {
        if (newSecond() == 0) return;
        index = 0;
        state++;
    } else if (state == 1) {
        if (index < FARM_SPECIES) {
            if (genTime[index]) {
                genCount[index]++;
                if (genCount[index] >= genTime[index]) {
                    genCount[index] = 0;
                    createAnimal(index);
                }
            }
            index++;
        } else {
            index = 0;
            state++;
        }
    } else if (state == 2) {
        if (index < FARM_SPECIES) {
            if (rebellion == 0) {
                prodCount[index]++;
                if (prodCount[index] >= PRODUCT_TIME(index)) {
                    prodCount[index] = 0;
                    awake = (unsigned char)(animalCount[index] - criticalCount[index]);
                    if (awake) {
                        if (products[index] > (unsigned char)(255 - awake)) products[index] = 255;
                        else products[index] = (unsigned char)(products[index] + awake);
                        pushNotif(FARM_NOTIF_PRODUCT, index, products[index]);
                    }
                }
            }
            index++;
        } else {
            index = 0;
            state++;
        }
    } else {
        if (index < totalAnimals) {
            if (IS_CRITICAL(index) == 0 && elapsedCritical(&animals[index])) {
                s = SPECIES_OF(index);
                animals[index].info |= FARM_CRITICAL;
                criticalCount[s]++;
            }
            index++;
        } else {
            state = 0;
        }
    }
}

void Farm_GetAnimal(unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical)
{
    unsigned char i, n = 0, s = SPECIES_OF(index);

    for (i = 0; i <= index; i++) {
        if (SPECIES_OF(i) == s) n++;
    }
    *species = s;
    *number = n;
    *critical = IS_CRITICAL(index);
}

void Farm_Consume(unsigned char recipe)
{
    if (recipe == 0) {
        if (products[FARM_CHICKEN]) products[FARM_CHICKEN]--;
    } else if (recipe == 1) {
        if (products[FARM_CHICKEN] && products[FARM_PIG]) {
            products[FARM_CHICKEN]--;
            products[FARM_PIG]--;
        }
    } else if (recipe == 2) {
        if (products[FARM_COW] >= 2) products[FARM_COW] = (unsigned char)(products[FARM_COW] - 2);
    } else if (recipe == 3) {
        if (products[FARM_HORSE] >= 2) products[FARM_HORSE] = (unsigned char)(products[FARM_HORSE] - 2);
    }
}

unsigned char Farm_GetNotification(FarmNotification *notification)
{
    if (queueCount == 0) return 0;
    *notification = queue[queueHead];
    queueHead++;
    if (queueHead >= FARM_QUEUE) queueHead = 0;
    queueCount--;
    return 1;
}

void Farm_RequestSleep(unsigned char species, unsigned char number)
{
    sleepSpecies = species;
    sleepNumber = number;
    sleepIndex = 0;
    sleepSeen = 0;
    sleepDone = 0;
    sleepFound = 0;
    restPending = 0;
    restResult = FARM_REST_NONE;
    selectedIndex = 255;
    sleepReq = 1;
}

void Farm_NotifyRestSuccess(void)
{
    unsigned char s;

    if (selectedIndex < totalAnimals) {
        s = SPECIES_OF(selectedIndex);
        if (IS_CRITICAL(selectedIndex) && criticalCount[s]) criticalCount[s]--;
        animals[selectedIndex].info &= (unsigned char)(~FARM_CRITICAL);
        animals[selectedIndex].day = curDay;
        animals[selectedIndex].month = curMonth;
        animals[selectedIndex].hour = curHour;
        animals[selectedIndex].minute = curMinute;
        animals[selectedIndex].second = curSecond;
        dirty = 1;
    }
    selectedIndex = 255;
    restPending = 0;
    restResult = FARM_REST_SUCCESS;
}

void Farm_NotifyRestTimeout(void)
{
    restPending = 0;
    restResult = FARM_REST_TIMEOUT;
    selectedIndex = 255;
}
