/*
 * 
 * this is a game prototype modeled after "händler des nordens", aka "patrizier online"
 * it uses a ncurses based gui capable of 256 colors.
 * 
 * author (C) 2011 rofl0r
 * 
 * all source code files are 
 * 
 * LICENSE: GPL v3
 * 
 * creative content such as graphics or world describing texts are CC by SA (Creative Commons License)
 * 
 * 
 * 
TODO

wenn goodsreq4good > gesamtlager kapa, lager kaufen
bug workers hamburgpc total > worker players
gui.c menus for players

bug repeated resizing of the terminal will crash game

*/

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

#include "../lib/include/iniparser.h"

#include "pato.h"
#include "gui.h"

Gui *gui;

// gcc -Wall -Wextra -g -DNOKDEV pato.c ../lib/stringptr.c ../lib/iniparser.c -lm -o pato

size_t GAME_SPEED = 1;

const unsigned long long goodcat_minprice[] = {
	0,
	32,
	60,
	100,
	80
};

#ifndef IN_KDEVELOP_PARSER
const ShipProps shipProps[] = {
	{NULL, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{SPLITERAL("schnigge"),
		50 , 20, 15000, 10, 81, 70 , 8 , 4 , 24, 30, 10, 5 , 5 , 80 },
	{SPLITERAL("kraier"),
		80 , 30, 40000, 14, 90, 80 , 12, 12, 42, 50, 20, 10, 10, 240 },
	{SPLITERAL("kogge"),
		130, 30, 30000, 8 , 60, 100, 16, 6 , 56, 50, 15, 15, 10, 120 },
	{SPLITERAL("holk"),
		140, 40, 70000, 12, 75, 120, 20, 16, 76, 80, 30, 15, 20, 320 }
};
#endif

#ifndef IN_KDEVELOP_PARSER
const stringptr* populationDesc[] = {
	SPLITERAL("none"),
	SPLITERAL("beggar"),
	SPLITERAL("worker"),
	SPLITERAL("commoner"),
	SPLITERAL("aristocrat"),
};
#endif

const unsigned long long rentTax[PT_MAX] = {
	0,
	0,
	1,
	5,
	20
};

FactoryCommon factoryProps;

Factory Factories[GT_MAX];

char externalGoodRequiredForProduction[GT_MAX]; // if another good is req. for the production of this
char goodRequiredForProduction[GT_MAX]; // if this good is required for the production of another good

Warehouse Storage;
Warehouse Branchoffice;

#ifndef IN_KDEVELOP_PARSER

const Good Goods[] = {
	{ SPLITERAL("none"), GT_NONE, GC_NONE },
	{ SPLITERAL("wood"), GT_WOOD, GC_A },
	{ SPLITERAL("meat"), GT_MEAT, GC_A },
	{ SPLITERAL("veg"), GT_VEG, GC_A },
	{ SPLITERAL("barleyjuice"), GT_BARLEYJUICE, GC_A },
	{ SPLITERAL("crop"), GT_CROP, GC_A },
	{ SPLITERAL("brick"), GT_BRICK, GC_A },
	{ SPLITERAL("ore"), GT_ORE, GC_B },
	{ SPLITERAL("hemp"), GT_HEMP, GC_B },
	{ SPLITERAL("honey"), GT_HONEY, GC_B },
	{ SPLITERAL("copper"), GT_COPPER, GC_B },
	{ SPLITERAL("salt"), GT_SALT, GC_B },
	{ SPLITERAL("wool"), GT_WOOL, GC_B },
	{ SPLITERAL("tool"), GT_TOOL, GC_C },
	{ SPLITERAL("rope"), GT_ROPE, GC_C },
	{ SPLITERAL("met"), GT_MET, GC_C },
	{ SPLITERAL("ceramics"), GT_CERAMICS, GC_C },
	{ SPLITERAL("ham"), GT_HAM, GC_C },
	{ SPLITERAL("cloth"), GT_CLOTH, GC_C },
	{ SPLITERAL("beer"), GT_BEER, GC_D },
	{ SPLITERAL("fur"), GT_FUR, GC_D },
	{ SPLITERAL("cheese"), GT_CHEESE, GC_D },
	{ SPLITERAL("pitch"), GT_PITCH, GC_D },
	{ SPLITERAL("fish"), GT_FISH, GC_D },
	{ SPLITERAL("wine"), GT_WINE, GC_D },
};

const stringptr* NotificationNames[NT_MAX] = {
	[NT_NONE] = NULL,
	[NT_NO_MONEY] = SPL("out of money"),
	[NT_BAD_MOOD] = SPL("bad mood!"),
	[NT_OUT_OF_PRODUCTIONGOODS] = SPL("out of prod. goods"),
	[NT_STOCK_FULL] = SPL("stock full!"),
};

#endif


const Notification emptyNotification = {NT_NONE, 0, 0, 0.f, 0.f};
Player Players[MAX_PLAYERS];
World world;
City Cities[MAX_CITIES];

//lookup table for producer cities for certain goods.
size_t producers[GT_MAX][MAX_CITIES];
size_t producerCount[GT_MAX];

size_t numCities;
size_t numPlayers;

float populationConsumation[PT_MAX][GT_MAX];


PlayerType playerTypeFromString(stringptr* pt) {
	if(stringptr_eq(pt, SPLITERAL("pc")))
		return PLT_CPU;
	else return PLT_USER;
}


Notification makeNotification(NotificationType nt, size_t val1, size_t val2, float fval1, float fval2) {
	Notification result;
	result.nt = nt;
	result.val1 = val1;
	result.val2 = val2;
	result.fval1 = fval1;
	result.fval2 = fval2;
	return result;
}

stringptr* getNotificationName(NotificationType x) {
	return (stringptr*) NotificationNames[x];
}

void notify(size_t player, Notification n) {
	Notification* last;
	last = &Players[player].inbox.notifications[Players[player].inbox.last];
	if(last->nt == n.nt && last->val1 == n.val1 && last->val2 == n.val2)
		return;
	gui_notify(gui, player, n);
	Players[player].inbox.last++;
	if(Players[player].inbox.last >= MAX_NOTIFICATIONS)
		Players[player].inbox.last = 0;
	Players[player].inbox.notifications[Players[player].inbox.last] = n;
}

stringptr* stringFromPopulationType(populationType p) {
	if (p < PT_MAX) return (stringptr*) populationDesc[p];
	return (stringptr*) populationDesc[0];
}

Goodtype goodTypeFromString(stringptr* good) {
	size_t i;
	for(i = 0; i < GT_MAX; i++) {
		if(stringptr_eq(good, Goods[i].name))
			return Goods[i].type;
	}
	return GT_NONE;
}

stringptr* stringFromGoodType(Goodtype g) {
	if(g < GT_MAX) return Goods[g].name;
	return Goods[0].name;
}

void initWorld(void) {
	stringptr* cf = stringptr_fromfile("world.txt");
	stringptrlist* lines = stringptr_linesplit(cf);
	stringptr inibuf;
	ini_section sec;
	sec = iniparser_get_section(lines, SPLITERAL("main"));
	iniparser_getvalue(lines, &sec, SPLITERAL("date"), &inibuf);
	world.date = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("monthsperyear"), &inibuf);
	world.monthsperyear = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("dayspermonth"), &inibuf);
	world.dayspermonth = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("hoursperday"), &inibuf);
	world.hoursperday = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("minutesperhour"), &inibuf);
	world.minutesperhour = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("secondsperminute"), &inibuf);
	world.secondsperminute = atoi(inibuf.ptr);
	
	world._dayseconds = world.secondsperminute * world.minutesperhour * world.hoursperday;
	world._fdaysperyear = (float) world.dayspermonth * (float) world.monthsperyear;
	
	stringptr_free(cf);
	stringptrlist_free(lines);
}

