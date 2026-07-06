#ifndef TAD_FARM_H
#define TAD_FARM_H

#define FARM_COW     0
#define FARM_HORSE   1
#define FARM_PIG     2
#define FARM_CHICKEN 3

#define FARM_NOTIF_ANIMAL  0
#define FARM_NOTIF_PRODUCT 1

typedef struct {
    unsigned char kind;
    unsigned char species;
    unsigned char number;
} FarmNotification;

void Farm_Init(void);
void motorFarm(void);
void Farm_Reset(void);
void Farm_SetConfig(const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken);
void Farm_SetCurrentDate(unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
unsigned char Farm_IsConfigured(void);
const char *Farm_GetName(void);
unsigned char Farm_GetAnimalCount(void);
void Farm_GetAnimal(unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
unsigned char Farm_GetProductTotal(unsigned char species);
void Farm_Consume(unsigned char recipe);
void Farm_SetRebellion(unsigned char active);
unsigned char Farm_GetNotification(FarmNotification *notification);
void Farm_RequestSleep(unsigned char species, unsigned char number);
unsigned char Farm_IsSleepSearchDone(void);
unsigned char Farm_IsSleepSearchFound(void);
void Farm_ApplySleep(void);

#endif
