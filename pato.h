#ifndef _PATO_H
#define _PATO_H

#include <stddef.h>
#include <stdint.h>

#include "../lib/include/stringptr.h"

#define MAX_PLAYERS 64
#define CITY_MAX_INDUSTRYTYPES 5
#define MAX_CITIES 64
#define MAX_BRANCHES 32
#define MAX_CONVOYS 64
#define MAX_NOTIFICATIONS 16

typedef enum {
	GT_NONE = 0,
	GT_WOOD,
	GT_MEAT,
	GT_VEG,
	GT_BARLEYJUICE,
	GT_CROP,
	GT_BRICK,
	GT_ORE,
	GT_HEMP,
	GT_HONEY,
	GT_COPPER,
	GT_SALT,
	GT_WOOL,
	GT_TOOL,
	GT_ROPE,
	GT_MET,
	GT_CERAMICS,
	GT_HAM,
	GT_CLOTH,
	GT_BEER,
	GT_FUR,
	GT_CHEESE,
	GT_PITCH,
	GT_FISH,
	GT_WINE,
	GT_MAX
} Goodtype;

typedef Goodtype IndustryType;

typedef enum {
	GC_NONE = 0,
	GC_A, // grundwaren
	GC_B, // rohstoffe
	GC_C, // fertigwaren
	GC_D, // regionalwaren
	GC_MAX
} Goodcategory;

typedef struct {
	stringptr* name;
	Goodtype type;
	Goodcategory cat;
} Good;

typedef struct {
	size_t buildcost;
	float yield;
	float consumation[GT_MAX];
} Factory;

typedef struct {
	size_t maxworkersperfactory;
	size_t workersalary;
} FactoryCommon;

typedef struct {
	size_t buildcost;
	size_t storage;
} Warehouse;


typedef struct {
	float stock[GT_MAX];
	float avgPricePayed[GT_MAX];
} Market;

typedef struct {
	float x;
	float y;
} Coords;

typedef enum {
	PT_NONE = 0,
	PT_BEGGAR,
	PT_WORKER,
	PT_COMMONER,
	PT_ARISTOCRAT,
	PT_MAX
} populationType;

typedef enum {
	ST_NONE = 0,
	ST_A,
	ST_B,
	ST_C,
	ST_D,
	ST_MAX
} ShipTypes;

typedef struct {
	size_t total;
	size_t numShips[ST_MAX];
} ShipCounter;

typedef struct {
	ShipCounter availableShips;
} Shipyard;

typedef struct {
	stringptr* name;
	Market market;
	Coords coords;
	float tax;
	long long money;
	IndustryType industry[CITY_MAX_INDUSTRYTYPES];
	size_t numFactories[CITY_MAX_INDUSTRYTYPES];
	size_t population[PT_MAX];
	float populationMood[PT_MAX];
	Shipyard shipyard;
} City;

typedef struct {
	stringptr* name;
	size_t maxload;
	size_t buildTime;
	size_t buildCost;
	size_t speed;
	size_t agility;
	size_t stability;
	size_t maxCannons;
	size_t minCrew;
	size_t maxCrew;
	size_t woodReq;
	size_t clothReq;
	size_t pitchReq;
	size_t ropeReq;
	size_t runningCost;
} ShipProps;

typedef enum {
	SLT_NONE = 0,
	SLT_SEA,
	SLT_CITY,
	SLT_MAX
} ShipLocationType;


typedef struct {
	float condition[ST_MAX];
} ShipCondition;

typedef struct {
	stringptr* name;
	ShipCounter shipcounter;
	ShipLocationType loc;
	size_t locCity;
	size_t fromCity;
	size_t stepsdone;
	Coords coords;
	size_t numSailors;
	size_t captainSalary;
	float totalload;
	Market load;
	float condition;
} Convoy;

typedef enum {
	PLT_NONE = 0,
	PLT_CPU,
	PLT_USER,
	PLT_MAX
} PlayerType;

typedef enum {
	NT_NONE = 0,
	NT_NO_MONEY,
	NT_BAD_MOOD,
	NT_OUT_OF_PRODUCTIONGOODS,
	NT_STOCK_FULL
} NotificationType;

typedef struct {
	NotificationType nt;
	size_t val1;
	size_t val2;
	float fval1;
	float fval2;
} Notification;

typedef struct {
	Notification notifications[MAX_NOTIFICATIONS];
	size_t last;
} Inbox;

typedef enum {
	AIP_NONE = 0,
	AIP_SATISFY_MOODS = 1,
	AIP_PURCHASE_PRODUCTION_GOODS = 2,
	AIP_SELL_GOODS_FROM_STOCK = 4
} AIPlan;

typedef struct {
	size_t moodybranch;
	Goodtype req_production_good;
	size_t branch_req_goods;
	float amount_required;
} Plandata;