void initConsumationTable(void) {
	size_t i, g;
	char buf[64];
	stringptr bufptr;
	stringptr* cf = stringptr_fromfile("consumation.txt");
	stringptrlist* lines = stringptr_linesplit(cf);
	stringptr inibuf;
	ini_section sec;
	bufptr.ptr = buf;
	for(g = 1; g < PT_MAX; g++) {
		sec = iniparser_get_section(lines, stringFromPopulationType(g));
		for(i = 1; i < GT_MAX; i++) {
			bufptr.size = snprintf(buf, sizeof(buf), "consumation_%s", stringFromGoodType(i)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			populationConsumation[g][i] = atof(inibuf.ptr);
		}
	}
	stringptr_free(cf);
	stringptrlist_free(lines);
}

void freeCities(void) {
	size_t i;
	for (i = 0; i < numCities; i++) {
		if(Cities[i].name)
			stringptr_free(Cities[i].name);
	}
}

void initCities(void) {
	size_t i, g;
	char buf[32];
	stringptr bufptr;
	stringptr* cf = stringptr_fromfile("cities.txt");
	stringptrlist* lines = stringptr_linesplit(cf);
	ini_section sec = iniparser_get_section(lines, SPLITERAL("main"));
	stringptr inibuf;
	iniparser_getvalue(lines, &sec, SPLITERAL("citycount"), &inibuf);
	numCities = atoi(inibuf.ptr);
	bufptr.ptr = buf;
	memset(producerCount, 0, sizeof(producerCount));
	for (i = 0; i < numCities; i++) {
		bufptr.size = snprintf(buf, sizeof(buf), "city%.3d", (int) i);
		sec = iniparser_get_section(lines, &bufptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("name"), &inibuf);
		Cities[i].name = stringptr_copy(&inibuf);
		iniparser_getvalue(lines, &sec, SPLITERAL("x"), &inibuf);
		Cities[i].coords.x = atof(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("y"), &inibuf);
		Cities[i].coords.y = atof(inibuf.ptr);

		iniparser_getvalue(lines, &sec, SPLITERAL("tax"), &inibuf);
		Cities[i].tax = atof(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("money"), &inibuf);
		Cities[i].money = atoll(inibuf.ptr);
		
		// available industry
		for(g = 0; g < CITY_MAX_INDUSTRYTYPES; g++) {
			bufptr.size = snprintf(buf, sizeof(buf), "industry%.3d", (int) g);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].industry[g] = goodTypeFromString(&inibuf);
			producers[Cities[i].industry[g]][producerCount[Cities[i].industry[g]]++] = i;
		}
		// market
		for(g = 1; g < GT_MAX; g++) {
			bufptr.size = snprintf(buf, sizeof(buf), "stock_%s", stringFromGoodType(g)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].market.stock[g] = atoi(inibuf.ptr);
			
			bufptr.size = snprintf(buf, sizeof(buf), "avgprice_%s", stringFromGoodType(g)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].market.avgPricePayed[g] = atof(inibuf.ptr);			
		}
		// population
		for(g = 1; g < PT_MAX; g++) {
			bufptr.size = snprintf(buf, sizeof(buf), "%s_mood", stringFromPopulationType(g)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].populationMood[g] = atof(inibuf.ptr);
			if(Cities[i].populationMood[g] > 100) 
				Cities[i].populationMood[g] = 100.0f;
			if(g == PT_WORKER) {
				Cities[i].population[g] = 0;
				continue;
			}	
			bufptr.size = snprintf(buf, sizeof(buf), "%s_count", stringFromPopulationType(g)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].population[g] = atoi(inibuf.ptr);
			
		}
		// shipyard
		for (g = 1; g < ST_MAX; g++) {
			bufptr.size = snprintf(buf, sizeof(buf), "shiptype%dcount", (int) (g - 1));
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Cities[i].shipyard.availableShips.numShips[g] = atoi(inibuf.ptr);
		}
	}
	stringptr_free(cf);
	stringptrlist_free(lines);
}

ptrdiff_t findCityFromString(stringptr* name) {
	size_t i;
	for(i = 0; i < numCities; i++) {
		if(stringptr_eq(Cities[i].name, name))
			return i;
	}
	return -1;
}

ShipLocationType shipLocationTypeFromString(stringptr* loc) {
	if(stringptr_eq(loc, SPLITERAL("sea")))
		return SLT_SEA;
	else
		return SLT_CITY;
}

stringptr* getPlayerName(size_t playerid) {
	return Players[playerid].name;
}

void freePlayers(void) {
	size_t p, s;
	for(p = 0; p < numPlayers; p++) {
		if(Players[p].name)
			stringptr_free(Players[p].name);
		for (s = 0; s < Players[p].numConvoys; s++) {
			if(Players[p].convoys[s].name)
				stringptr_free(Players[p].convoys[s].name);
		}
	}
}

void initPlayers(void) {
	size_t i, p, s, t;
	char buf[32];
	stringptr bufptr;
	stringptr* cf = stringptr_fromfile("players.txt");
	stringptrlist* lines = stringptr_linesplit(cf);
	ini_section sec = iniparser_get_section(lines, SPLITERAL("main"));
	stringptr inibuf;
	iniparser_getvalue(lines, &sec, SPLITERAL("playercount"), &inibuf);
	numPlayers = atoi(inibuf.ptr);
	bufptr.ptr = buf;
	stringptr_free(cf);
	stringptrlist_free(lines);
	for(p = 0; p < numPlayers; p++) {
		bufptr.size = snprintf(buf, sizeof(buf), "players/player%.6d.txt", (int) p);
		cf = stringptr_fromfile(buf);
		lines = stringptr_linesplit(cf);
		sec = iniparser_get_section(lines, SPLITERAL("main"));
		iniparser_getvalue(lines, &sec, SPLITERAL("name"), &inibuf);
		Players[p].name = stringptr_copy(&inibuf);
		iniparser_getvalue(lines, &sec, SPLITERAL("type"), &inibuf);
		Players[p].type = playerTypeFromString(&inibuf);
		iniparser_getvalue(lines, &sec, SPLITERAL("branches"), &inibuf);
		Players[p].numBranches = atoi(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("convoys"), &inibuf);
		Players[p].numConvoys = atoi(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("othershiplocations"), &inibuf);
		Players[p].numShipLocations = atoi(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("money"), &inibuf);
		Players[p].money = atoll(inibuf.ptr);
		//branches
		for(s = 0; s < Players[p].numBranches; s++) {
			bufptr.size = snprintf(buf, sizeof(buf), "branch%.3d", (int) s);
			sec = iniparser_get_section(lines, &bufptr);
			iniparser_getvalue(lines, &sec, SPLITERAL("city"), &inibuf);
			Players[p].branchCity[s] = findCityFromString(&inibuf);
			assert((ptrdiff_t) Players[p].branchCity[s] >= 0);
			iniparser_getvalue(lines, &sec, SPLITERAL("workers"), &inibuf);
			Players[p].branchWorkers[s] = atoi(inibuf.ptr);
			Cities[Players[p].branchCity[s]].population[PT_WORKER] += Players[p].branchWorkers[s];
			for(i = 0; i < CITY_MAX_INDUSTRYTYPES + 1; i++) {
				bufptr.size = snprintf(buf, sizeof(buf), "factorycount%.3d", (int) i);
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].branchFactories[s][i] = atoi(inibuf.ptr);				
			}
			// market
			for(i = 1; i < GT_MAX; i++) {
				bufptr.size = snprintf(buf, sizeof(buf), "stock_%s", stringFromGoodType(i)->ptr);
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].branchStock[s].stock[i] = atoi(inibuf.ptr);
				
				bufptr.size = snprintf(buf, sizeof(buf), "avgprice_%s", stringFromGoodType(i)->ptr);
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].branchStock[s].avgPricePayed[i] = atof(inibuf.ptr);
				
			}
		}
		//convoys
		for (s = 0; s < Players[p].numConvoys; s++) {
			bufptr.size = snprintf(buf, sizeof(buf), "convoy%.3d", (int) s);
			sec = iniparser_get_section(lines, &bufptr);
			
			iniparser_getvalue(lines, &sec, SPLITERAL("name"), &inibuf);
			Players[p].convoys[s].name = stringptr_copy(&inibuf);
			
			iniparser_getvalue(lines, &sec, SPLITERAL("captainsalary"), &inibuf);
			Players[p].convoys[s].captainSalary = atoi(inibuf.ptr);
			
			iniparser_getvalue(lines, &sec, SPLITERAL("sailors"), &inibuf);
			Players[p].convoys[s].captainSalary = atoi(inibuf.ptr);

			iniparser_getvalue(lines, &sec, SPLITERAL("condition"), &inibuf);
			Players[p].convoys[s].condition = atof(inibuf.ptr);
			
			iniparser_getvalue(lines, &sec, SPLITERAL("location"), &inibuf);
			Players[p].convoys[s].loc = shipLocationTypeFromString(&inibuf);
			
			if(Players[p].convoys[s].loc == SLT_CITY) {
				Players[p].convoys[s].locCity = findCityFromString(&inibuf);
			} else {
				iniparser_getvalue(lines, &sec, SPLITERAL("x"), &inibuf);
				Players[p].convoys[s].coords.x = atof(inibuf.ptr);
				
				iniparser_getvalue(lines, &sec, SPLITERAL("y"), &inibuf);
				Players[p].convoys[s].coords.y = atof(inibuf.ptr);
			}
			for(i = 1; i < ST_MAX; i++) {
				bufptr.size = snprintf(buf, sizeof(buf), "shiptype%dcount", (int) (i - 1));
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].convoys[s].shipcounter.numShips[i] = atoi(inibuf.ptr);
			}
			// goods loaded
			Players[p].convoys[s].totalload = 0;
			for(i = 1; i < GT_MAX; i++) {
				bufptr.size = snprintf(buf, sizeof(buf), "stock_%s", stringFromGoodType(i)->ptr);
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].convoys[s].load.stock[i] = atoi(inibuf.ptr);
				Players[p].convoys[s].totalload += Players[p].convoys[s].load.stock[i];
				
				bufptr.size = snprintf(buf, sizeof(buf), "avgprice_%s", stringFromGoodType(i)->ptr);
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].convoys[s].load.avgPricePayed[i] = atof(inibuf.ptr);
				
			}			
			
		}
		// single ships (well, kinda)
		// size_t shipLocations[MAX_CITIES]; // stores the city "id" with single ships (those not in a convoy)
		// ShipCounter singleShips[MAX_CITIES]; // stored in the order of shipLocations		
		
		
		for (s = 0; s < Players[p].numShipLocations; s++) {
			bufptr.size = snprintf(buf, sizeof(buf), "shiplocation%.3d", (int) s);
			sec = iniparser_get_section(lines, &bufptr);
			iniparser_getvalue(lines, &sec, SPLITERAL("city"), &inibuf);
			t = findCityFromString(&inibuf);
			
			assert((ptrdiff_t) t >= 0); 
			
			Players[p].shipLocations[s] = t;
			Players[p].singleShips[s].total = 0;
			
			for(i = 1; i < ST_MAX; i++) {
				bufptr.size = snprintf(buf, sizeof(buf), "shiptype%dcount", (int) (i - 1));
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].singleShips[s].numShips[i] = atoi(inibuf.ptr);
				Players[p].singleShips[s].total += Players[p].singleShips[s].numShips[i];
				
				bufptr.size = snprintf(buf, sizeof(buf), "shiptype%dcondition", (int) (i - 1));
				iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
				Players[p].singleShipConditions[s].condition[i] = atof(inibuf.ptr);
				
			}
		}
		stringptr_free(cf);
		stringptrlist_free(lines);
	}
}

