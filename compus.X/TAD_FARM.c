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

typedef struct {
    unsigned char info;
    unsigned char day;
    unsigned char month;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} Animal;

static char farmName[FARM_MAX_NAME + 1];
static Animal animals[FARM_MAX_ANIMALS];
static unsigned char genTime[FARM_SPECIES];
static unsigned char genCount[FARM_SPECIES];
static unsigned char prodCount[FARM_SPECIES];
static unsigned char products[FARM_SPECIES];
static unsigned char animalCount[FARM_SPECIES];
static unsigned char criticalCount[FARM_SPECIES];
static unsigned char totalAnimals;
static unsigned char configured;
static unsigned char rebellion;
static unsigned char dateValid;
static unsigned char curDay, curMonth, curHour, curMinute, curSecond;
static unsigned char lastDay, lastMonth, lastHour, lastMinute, lastSecond;
static unsigned char timeSeen;
static FarmNotification queue[FARM_QUEUE];
static unsigned char queueHead, queueCount;
static unsigned char sleepReq, sleepDone, sleepFound;
static unsigned char sleepSpecies, sleepNumber, sleepIndex, sleepSeen;
static unsigned char selectedIndex;
static unsigned char dirty, eeMode, eeIndex, eeField, eeAddr;

static unsigned char speciesOf(unsigned char i) { return (unsigned char)(animals[i].info & FARM_SPECIES_MASK); }
static unsigned char isCriticalFlag(unsigned char i) { return (unsigned char)((animals[i].info & FARM_CRITICAL) != 0); }

static void clearCounts(void)
{
    unsigned char i;
    for (i = 0; i < FARM_SPECIES; i++) {
        animalCount[i] = 0;
        criticalCount[i] = 0;
    }
}

static void recount(void)
{
    unsigned char i, s;
    clearCounts();
    for (i = 0; i < totalAnimals; i++) {
        s = speciesOf(i);
        if (s < FARM_SPECIES) {
            animalCount[s]++;
            if (isCriticalFlag(i)) criticalCount[s]++;
        }
    }
}

static void clearRuntime(unsigned char clearAnimals)
{
    unsigned char i;

    configured = 0;
    rebellion = 0;
    queueHead = 0;
    queueCount = 0;
    sleepReq = 0;
    sleepDone = 0;
    sleepFound = 0;
    selectedIndex = 255;
    timeSeen = 0;
    dirty = 0;
    farmName[0] = '\0';
    for (i = 0; i < FARM_SPECIES; i++) {
        genTime[i] = 0;
        genCount[i] = 0;
        prodCount[i] = 0;
        products[i] = 0;
    }
    if (clearAnimals) totalAnimals = 0;
    recount();
}

static unsigned char daysInMonth(unsigned char month)
{
    if (month == 2) return 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

static unsigned char isNextDate(unsigned char d, unsigned char m)
{
    if (curMonth == m && curDay == (unsigned char)(d + 1)) return 1;
    if (curDay == 1 && curMonth == (unsigned char)(m + 1) && d == daysInMonth(m)) return 1;
    if (curDay == 1 && curMonth == 1 && m == 12 && d == 31) return 1;
    return 0;
}

static unsigned char elapsedCritical(const Animal *a)
{
    unsigned int now;
    unsigned int last;
    unsigned int elapsed;

    if (dateValid == 0) return 0;
    if (a->day == curDay && a->month == curMonth) {
        now = (((unsigned int)curHour << 6) - ((unsigned int)curHour << 2)) + curMinute;
        last = (((unsigned int)a->hour << 6) - ((unsigned int)a->hour << 2)) + a->minute;
        if (now < last) return 0;
        elapsed = now - last;
        if (elapsed > 2) return 1;
        if (elapsed < 2) return 0;
        return (unsigned char)(curSecond >= a->second);
    }
    if (isNextDate(a->day, a->month) && a->hour == 23 && curHour == 0) {
        elapsed = (unsigned int)(60 - a->minute);
        elapsed = (unsigned int)((elapsed << 6) - (elapsed << 2));
        elapsed = (unsigned int)(elapsed - a->second + (((unsigned int)curMinute << 6) - ((unsigned int)curMinute << 2)) + curSecond);
        return (unsigned char)(elapsed >= 120);
    }
    return 1;
}

static unsigned char productTime(unsigned char species)
{
    if (species == FARM_COW) return 47;
    if (species == FARM_HORSE) return 23;
    if (species == FARM_PIG) return 31;
    return 13;
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

static unsigned char recordByte(unsigned char index, unsigned char field)
{
    unsigned char *p = (unsigned char *)&animals[index];
    return p[field];
}

static void serviceEEPROM(void)
{
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
        if (EEPROM_StartByteWrite(eeAddr, recordByte(eeIndex, eeField))) {
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
        if (speciesOf(sleepIndex) == sleepSpecies) {
            sleepSeen++;
            if (sleepSeen == sleepNumber) {
                selectedIndex = sleepIndex;
                sleepFound = 1;
                sleepDone = 1;
                sleepReq = 0;
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
    clearRuntime(1);
    eeMode = 0;
    eeIndex = 0;
    eeField = 0;
    eeAddr = 1;
    loadEEPROM();
}

void Farm_Reset(void)
{
    clearRuntime(1);
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
                if (prodCount[index] >= productTime(index)) {
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
            if (isCriticalFlag(index) == 0 && elapsedCritical(&animals[index])) {
                s = speciesOf(index);
                animals[index].info |= FARM_CRITICAL;
                criticalCount[s]++;
            }
            index++;
        } else {
            state = 0;
        }
    }
}

unsigned char Farm_IsConfigured(void) { return configured; }
const char *Farm_GetName(void) { return farmName; }
unsigned char Farm_GetAnimalCount(void) { return totalAnimals; }

void Farm_GetAnimal(unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical)
{
    unsigned char i, n = 0, s = speciesOf(index);

    for (i = 0; i <= index; i++) {
        if (speciesOf(i) == s) n++;
    }
    *species = s;
    *number = n;
    *critical = isCriticalFlag(index);
}

unsigned char Farm_GetProductTotal(unsigned char species)
{
    return products[species];
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

void Farm_SetRebellion(unsigned char active)
{
    rebellion = active;
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
    selectedIndex = 255;
    sleepReq = 1;
}

unsigned char Farm_IsSleepSearchDone(void) { return sleepDone; }
unsigned char Farm_IsSleepSearchFound(void) { return sleepFound; }

void Farm_ApplySleep(void)
{
    unsigned char s;

    if (selectedIndex >= totalAnimals) return;
    s = speciesOf(selectedIndex);
    if (isCriticalFlag(selectedIndex) && criticalCount[s]) criticalCount[s]--;
    animals[selectedIndex].info &= (unsigned char)(~FARM_CRITICAL);
    animals[selectedIndex].day = curDay;
    animals[selectedIndex].month = curMonth;
    animals[selectedIndex].hour = curHour;
    animals[selectedIndex].minute = curMinute;
    animals[selectedIndex].second = curSecond;
    dirty = 1;
    selectedIndex = 255;
}
