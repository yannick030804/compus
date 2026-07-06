#ifndef TAD_FARM_H
#define TAD_FARM_H

#define FARM_COW     0
#define FARM_HORSE   1
#define FARM_PIG     2
#define FARM_CHICKEN 3

#define FARM_NOTIF_ANIMAL  0
#define FARM_NOTIF_PRODUCT 1
#define FARM_REST_NONE     0
#define FARM_REST_SUCCESS  1
#define FARM_REST_TIMEOUT  2

typedef struct {
    unsigned char kind;
    unsigned char species;
    unsigned char number;
} FarmNotification;

extern char farmName[];
extern unsigned char configured;
extern unsigned char totalAnimals;
extern unsigned char products[];
extern unsigned char sleepDone;
extern unsigned char sleepFound;
extern unsigned char rebellion;
extern unsigned char restPending;
extern unsigned char restResult;

#define Farm_IsConfigured()      (configured)
#define Farm_GetName()           (farmName)
#define Farm_GetAnimalCount()    (totalAnimals)
#define Farm_GetProductTotal(s)  (products[(s)])
#define Farm_IsSleepSearchDone() (sleepDone)
#define Farm_IsSleepSearchFound() (sleepFound)
#define Farm_SetRebellion(active) (rebellion = (active))
#define Farm_IsRestRequestPending() (restPending)
#define Farm_GetRestResult() (restResult)

void Farm_Init(void);
void motorFarm(void);
void Farm_Reset(void);
void Farm_SetConfig(const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken);
void Farm_SetCurrentDate(unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
void Farm_GetAnimal(unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
void Farm_Consume(unsigned char recipe);
unsigned char Farm_GetNotification(FarmNotification *notification);
void Farm_RequestSleep(unsigned char species, unsigned char number);
void Farm_NotifyRestSuccess(void);
void Farm_NotifyRestTimeout(void);

#endif