void initBuildings(void) {
	size_t i, g;
	char buf[32];
	stringptr bufptr;
	bufptr.ptr = buf;
	stringptr inibuf;
	stringptr* cf = stringptr_fromfile("buildings.txt");
	stringptrlist* lines = stringptr_linesplit(cf);
	ini_section sec;
	int temp;
	
	sec = iniparser_get_section(lines, SPLITERAL("main"));
	iniparser_getvalue(lines, &sec, SPLITERAL("maxworkersperfactory"), &inibuf);
	factoryProps.maxworkersperfactory = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("workersalary"), &inibuf);
	factoryProps.workersalary = atoi(inibuf.ptr);
	
	sec = iniparser_get_section(lines, SPLITERAL("branch"));
	iniparser_getvalue(lines, &sec, SPLITERAL("buildcost"), &inibuf);
	Branchoffice.buildcost = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("storage"), &inibuf);
	Branchoffice.storage = atoi(inibuf.ptr);
	
	sec = iniparser_get_section(lines, SPLITERAL("warehouse"));
	iniparser_getvalue(lines, &sec, SPLITERAL("buildcost"), &inibuf);
	Storage.buildcost = atoi(inibuf.ptr);
	iniparser_getvalue(lines, &sec, SPLITERAL("storage"), &inibuf);
	Storage.storage = atoi(inibuf.ptr);
	
	memset(goodRequiredForProduction, 0, sizeof(goodRequiredForProduction));
	
	for(i = 1; i < GT_MAX; i++) {
		bufptr.size = snprintf(buf, sizeof(buf), "factory_%s", stringFromGoodType(i)->ptr);
		sec = iniparser_get_section(lines, &bufptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("buildcost"), &inibuf);
		Factories[i].buildcost = atoi(inibuf.ptr);
		iniparser_getvalue(lines, &sec, SPLITERAL("yield"), &inibuf);
		Factories[i].yield = atof(inibuf.ptr);
		temp = 0;
		for(g = 1; g < GT_MAX; g++) {
			bufptr.size = snprintf(buf, sizeof(buf), "consumation_%s", stringFromGoodType(g)->ptr);
			iniparser_getvalue(lines, &sec, &bufptr, &inibuf);
			Factories[i].consumation[g] = inibuf.ptr ? atof(inibuf.ptr) : 0.0f;
			if(Factories[i].consumation[g] > 0.f) {
				temp = 1;
				goodRequiredForProduction[g] = 1;
			}
		}
		if(temp)
			externalGoodRequiredForProduction[i] = 1;
		else
			externalGoodRequiredForProduction[i] = 0;
	}
	stringptr_free(cf);
	stringptrlist_free(lines);	
}

size_t getMaxWorkerCount(size_t branch, size_t player) {
	size_t result = 0;
	size_t i;
	for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
		result += Players[player].branchFactories[branch][i] * 10;
	}
	return result;
}

size_t getFreeJobs(size_t branch, size_t player) {
	size_t result = getMaxWorkerCount(branch, player);
	if(result) result -= Players[player].branchWorkers[branch];
	return result;
}

void getPlayersWithFreeWorkCapacity(size_t city, size_t* totalcapacity, size_t* num_players, size_t* affected_players, size_t* free_jobs) {
	size_t p, b, c;
	*num_players = 0;
	*totalcapacity = 0;
	
	for(p = 0; p < numPlayers; p++) {
		if(Players[p].money) {
			for(b = 0; b < Players[p].numBranches; b++) {
				if(Players[p].branchCity[b] == city) {
					c = getFreeJobs(b, p);
					if(c) {
						*totalcapacity += c;
						(*num_players)++;
						affected_players[*num_players - 1] = p;
						free_jobs[*num_players - 1] = c;
						break;
					}
				}
			}
		}
	}
}

ptrdiff_t getShipLocationIDFromCity(size_t city, size_t player) {
	size_t i;
	for(i = 0; i < Players[player].numShipLocations; i++) {
		if(Players[player].shipLocations[i] == city)
			return i;
	}
	return -1;
}

ptrdiff_t getCityIDFromBranch(size_t branch, size_t player) {
	if(branch >= Players[player].numBranches) return -1;
	return Players[player].branchCity[branch];
}

ptrdiff_t getBranchIDFromCity(size_t city, size_t player) {
	size_t b;
	for(b = 0; b < Players[player].numBranches; b++) {
		if(Players[player].branchCity[b] == city) {
			return b;
		}
	}
	return -1;
}

size_t getPlayerMaxBranchStorage(size_t branch, size_t player) {
	size_t result = 0;
	result += Players[player].branchFactories[branch][CITY_MAX_INDUSTRYTYPES] * Storage.storage;
	result += Branchoffice.storage;
	return result;
}