typedef struct {
	stringptr* name;
	PlayerType type;
	size_t numBranches;
	size_t branchFactories[MAX_BRANCHES][CITY_MAX_INDUSTRYTYPES + 1]; // number of factories of each type. plus warehouses.
	Market branchStock[MAX_BRANCHES];
	size_t branchCity[MAX_BRANCHES];
	size_t branchWorkers[MAX_BRANCHES];
	size_t numConvoys;
	Convoy convoys[MAX_CONVOYS];
	size_t numShipLocations;
	size_t shipLocations[MAX_CITIES]; // stores the city "id" with single ships (those not in a convoy)
	ShipCounter singleShips[MAX_CITIES]; // stored in the order of shipLocations
	ShipCondition singleShipConditions[MAX_CITIES]; // same
	long long money;
	Inbox inbox;
	AIPlan plan;
	Plandata plandata;
} Player;

typedef struct {
	uint64_t date; // seconds till genesis
	size_t monthsperyear;
	size_t dayspermonth;
	size_t hoursperday;
	size_t minutesperhour;
	size_t secondsperminute;
	size_t _dayseconds;
	float _fdaysperyear;
} World;

typedef enum {
	SELL_FROM_OVERPRODUCTION = 1,
	SELL_FROM_STOCK = 2,
	SELL_FROM_CONVOY = 4,
	SELL_FROM_CITY = 8,
	SELL_TO_CITY = 16,
	SELL_TO_PLAYERSTOCK = 32,
	SELL_TO_CONVOY = 64,
	SELL_TO_POPULATION = 128
} sellFlags;

//lookup table for producer cities for certain goods.
extern size_t producers[GT_MAX][MAX_CITIES];
extern size_t producerCount[GT_MAX];
extern size_t numCities;
extern size_t numPlayers;
extern float populationConsumation[PT_MAX][GT_MAX];

extern World world;
extern Player Players[MAX_PLAYERS];
extern City Cities[MAX_CITIES];
extern const Notification emptyNotification;
extern const unsigned long long goodcat_minprice[];
extern const unsigned long long rentTax[PT_MAX];
extern FactoryCommon factoryProps;
extern Factory Factories[GT_MAX];
extern char externalGoodRequiredForProduction[GT_MAX]; // if another good is req. for the production of this
extern char goodRequiredForProduction[GT_MAX]; // if this good is required for the production of another good
extern Warehouse Storage;
extern Warehouse Branchoffice;
extern const Good Goods[];

extern size_t GAME_SPEED;

extern const ShipProps shipProps[];
extern const stringptr* populationDesc[];

PlayerType playerTypeFromString(stringptr* pt);
Notification makeNotification(NotificationType nt, size_t val1, size_t val2, float fval1, float fval2);
void notify(size_t player, Notification n);
stringptr* stringFromPopulationType(populationType p);
Goodtype goodTypeFromString(stringptr* good);
stringptr* stringFromGoodType(Goodtype g);
void initWorld(void);
void initConsumationTable(void);
void initCities(void);
ptrdiff_t findCityFromString(stringptr* name);
ShipLocationType shipLocationTypeFromString(stringptr* loc);
void initPlayers(void);
void initBuildings(void);
size_t getMaxWorkerCount(size_t branch, size_t player);
size_t getFreeJobs(size_t branch, size_t player);
void getPlayersWithFreeWorkCapacity(size_t city, size_t* totalcapacity, size_t* num_players, size_t* affected_players, size_t* free_jobs);
ptrdiff_t getShipLocationIDFromCity(size_t city, size_t player);
ptrdiff_t getCityIDFromBranch(size_t branch, size_t player);
ptrdiff_t getBranchIDFromCity(size_t city, size_t player);
size_t getPlayerMaxBranchStorage(size_t branch, size_t player);
float getPlayerFreeBranchStorage(size_t branch, size_t player);
size_t getPlayerFactoryCount(size_t branch, size_t player);
size_t getCityPopulation(size_t city);
unsigned long long calculatePrice(size_t city, Goodtype g, float amount, unsigned sell);
void sell(size_t city, size_t player, size_t branch, size_t convoy, Goodtype g, float amount, sellFlags flags);
void newDay(void);
size_t buyShips(size_t city, size_t player, size_t shipcount, ShipTypes t);
void addToConvoy(Convoy* c, size_t shiploc, size_t player, size_t numships, ShipTypes t);
ptrdiff_t makeConvoy(size_t city, size_t shiploc, size_t player, size_t numships, ShipTypes t);
size_t getMinCrew(size_t convoy, size_t player);
void hireCrew(size_t convoy, size_t player, size_t numsailors);
void hireCaptain(size_t convoy, size_t player);
unsigned requireProductionGood(size_t player, size_t branch, Goodtype t);
void sellWholeStock(size_t player, size_t branch, unsigned skiprequired);
unsigned convoysAvailable(size_t player, size_t branch);
float calculateDistance(Coords* a, Coords* b);
ptrdiff_t findNearestCityWithGood(size_t city, Goodtype neededgood);
void embark(Convoy* convoy, size_t to_city);
void land(Convoy* convoy);
float getConvoyMaxStorage(Convoy* c);
void moveGoodsConvoy(Convoy* c, Market* m, Goodtype g, float amount, unsigned fromConvoy);
float getSaneAmount(size_t player, size_t city, float amount, Goodtype g);
void purchaseFactories(size_t player, size_t city, Goodtype g, size_t amount);
unsigned canProduce(size_t city, Goodtype g);
void aiThink(void);
ShipTypes getSlowestShip(Convoy* convoy);
void newSec(void);
int microsleep(long microsecs);

#endif