float getPlayerFreeBranchStorage(size_t branch, size_t player) {
	float result = (float) getPlayerMaxBranchStorage(branch, player);
	size_t i;
	for(i = 1; i < GT_MAX; i++) {
		result -= Players[player].branchStock[branch].stock[i];
	}
	return result;
}

size_t getPlayerFactoryCount(size_t branch, size_t player) {
	size_t result = 0;
	size_t i;
	for (i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
		result += Players[player].branchFactories[branch][i];
	}
	return result;
}

size_t getCityPopulation(size_t city) {
	size_t i, result = 0;
	for(i = 1; i < PT_MAX; i++) {
		result += Cities[city].population[i];
	}
	return result;
}

// return price for 1 ton
unsigned long long calculatePrice(size_t city, Goodtype g, float amount, unsigned sell) {
	unsigned long long minprice = goodcat_minprice[Goods[g].cat];
	//Cities[city].market.avgPricePayed[g];
	float factor = (float) ((Cities[city].market.stock[g] - amount) * 2) + 0.99f / (float) (getCityPopulation(city) + 1);
	float value = minprice + ((minprice * (sell ? 3 : 2)) * (1.f - factor));
	if (value < (float) minprice) 
		value = (float) minprice;
	if(sell)
		value = value * (1.f + (Cities[city].tax / 100.f));
	return (unsigned long long) value;
}

void sell(size_t city, size_t player, size_t branch, size_t convoy, Goodtype g, float amount, sellFlags flags) {
	size_t price;
	Market* buyermarket;
	Market* sellermarket;
	long long* buyerportemonnaie;
	long long* sellerportemonnaie;
	unsigned citysells = 0;
	if(flags & SELL_TO_CITY) {
		buyermarket = &Cities[city].market;
		buyerportemonnaie = &Cities[city].money;
	} else if (flags & SELL_TO_CONVOY) {
		citysells = 1;
		buyermarket = &Players[player].convoys[convoy].load;
		Players[player].convoys[convoy].totalload += amount;
		buyerportemonnaie = &Players[player].money;
	} else if (flags & SELL_TO_PLAYERSTOCK) {
		citysells = 1;
		buyermarket = &Players[player].branchStock[branch];
		buyerportemonnaie = &Players[player].money;
	} else if (flags & SELL_TO_POPULATION) {
		citysells = 1;
		buyermarket = NULL;
		buyerportemonnaie = NULL;
	} else {
		assert(0);
	}
	
	if(flags & SELL_FROM_OVERPRODUCTION) {
		sellermarket = NULL;
		sellerportemonnaie = &Players[player].money;
	} else if (flags & SELL_FROM_CONVOY) {
		sellermarket = &Players[player].convoys[convoy].load;
		Players[player].convoys[convoy].totalload -= amount;
		sellerportemonnaie = &Players[player].money;
	} else if (flags & SELL_FROM_STOCK) {
		sellermarket =  &Players[player].branchStock[branch];
		sellerportemonnaie = &Players[player].money;
	} else if (flags & SELL_FROM_CITY) {
#ifdef CONSOLE_DEBUG		
		if(player != (size_t) -1)
			printf("player %d buys %f %s from %d!\n", (int) player, amount, Goods[g].name->ptr, (int) city);
#endif		
		sellermarket = &Cities[city].market;
		sellerportemonnaie = &Cities[city].money;
	} else {
		assert(0);
	}
	
	price = calculatePrice(city, g, amount, citysells);
	
	if(buyerportemonnaie)
		*buyerportemonnaie -= (unsigned long long) ((float) price * amount);
	
	if(sellerportemonnaie)
		*sellerportemonnaie += (unsigned long long) ((float) price * amount);
	
	if(sellermarket)
		sellermarket->stock[g] -= amount;
	
	if(buyermarket)
		buyermarket->stock[g] += amount;
}

#ifdef CONSOLE_DEBUG
void showDate(void) {
	puts("=================================================");
	printf("day: %lu\n", world.date / world._dayseconds);
}

void showPlayerStatus(size_t player, size_t branch) {
	puts("=================================================");
	printf("status for %s\n", Players[player].name->ptr);
	printf("money: %llu\n", Players[player].money);
	printf("workers: %d\n", (int) Players[player].branchWorkers[branch]);
	printf("free stock: %d\n", (int) getPlayerFreeBranchStorage(branch, player));
}

void showCityStatus(size_t city) {
	size_t i;
	puts("=================================================");
	printf("status for %s\n", Cities[city].name->ptr);
	printf("money: %llu\n", Cities[city].money);
	printf("population:\n");
	for(i = 1; i < PT_MAX; i++) {
		printf("%s: count: %d, mood: %f\n", stringFromPopulationType(i)->ptr, (int) Cities[city].population[i], Cities[city].populationMood[i]);
	}
	printf("\nmarket:\n");
	for(i = 1; i < GT_MAX; i++) {
		printf("%s: stock %f\n", stringFromGoodType(i)->ptr, Cities[city].market.stock[i]);
	}
}
#endif

void newDay(void) {
	size_t i, g, p, pl, plsave, b, f;
	float stock;
	float maxconsumption[PT_MAX];
	float consumed[PT_MAX];
	float wantsconsume;
	float percent, leavepercent;
	size_t leaving, leavingp;
	size_t affected_players[MAX_PLAYERS];
	size_t free_jobs[MAX_PLAYERS];
	size_t totalcapacity;
	size_t num_players_affected;
	size_t luckyplayer;
	ptrdiff_t branchid = -1;
	float freestorage;
	float workersPerFactory;
	float produced, required;
	long long cost;
	
	for (i = 0; i < numCities; i++) {
		// create ships
		if(Cities[i].shipyard.availableShips.numShips[ST_A] < 2) {
			Cities[i].shipyard.availableShips.numShips[ST_A] = 2;
			// TODO: decrease stock/money for goods necessary to build ships...
		}
		
		// cash-in rent-tax...
		for(p = 1; p < PT_MAX; p++) {
			Cities[i].money += rentTax[p] * Cities[i].population[p];
		}
		
		// consumation of goods.
		for(p = 1; p < PT_MAX; p++) {
			maxconsumption[p] = 0.0f;
			consumed[p] = 0.0f;
			for(g = 1; g < GT_MAX; g++) {
				maxconsumption[p] += (populationConsumation[p][g] / world._fdaysperyear) * Cities[i].population[p];
			}
		}
		
		for(g = 1; g < GT_MAX; g++) {
			for(p = 1; p < PT_MAX; p++) {
				if(Cities[i].population[p] && populationConsumation[p][g] > 0.f) {
					stock = Cities[i].market.stock[g];
					wantsconsume = (populationConsumation[p][g] / world._fdaysperyear) * Cities[i].population[p];
					if(stock > wantsconsume) {
						sell(i, -1, -1, -1, g, wantsconsume, SELL_TO_POPULATION | SELL_FROM_CITY);
						//Cities[i].market.stock[g] -= wantsconsume;
						consumed[p] += wantsconsume;
					} else {
						
						consumed[p] = Cities[i].market.stock[g];
						sell(i, -1, -1, -1, g, consumed[p], SELL_TO_POPULATION | SELL_FROM_CITY);
						//Cities[i].market.stock[g] = 0;
					}
				} else {
					consumed[p] = 0.f;
				}
			}	
		}
		// decrease mood by max 2%, if no wares consumed, and let a random number between 0 and 10% percent leave the city if mood below 10%
		for(p = 1; p < PT_MAX; p++) {
			
			// only do mood adjustement if there ARE some ppl of that type
			if(Cities[i].population[p]) {
				percent = maxconsumption[p] / 100;
				percent = consumed[p] / percent;
				if(percent > 66.6f)
					Cities[i].populationMood[p] += (1.0f / 100.0f) * percent;
				else {
					Cities[i].populationMood[p] -= 2.0f - ((2.0f / 100.0f) * percent);
					// notify players with branches...
					for(pl = 0; pl < numPlayers; pl++) {
						for(b = 0; b < Players[pl].numBranches; b++) {
							if(Players[pl].branchCity[b] == i)
								notify(pl, makeNotification(NT_BAD_MOOD, b, 0, 0.f, 0.f));
						}
					}
				}
				// correct bounds
				if(Cities[i].populationMood[p] > 100.0f)
					Cities[i].populationMood[p] = 100.0f;
				else if(Cities[i].populationMood[p] < 0.0f)
					Cities[i].populationMood[p] = 0.0f;
				
				
			} else {
				// if there are no ppl we take the average of the other folk moods, and increase it a bit.
				wantsconsume = 0.f;
				for(pl = 1; pl < PT_MAX; pl++) {
					if(pl != p)
						wantsconsume += Cities[i].populationMood[pl];
				}
				Cities[i].populationMood[p] += (wantsconsume / (float) (PT_MAX -2)) + 3.f; // PT_MAX -2 == number of effective population types.
				if(Cities[i].populationMood[p] > 100.f)
					Cities[i].populationMood[p] = 100.f;
			}
			
			if(Cities[i].population[p] && Cities[i].populationMood[p] < 10.0f) {
				leaving = Cities[i].population[p] > 20 ? rand() % (Cities[i].population[p] / 10) : 1;
				if(leaving > Cities[i].population[p])
					leaving = Cities[i].population[p];
				
				leavepercent = leaving / ((float) Cities[i].population[p] / 100.0f);
				Cities[i].population[p] -= leaving;
				
				if(p == PT_WORKER) {
					
					for(pl = 0; pl < numPlayers; pl++) {
						for(b = 0; b < Players[pl].numBranches; b++) {
							if(Players[pl].branchCity[b] == i && Players[pl].branchWorkers[b]) {
								leavingp = (size_t) (((float) Players[pl].branchWorkers[b] / 100.0f) * leavepercent) + 0.99f;
								if (leavingp > leaving)
									leavingp = leaving;
								leaving -= leavingp;
								Players[pl].branchWorkers[b] -= leavingp;
								if (!leaving) break;
							}
						}
					}
				}
			} else if (p != PT_WORKER && Cities[i].populationMood[p] > 85.0f) {
				// mood is good, so they recommend the city to their friends.
				percent = (float) Cities[i].population[p] / 100.0f;
				// lets say 2 percent talk to one outside friend each day, from those a random number will join the city.
				// read "leaving" in this context as "joining"
				if(!Cities[i].population[p] && p == PT_BEGGAR)
					percent = (float) Cities[i].population[PT_WORKER] / 100.0f;
				if(percent > 0.01f)
					leaving = rand() % (1 + (int) ((2.f * percent) + 0.999f));
				else
					leaving = rand() % 2;
				
				Cities[i].population[p] += leaving;
			}
		}
		if (Cities[i].population[PT_BEGGAR] > 0 && Cities[i].populationMood[PT_WORKER] > 60.f) {
			// see if we have work for beggars...
			leaving = Cities[i].population[PT_BEGGAR];
			// we need a list of players with free resources, and their number n
			// then we pick a random one for n times, and give them a random number of workers from the "pool".
			// if there are some left, we'll iterate the list and distribute them evenly.
			getPlayersWithFreeWorkCapacity(i, &totalcapacity, &num_players_affected, affected_players, free_jobs);
			if(totalcapacity < leaving) 
				leaving = totalcapacity;
			
			totalcapacity = leaving; // we use that variable now for reminding how many ppl we have left to distribute.
			for(pl = 0; pl < num_players_affected; pl++) {
				leavingp = rand() % totalcapacity;
				plsave = rand() % num_players_affected;
				luckyplayer = affected_players[plsave];
				if(leavingp > free_jobs[plsave])
					leavingp = free_jobs[plsave];
				branchid = getBranchIDFromCity(i, luckyplayer);
				Players[luckyplayer].branchWorkers[branchid] += leavingp;
				totalcapacity -= leavingp;
				Cities[i].population[PT_WORKER] += leavingp;
				Cities[i].population[PT_BEGGAR] -= leavingp;
				if(!totalcapacity) break;
			}
			if(totalcapacity) {
				getPlayersWithFreeWorkCapacity(i, &leavingp, &num_players_affected, affected_players, free_jobs);
				leaving = totalcapacity / num_players_affected;
				for(pl = 0; pl < num_players_affected; pl++) {
					leavingp = leaving;
					luckyplayer = affected_players[pl];
					if(leavingp > free_jobs[pl]) {
						leavingp = free_jobs[pl];
						totalcapacity -= free_jobs[pl];
						leaving = totalcapacity / (num_players_affected - (pl + 1));
					} else 
						totalcapacity -= leavingp;
					Players[luckyplayer].branchWorkers[branchid] += leavingp;
					Cities[i].population[PT_WORKER] += leavingp;
					Cities[i].population[PT_BEGGAR] -= leavingp;
				}
			}
		}
		
		for(pl = 0; pl < numPlayers; pl++) {
			
			branchid = getBranchIDFromCity(i, pl);
			if(branchid >= 0) {
				// now to the factories.
				if(Players[pl].branchWorkers[branchid]) {
					workersPerFactory = (float) Players[pl].branchWorkers[branchid] / (float) getPlayerFactoryCount(branchid, pl);
					percent = workersPerFactory / ((float) factoryProps.maxworkersperfactory / 100.f);
					for(f = 0; f < CITY_MAX_INDUSTRYTYPES; f++) {
						if(Players[pl].branchFactories[branchid][f]) {
							produced = (Factories[Cities[i].industry[f]].yield / 100.f) * percent * Players[pl].branchFactories[branchid][f];
							
							if(externalGoodRequiredForProduction[Cities[i].industry[f]]) {
								// check if all required goods are in stock, and reduce produced amount if not
								for(g = 1; g < GT_MAX; g++) {
									if(Factories[Cities[i].industry[f]].consumation[g] > 0.f) {
										required = Factories[Cities[i].industry[f]].consumation[g] * percent * Players[pl].branchFactories[branchid][f];
										if(required > 0.f) {
											if(Players[pl].branchStock[branchid].stock[g] < required) {
												produced = Players[pl].branchStock[branchid].stock[g] / required * produced;
												notify(pl, makeNotification(NT_OUT_OF_PRODUCTIONGOODS, branchid, g, required, 0.f));
											}
										}
									}	
								}
								if(produced > 0.f) {
									for(g = 1; g < GT_MAX; g++) {
										if(Factories[Cities[i].industry[f]].consumation[g] > 0.f) {
											required = (Factories[Cities[i].industry[f]].yield / Factories[Cities[i].industry[f]].consumation[g]) * produced;
											Players[pl].branchStock[branchid].stock[g] -= required;
										}	
									}
								}	
							}
							if(produced > 0.f) {
								freestorage = getPlayerFreeBranchStorage(branchid, pl);
								if(freestorage >= produced)
									Players[pl].branchStock[branchid].stock[Cities[i].industry[f]] += produced;
								else {
									Players[pl].branchStock[branchid].stock[Cities[i].industry[f]] += freestorage;
									sell(i, pl, branchid, (size_t) -1, Cities[i].industry[f], produced - freestorage, SELL_FROM_OVERPRODUCTION | SELL_TO_CITY);
									notify(pl, makeNotification(NT_STOCK_FULL, branchid, 0, produced - freestorage, 0.f));
								}
							}	
						}
					}
				}
				// and to the money.
				cost = Players[pl].branchWorkers[branchid] * factoryProps.workersalary;
				if(Players[pl].money >= cost)
					Players[pl].money -= cost;
				else {
					leaving = Players[pl].branchWorkers[branchid];
					produced = (float) Players[pl].money / (float) factoryProps.workersalary;
					if(produced >= 1.f) {
						leaving -= (size_t) produced;
					}
					Players[pl].money = 0;
					Players[pl].branchWorkers[branchid] -= leaving;
					Cities[i].population[PT_WORKER] -= leaving;
					Cities[i].population[PT_BEGGAR] += leaving;
					notify(pl, makeNotification(NT_NO_MONEY, 0, 0, 0.f, 0.f));
				}
			}
		}
	}
#ifdef CONSOLE_DEBUG	
	showDate();
	showPlayerStatus(0, 0);
	showCityStatus(0);
#endif	
}

size_t buyShips(size_t city, size_t player, size_t shipcount, ShipTypes t) {
	size_t singleShipCost = (size_t) ((float) shipProps[t].buildCost * 1.6f);
	size_t i;
	ptrdiff_t loc = -1;
	while(shipcount && Players[player].money < (long long) (shipcount * singleShipCost)) shipcount--;
	if(shipcount) {
		Players[player].money -= singleShipCost * shipcount;
		Cities[city].money += singleShipCost * shipcount;
		Cities[city].shipyard.availableShips.numShips[t] -= shipcount;
		for(i = 0; i < Players[player].numShipLocations; i++) {
			if(Players[player].shipLocations[i] == city) {
				loc = i;
				break;
			}
		}
		if(loc < 0) {
			loc = Players[player].numShipLocations;
			Players[player].numShipLocations++;
			Players[player].shipLocations[loc] = city;
		}
		Players[player].singleShips[loc].numShips[t] += shipcount;
		Players[player].singleShips[loc].total += shipcount;
	}
	return shipcount;
}

void addToConvoy(Convoy* c, size_t shiploc, size_t player, size_t numships, ShipTypes t) {
	size_t i;
	if(t != ST_NONE) {
		if(numships > Players[player].singleShips[shiploc].numShips[t])
			numships = Players[player].singleShips[shiploc].numShips[t];
		c->shipcounter.numShips[t] += numships;
		c->shipcounter.total += numships;
		Players[player].singleShips[shiploc].numShips[t] -= numships;
		Players[player].singleShips[shiploc].total -= numships;
	} else {
		// ST_NONE acts as Wildcard here
		for (i = 1; i < ST_MAX; i++) {
				c->shipcounter.numShips[i] += Players[player].singleShips[shiploc].numShips[i];
				c->shipcounter.total += Players[player].singleShips[shiploc].numShips[i];
				Players[player].singleShips[shiploc].numShips[i] = 0;
				Players[player].singleShips[shiploc].total = 0;
		}
	}
	if(!Players[player].singleShips[shiploc].total && Players[player].numShipLocations == 1)
		Players[player].numShipLocations = 0;
}

ptrdiff_t makeConvoy(size_t city, size_t shiploc, size_t player, size_t numships, ShipTypes t) {
	Convoy* c;
	char buf[16];
	stringptr bufptr;
	bufptr.ptr = buf;
	ptrdiff_t result = -1;
	if(Players[player].numConvoys < MAX_CONVOYS) {
		c = &Players[player].convoys[Players[player].numConvoys];
		memset(c, 0, sizeof(Convoy));
		bufptr.size = snprintf(buf, sizeof(buf), "Convoy%.3d", (int) Players[player].numConvoys);
		c->name = stringptr_copy(&bufptr);
		c->loc = SLT_CITY;
		c->locCity = city;
		addToConvoy(c, shiploc, player, numships, t);
			
		result = Players[player].numConvoys;
		Players[player].numConvoys++;
	}
	return result;
}

size_t getMinCrew(size_t convoy, size_t player) {
	size_t i;
	size_t result = 0;
	if(convoy >= MAX_CONVOYS) return 0;
	for(i = 1; i < ST_MAX; i++) {
		result += shipProps[i].minCrew * Players[player].convoys[convoy].shipcounter.numShips[i];;
	}
	return result;
}

void hireCrew(size_t convoy, size_t player, size_t numsailors) {
	if(convoy >= MAX_CONVOYS) return;
	Players[player].convoys[convoy].numSailors += numsailors;
}

void hireCaptain(size_t convoy, size_t player) {
	if(convoy >= MAX_CONVOYS) return;
	Players[player].convoys[convoy].captainSalary = (rand() % 30) + 10;
}

unsigned requireProductionGood(size_t player, size_t branch, Goodtype t) {
	size_t i;
	if(!goodRequiredForProduction[t]) return 0;
	ptrdiff_t city = getCityIDFromBranch(branch, player);
	if(city == -1) return 0;
	// look if we have factories needing that good.
	for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
		if(Players[player].branchFactories[i] && Factories[Cities[city].industry[i]].consumation[t] > 0.0f) {
			return 1;
		}
	}
	return 0;
}

void sellWholeStock(size_t player, size_t branch, unsigned skiprequired) {
	size_t g;
	for (g = 1; g < GT_MAX; g++) {
		if (Players[player].branchStock[branch].stock[g] > 0.f) {
			if(!skiprequired || !requireProductionGood(player, branch, g))
				sell(Players[player].branchCity[branch], player, branch, -1, g, Players[player].branchStock[branch].stock[g], SELL_TO_CITY | SELL_FROM_STOCK);
		}
	}
}

unsigned convoysAvailable(size_t player, size_t branch) {
	size_t i;
	size_t city;
	city = getCityIDFromBranch(branch, player);
	for(i = 0; i < Players[player].numConvoys; i++) {
		if(Players[player].convoys[i].loc == SLT_CITY && Players[player].convoys[i].locCity == city)
			return 1;
	}
	return 0;
}

float calculateDistance(Coords* a, Coords* b) {
	// TODO do proper pathfinding
	float x = a->x - b->x;
	float y = a->y - b->y;
	return (float) sqrtf((x * x) + (y * y));
}

ptrdiff_t findNearestCityWithGood(size_t city, Goodtype neededgood) {
	float mindistance = 1000000000000.f;
	float distance;
	size_t g;
	for(g = 0; g < producerCount[neededgood]; g++) {
		distance = calculateDistance(&Cities[city].coords, &Cities[producers[neededgood][g]].coords);
		if(distance < mindistance)
			mindistance = distance;
	}
	for(g = 0; g < producerCount[neededgood]; g++) {
		distance = calculateDistance(&Cities[city].coords, &Cities[producers[neededgood][g]].coords);
		if(distance == mindistance) {
			return producers[neededgood][g];
		}
	}
	return -1;
}

void embark(Convoy* convoy, size_t to_city) {
	convoy->fromCity = convoy->locCity;
	assert(convoy->locCity != to_city);
	convoy->loc = SLT_SEA;
	convoy->locCity = to_city;
	convoy->coords = Cities[convoy->fromCity].coords;
	convoy->stepsdone = 0;
}

void land(Convoy* convoy) {
	convoy->loc = SLT_CITY;
	convoy->coords = Cities[convoy->locCity].coords;
}

float getConvoyMaxStorage(Convoy* c) {
	size_t s;
	float result = 0.f;
	for(s = 1; s < ST_MAX; s++) {
		result += shipProps[s].maxload * c->shipcounter.numShips[s];
	}
	return result;
}

// blindly moves the goods from/to a market to a convoy. no checks of any sort.
void moveGoodsConvoy(Convoy* c, Market* m, Goodtype g, float amount, unsigned fromConvoy) {
	if (fromConvoy) 
		amount = -amount;
	
	c->load.stock[g] += amount;
	c->totalload += amount;
	m->stock[g] -= amount;
}

float getSaneAmount(size_t player, size_t city, float amount, Goodtype g) {
	unsigned long long price;
	while(amount > 2.f && (price = calculatePrice(city, g, amount, 1)) && 
		((long long) ((float) price * amount) > Players[player].money 
			|| price > (goodcat_minprice[Goods[g].cat] * 2)
		) 
	) amount /= 2.f;
	return amount;
}

void purchaseFactories(size_t player, size_t city, Goodtype g, size_t amount) {
	long long price = 0;
	size_t industry_id = 0;
	ptrdiff_t branch_id = -1;
	size_t i;
	
	// check if the city can produce the good...
	for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
		if(Cities[city].industry[i] == g) {
			price = 1;
			break;
		}
	}
	if(!industry_id) 
		return;
	// check if the player has a branch there...
	for (i=0; i < Players[player].numBranches; i++) {
		if(Players[player].branchCity[i] == city) {
			branch_id = i;
			break;
		}
	}
	if(branch_id == -1) 
		return;
	
	while ((price = (long long) (Factories[i].buildcost * amount)) && price > Players[player].money)
		if(amount == 1)
			amount = 0;
		else amount = amount / 2;
		
	Players[player].money -= price;
	Players[player].branchFactories[branch_id][industry_id] += amount;
}

unsigned canProduce(size_t city, Goodtype g) {
	size_t i;
	for (i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
		if(Cities[city].industry[i] == g)
			return 1;
	}
	return 0;
}

void aiThink(void) {
	size_t p;
	size_t b, c;
	size_t g, x;
	size_t bought;
	ptrdiff_t city, shiploc;
	ptrdiff_t new_convoy;
	Notification* last;
	float freestorage = 0.f;
	size_t neededgood = GT_NONE;
	ptrdiff_t nearestcity;
	ptrdiff_t branchOfInterest;
	unsigned jumpedhere;
	size_t here;
	Player* dude;
	Convoy* convoy;
	unsigned jobdone;

	for(p = 0; p < numPlayers; p++) {
		dude = &Players[p];
		
		if(dude->type == PLT_CPU) {
			
			// go through notifications and setup a plan accordingly, or act directy.
			while((last = &dude->inbox.notifications[dude->inbox.last]) && last->nt != NT_NONE) {
				switch(last->nt) {
					case NT_NO_MONEY:
						for (b = 0; b < dude->numBranches; b++) {
							sellWholeStock(p, b, 0);
						}
						break;
					case NT_BAD_MOOD:
						b = last->val1;
						if(!(dude->plan & AIP_SATISFY_MOODS)) {
							if(dude->numConvoys == 0) {
								// cpu needs at least one convoy to organise goods
								bought = 0;
								for(g = 1; g < ST_MAX; g++) {
									if(Cities[dude->branchCity[b]].shipyard.availableShips.numShips[g])
										bought += buyShips(dude->branchCity[b], p, Cities[dude->branchCity[b]].shipyard.availableShips.numShips[g], g);
								}
								if(bought) {
									city = getCityIDFromBranch(b, p);
									assert(city != -1);
									shiploc = getShipLocationIDFromCity(city, p);
									assert(shiploc != -1);
									new_convoy = makeConvoy(city, shiploc, p, bought, ST_NONE);
									hireCrew(new_convoy, p, getMinCrew(new_convoy, p));
									hireCaptain(new_convoy, p);
								}
							}
							dude->plan |= AIP_SATISFY_MOODS;
							dude->plandata.moodybranch = b; // if theres more than one moody branch, the others have to wait...
						}
						break;
					case NT_OUT_OF_PRODUCTIONGOODS:
						dude->plan |= AIP_PURCHASE_PRODUCTION_GOODS;
						dude->plandata.branch_req_goods = last->val1;
						dude->plandata.req_production_good = last->val2;
						dude->plandata.amount_required = last->fval1;
						break;
					case NT_STOCK_FULL:
						b = last->val1;
						if(!convoysAvailable(p, b))
							sellWholeStock(p, b, 1);
						else 
							dude->plan |= AIP_SELL_GOODS_FROM_STOCK;
						break;
					default:
						break;
						
				}
				*last = emptyNotification;
				if(!dude->inbox.last)
					dude->inbox.last = MAX_NOTIFICATIONS;
				dude->inbox.last--;
			}
			
			if(dude->plan & AIP_SATISFY_MOODS) {
				// easy route: see if we got the stuff in our own stock and sell it to the city.
				jobdone = 0;
				for(g = 1; g < GT_MAX; g++) {
					if(Cities[dude->branchCity[dude->plandata.moodybranch]].market.stock[g] < 1.f) {
						if(dude->branchStock[dude->plandata.moodybranch].stock[g] > 0.f) {
							sell(dude->branchCity[dude->plandata.moodybranch], p, dude->plandata.moodybranch, -1, g, dude->branchStock[dude->plandata.moodybranch].stock[g], SELL_FROM_STOCK | SELL_TO_CITY);
							jobdone = 1;
						} else if (canProduce(dude->branchCity[dude->plandata.moodybranch], g)) {
							purchaseFactories(p, dude->branchCity[dude->plandata.moodybranch], g, 1);
						}
					}
				}
				if(jobdone) {
					dude->plan = dude->plan & ~AIP_SATISFY_MOODS;
				}
			}
			
			// iterate through convoys, if they're in a branch city put load into stock, otherwise sell to city and try to purchase required goods.
			for(c = 0; c < dude->numConvoys; c++) {
				
				convoy = &dude->convoys[c];
				
				if(convoy->loc == SLT_SEA) 
					continue;
				
				here = convoy->locCity; // location of the convoi we're dealing with.
				
				if(dude->numShipLocations) {
					for(x = 0; x < dude->numShipLocations; x++) {
						if(dude->singleShips[x].total && dude->shipLocations[x] == here)
							addToConvoy(convoy, x, p, dude->singleShips[x].total, ST_NONE);
					}
				}
				
				jumpedhere = 0;
				// check if the convoy is in a branch of ours
				for(b = 0; b < dude->numBranches; b++) {
					if(here == dude->branchCity[b]) {
						if(convoy->totalload > 0.f) {
							// try to put load into warehouse, or sell to city if no space.
							for(g = 1; g < GT_MAX; g++) {
								if(convoy->load.stock[g] > 0.f) {
									freestorage = getPlayerFreeBranchStorage(b, p);
									if((dude->plan & AIP_SATISFY_MOODS) && b == dude->plandata.moodybranch) {
										// sell everything to city.
										freestorage = 0.f;
										dude->plan = dude->plan & ~ AIP_SATISFY_MOODS;
									} else {
										if(freestorage < convoy->load.stock[g]) {
											sellWholeStock(p, b, 1);
											freestorage = getPlayerFreeBranchStorage(b, p);
										}
										if(freestorage > convoy->load.stock[g]) 
											freestorage = convoy->load.stock[g];
									}
									convoy->load.stock[g] -= freestorage;
									convoy->totalload -= freestorage;
									dude->branchStock[b].stock[g] += freestorage;
									if(convoy->load.stock[g] > 0.f)
										sell(dude->branchCity[b], p, b, c, g, convoy->load.stock[g], SELL_FROM_CONVOY | SELL_TO_CITY);
									
									if((dude->plan & AIP_PURCHASE_PRODUCTION_GOODS) && g == dude->plandata.req_production_good && b == dude->plandata.branch_req_goods)
										dude->plan = dude->plan & ~ AIP_PURCHASE_PRODUCTION_GOODS;
									
								}
							}
						}
						// now that the load is cleared, use the convoy for our plans...
						// in the order of importance.
						// #TAG1
						
						jump:
						
						branchOfInterest = -1;
						if(dude->plan & AIP_SATISFY_MOODS) {
							//check which good we need
							neededgood = GT_NONE;
							for(g = 1; g < GT_MAX; g++) {
								if(Cities[dude->branchCity[dude->plandata.moodybranch]].market.stock[g] < 1.f) {
									neededgood = g;
									break;
								}
							}
							if(neededgood == GT_NONE) {
								dude->plan = dude->plan & ~AIP_SATISFY_MOODS;
							} else {
								branchOfInterest = dude->plandata.moodybranch;
							}
						}
						if(dude->plan & AIP_SATISFY_MOODS || dude->plan & AIP_PURCHASE_PRODUCTION_GOODS) {
							if(branchOfInterest == -1) {
								branchOfInterest = dude->plandata.branch_req_goods;
								neededgood = dude->plandata.req_production_good;
							}
							
							//calculate daily required amount for production, and if convoy maxload is below that, buy additional ships
							if ((dude->plan & AIP_PURCHASE_PRODUCTION_GOODS) && getConvoyMaxStorage(convoy) < dude->plandata.amount_required)
								buyShips(here, p, 1, ST_A);
							
							if (jumpedhere) goto jump2;
							
							// the branch were the convoy is the branch which needs stuff.
							if(b == (size_t) branchOfInterest) {
								// choose a city which produces the required good, and send convoy there.
								if(producerCount[neededgood] == 0 || rand() % 2)
									nearestcity = findNearestCityWithGood(here, neededgood);
								else
									nearestcity = producers[neededgood][rand() % producerCount[neededgood]];
								if(nearestcity == -1) {
									fprintf(stderr, "warning, there's no supplier for good %d", (int) neededgood);
									return;
								} else if ((size_t) nearestcity == dude->branchCity[branchOfInterest]) {
									// it's not useful to send a ship to where it already is...
									for(x = 0; x < producerCount[neededgood]; x++) {
										if(producers[neededgood][x] != (size_t) nearestcity) {
											nearestcity = producers[neededgood][x];
											break;
										}
									}
									if ((size_t) nearestcity == dude->branchCity[branchOfInterest]) {
										// only city producing the good...so lets increase production
										purchaseFactories(p, dude->branchCity[branchOfInterest], neededgood, 1);
										dude->plan = dude->plan & ~ AIP_PURCHASE_PRODUCTION_GOODS;
										return;
									}
								}
								// plan to take regional wares with us - unfortunately that gives us soon moods
								/*
								freestorage = getConvoyMaxStorage(convoy) - convoy->totalload;
								if(freestorage > 0.f) {
									for(g = 0; g < CITY_MAX_INDUSTRYTYPES; g++) {
										if(dude->branchStock[b].stock[Cities[dude->branchCity[b]].industry[g]] > 0.f)
											moveGoodsConvoy(convoy, &dude->branchStock[b], Cities[dude->branchCity[b]].industry[g], freestorage / (float) CITY_MAX_INDUSTRYTYPES, 0);
									}
									freestorage = getConvoyMaxStorage(convoy) - convoy->totalload;
									if(freestorage > 0.f) {
										// see if we can take a regional good with us...
										if(Cities[nearestcity].industry[CITY_MAX_INDUSTRYTYPES -1] != Cities[dude->branchCity[b]].industry[CITY_MAX_INDUSTRYTYPES -1]
											&& Cities[dude->branchCity[b]].market.stock[CITY_MAX_INDUSTRYTYPES -1] > 0.f
											
										) {
											freestorage = getSaneAmount(p, dude->branchCity[b], freestorage, Cities[dude->branchCity[b]].industry[CITY_MAX_INDUSTRYTYPES -1]);
											sell(dude->branchCity[b], p, b, c, Cities[dude->branchCity[b]].industry[CITY_MAX_INDUSTRYTYPES -1], freestorage, SELL_FROM_CITY | SELL_TO_CONVOY);
										}
									}
								}
								*/
								embark(convoy, nearestcity);
							} else {
								// see if our branch here or the city has the good we want,
								
								if(dude->branchStock[b].stock[neededgood] > 0.f) {
									if(dude->branchStock[b].stock[neededgood] < freestorage)
										freestorage = dude->branchStock[b].stock[neededgood];
									else
										freestorage = getConvoyMaxStorage(convoy) - convoy->totalload;
									
									moveGoodsConvoy(convoy, &dude->branchStock[b], neededgood, freestorage, 0);
								}
								
								jump2:
								
								if(dude->money) {
									freestorage = getConvoyMaxStorage(convoy) - convoy->totalload;
									if(freestorage > 0.f && Cities[here].market.stock[neededgood] > 0.f) {
										if(Cities[here].market.stock[neededgood] < freestorage)
											freestorage = Cities[here].market.stock[neededgood];
										
										freestorage = getSaneAmount(p, here, freestorage, neededgood);
										// FIXME this loop might impact performance. also we should check if the price is much higher than the minprice.
										//while(freestorage >= 1.f && (price = calculatePrice(here, neededgood, freestorage, 1)) && (price * freestorage > dude->money)) freestorage /= 2.f;
										sell(here, p, b, c, neededgood, freestorage, SELL_FROM_CITY | SELL_TO_CONVOY);
									}
									embark(convoy, dude->branchCity[branchOfInterest]);
								}
							}
						} else {
							// no specific plan, buy a random good and gtfo
							// TODO
							
							
						}	
						break;
					}
				}

				if(jumpedhere) // remove once the TODO 5 lines above is removed
					continue;
				
				// if the convoy was in a branch of ours, we certainly sent it back to sea
				if(convoy->loc == SLT_SEA) 
					continue;
				// so we'll get here only if it is in a city without a branch.
				// means we sell our load, buy something useful and be gone.
				if(convoy->totalload > 0.f) {
					for(g = 1; g < GT_MAX; g++) {
						if(convoy->load.stock[g] > 0.f)
							sell(here, p, -1, c, g, convoy->load.stock[g], SELL_FROM_CONVOY | SELL_TO_CITY);
					}
				}
				jumpedhere = 1;
				goto jump;
			}
		}
	}
}

ShipTypes getSlowestShip(Convoy* convoy) {
	size_t slowest = 10000000;
	size_t s;
	ShipTypes result = ST_NONE;
	for(s = 1; s < ST_MAX; s++) {
		if(convoy->shipcounter.numShips[s] && shipProps[s].speed < slowest) {
			slowest = shipProps[s].speed;
			result = s;
		}
	}
	return result;
}
FILE* dbg;
void newSec(void) {
	
	size_t p, c, s;
	Player* player;
	Convoy* convoy;
	float milesperminute[ST_MAX];

	Coords diff, dir;
	ShipTypes slowestship;
	float dist;
	float stepsneeded;
	
	world.date += GAME_SPEED + (world.date % GAME_SPEED);
	
	if(world.date % world.secondsperminute == 0) {
		for(s = 1; s < ST_MAX; s++) {
			milesperminute[s] = ((float) shipProps[s].speed) / ((float) world.minutesperhour);
		}
		// calculate position of ships
		for(p = 0; p < numPlayers; p++) {
			player = &Players[p];
			for(c = 0; c < player->numConvoys; c++) {
				convoy = &player->convoys[c];
				if(convoy->loc == SLT_SEA) {
					slowestship = getSlowestShip(convoy);
					if(shipProps[slowestship].speed != 10)
						fprintf(dbg, "slowest: %d, speed: %d\n", slowestship, (int) shipProps[slowestship].speed);
					dist = calculateDistance(&Cities[convoy->fromCity].coords, &Cities[convoy->locCity].coords);
					fprintf(dbg, "dist from %s to %s: %f\n", Cities[convoy->fromCity].name->ptr, Cities[convoy->locCity].name->ptr, dist);
					stepsneeded = dist / milesperminute[slowestship];
					dir.x = Cities[convoy->fromCity].coords.x - Cities[convoy->locCity].coords.x;
					dir.y = Cities[convoy->fromCity].coords.y - Cities[convoy->locCity].coords.y;
					diff.x = dir.x / (dist / milesperminute[slowestship]); // movement on x axis per turn
					diff.y = dir.y / (dist / milesperminute[slowestship]); // -"- y
					convoy->stepsdone++;
					convoy->condition -= 0.01;
					if (convoy->stepsdone >= stepsneeded) {
						land(convoy);
					} else {
						convoy->coords.x = Cities[convoy->fromCity].coords.x - convoy->stepsdone * diff.x; 
						convoy->coords.y = Cities[convoy->fromCity].coords.y - convoy->stepsdone * diff.y; 
					}
				}
			}
		}
	}
	
	if(world.date % world._dayseconds == 0) {
		newDay();
	}

	if(world.date % (world.secondsperminute * (world.minutesperhour / 4)) == 0)
		aiThink(); // let the ai think only every 15 minutes... so that we can see its ships at the coast...

}

int microsleep(long microsecs) {
	struct timespec req, rem;
	req.tv_sec = microsecs / 1000000;
	req.tv_nsec = (microsecs % 1000000) * 1000;
	int ret;
	while((ret = nanosleep(&req, &rem)) == -1 && errno == EINTR) req = rem;
	return ret;	
}

int main(int argc, char** argv) {
	(void) argc;
	(void) argv;
	Gui gui_buf;
	gui = &gui_buf;

	dbg = fopen("debug.pato", "w");

	srand(time(NULL));
	initWorld();
	initConsumationTable();
	initCities();
	initBuildings();
	initPlayers();
	gui_init(gui);
	while(1) {
		// lets cheat... to speed up
		//world.date += world.secondsperminute - 1;
		newSec();
		microsleep(1000);
		if(gui_processInput(gui) == -1) break;
		if(world.date % (world.secondsperminute * GAME_SPEED) == 0)
			gui_repaint(gui);
	}	
	gui_free(gui);
	freeCities();
	freePlayers();
	return 0;
}

