#include <string.h>
#include <signal.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

#include "gui.h"
#include "../concol/console.h"
#include "../concol/ncconsole.h"
#include "../concol/ncconsole_chars.h"

const rgb_t MENU_BGCOLOR = RGB(30, 30, 120);
const rgb_t MENU_FONTCOLOR = RGB(255, 255, 255);
const rgb_t MENU_HIGHLIGHT_FONTCOLOR = RGB(220, 30, 0);
const rgb_t MAP_CITY_COLOR = RGB(255, 0, 0);
const rgb_t MAP_SHIP_COLOR = RGB(220, 220, 0);
rgb_t MAP_BG_COLOR = RGB(0, 0, 0);
const rgb_t TITLEBAR_BG_COLOR = RGB(66, 66, 66);
const rgb_t TITLEBAR_FONT_COLOR = RGB(255, 255, 255);

const stringptr* MENU_TEXT_BACK = SPLITERAL("..");

FILE* dbgf;

#ifndef IN_KDEVELOP_PARSER

const Menu menu_empty = {
	0, 0, 
	
	{
		
		{
			NULL,
			{
				MAT_NONE,
				MP_NONE,
				GP_NONE,
				0,0
			},
		},	
	}
};

Menu menu_main = {
	
	3, 0,

	{
		{
			SPLITERAL("Map"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_MAP,
				0,0
			},
			
		},
		
		{
		
			SPLITERAL("Players"),
			{
				MAT_SHOW_MENU,
				MP_PLAYERS,
				GP_NONE,
				0,0
			},
		},
		
		{

			SPLITERAL("Cities"),
			{
				MAT_SHOW_MENU,
				MP_CITIES,
				GP_NONE,
				0,0
			},
		},
		
		{
			NULL,
			{
				MAT_NONE,
				MP_NONE,
				GP_NONE,
				0,0
			},
		}	
		
	}
};


Menu menu_player = {
	3, 0, 
	{
		{
			SPLITERAL(".."), //MENU_TEXT_BACK,
			{
				MAT_SHOW_MENU,
				MP_PLAYERS,
				GP_NONE,
				0,0
			},
		},
		
		{
			SPLITERAL("Branches"),
			{
				MAT_SHOW_MENU,
				MP_BRANCHES,
				GP_NONE,
				0,0
			},
		},
		
		{
			SPLITERAL("Convoys"),
			{
				MAT_SHOW_MENU,
				MP_CONVOYS,
				GP_NONE,
				0,0
			}
			
		}
		
	}
};

Menu menu_player_main = {
	3, 0,

	{
		{
			SPLITERAL("Map"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_MAP,
				0,0
			},
			
		},
		
		{
			SPLITERAL("Branches"),
			{
				MAT_SHOW_MENU,
				MP_PLAYER_BRANCHES,
				GP_NONE,
				0,0
			},
		},
		
		{
			SPLITERAL("Convoys"),
			{
				MAT_SHOW_MENU,
				MP_PLAYER_CONVOYS,
				GP_NONE,
				0,0
			}
			
		}		
		
	}	
	
};

Menu menu_player_branch = {
	6, 0, 
	{
		{
			SPLITERAL(".."), //MENU_TEXT_BACK,
			{
				MAT_SHOW_MENU,
				MP_PLAYER_BRANCHES,
				GP_NONE,
				0,0
			},
		},
		
		{
			SPLITERAL("Trade"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},
		
		{
			SPLITERAL("Buy Buildings"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},
		
		{
			SPLITERAL("Buy Ships"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},

		{
			SPLITERAL("Build Ships"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},

		{
			SPLITERAL("Make Convoy"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},
	}
};

Menu menu_player_convoy = {
	2, 0, 
	{
		{
			SPLITERAL(".."), //MENU_TEXT_BACK,
			{
				MAT_SHOW_MENU,
				MP_PLAYER_CONVOYS,
				GP_NONE,
				0,0
			},
		},
		
		{
			SPLITERAL("Trade"),
			{
				MAT_SHOW_PAGE,
				MP_NONE,
				GP_TRADE,
				0,0
			},
		},
	}
};

#endif

Menu* p_menu_cities;
Menu* p_menu_players;
Menu* menus[MP_MAX];

Menu* menu_copy(Menu* source) {
	size_t i;
	Menu* result = malloc(sizeof(Menu) + (source->numElems * sizeof(Menuitem)));
	result->activeMenuEntry = source->activeMenuEntry;
	result->numElems = source->numElems;
	for(i = 0; i < result->numElems; i++) {
		result->items[i] = source->items[i];
	}
	return result;
}

inline int havePlayer(Gui* gui) {
	return (ptrdiff_t) gui->persona != -1;
}

void gui_adjust_areas(Gui* gui) {
	size_t menuwidth;
	gui->areas.toolbar.x = 0;
	gui->areas.toolbar.y = 0;
	gui->areas.toolbar.w = gui->w;
	gui->areas.toolbar.h = 1;
	
	if(gui->col == IC_MENU)
		menuwidth = MENU_WIDTH;
	else
		menuwidth = 0;
	
	gui->areas.menu.x = gui->w - menuwidth;
	gui->areas.menu.y = gui->areas.toolbar.h;
	gui->areas.menu.w = menuwidth;
	gui->areas.menu.h = gui->h - gui->areas.toolbar.h;
	
	gui->areas.page.x = 0;
	gui->areas.page.y = gui->areas.toolbar.h;
	gui->areas.page.w = gui->w - gui->areas.menu.w;
	gui->areas.page.h = gui->h - gui->areas.toolbar.h;
}

void createBranchMenu(Gui* gui) {
	size_t player = gui->menuParam;
	size_t b;
	gui->dynMenu = malloc(sizeof(Menu) + (sizeof(Menuitem) * (Players[player].numBranches + 1)));
	gui->dynMenu->numElems = Players[player].numBranches + 1;
	gui->dynMenu->activeMenuEntry = 0;	
	
	gui->dynMenu->items[0].text = (stringptr*) MENU_TEXT_BACK;
	gui->dynMenu->items[0].action.type = MAT_SHOW_MENU;
	
	if(havePlayer(gui))
		gui->dynMenu->items[0].action.targetMenu = MP_PLAYER_MAIN;
	else
		gui->dynMenu->items[0].action.targetMenu = MP_PLAYER;
	
	gui->dynMenu->items[0].action.param1 = 0;
	
	for (b = 0; b < Players[player].numBranches; b++) {
		gui->dynMenu->items[b + 1].text = Cities[Players[player].branchCity[b]].name;
		gui->dynMenu->items[b + 1].action.type = MAT_SHOW_PAGE;
		if(havePlayer(gui)) {
			gui->dynMenu->items[b + 1].action.type |= MAT_SHOW_MENU;
			gui->dynMenu->items[b + 1].action.targetMenu = MP_PLAYER_BRANCH;
		}	
		gui->dynMenu->items[b + 1].action.targetPage = GP_BRANCH;
		gui->dynMenu->items[b + 1].action.param1 = b;
		gui->dynMenu->items[b + 1].action.param2 = player;
	}
	if(havePlayer(gui))
		menus[MP_PLAYER_BRANCHES] = gui->dynMenu;
	else
		menus[MP_BRANCHES] = gui->dynMenu;
}

void createConvoyMenu(Gui* gui) {
	size_t player = gui->menuParam;
	size_t b;
	gui->dynMenu = malloc(sizeof(Menu) + (sizeof(Menuitem) * (Players[player].numConvoys + 1)));
	gui->dynMenu->numElems = Players[player].numConvoys + 1;
	gui->dynMenu->activeMenuEntry = 0;	
	
	gui->dynMenu->items[0].text = (stringptr*) MENU_TEXT_BACK;
	gui->dynMenu->items[0].action.type = MAT_SHOW_MENU;
	
	if(havePlayer(gui))
		gui->dynMenu->items[0].action.targetMenu = MP_PLAYER_MAIN;
	else
		gui->dynMenu->items[0].action.targetMenu = MP_PLAYER;
	
	gui->dynMenu->items[0].action.param1 = 0;
	
	for (b = 0; b < Players[player].numConvoys; b++) {
		gui->dynMenu->items[b + 1].text = Players[player].convoys[b].name;
		gui->dynMenu->items[b + 1].action.type = MAT_SHOW_PAGE;
		if(havePlayer(gui)) {
			gui->dynMenu->items[b + 1].action.type |= MAT_SHOW_MENU;
			gui->dynMenu->items[b + 1].action.targetMenu = MP_PLAYER_CONVOY;
		}
		gui->dynMenu->items[b + 1].action.targetPage = GP_CONVOY;
		gui->dynMenu->items[b + 1].action.param1 = b;
		gui->dynMenu->items[b + 1].action.param2 = player;
	}
	if(havePlayer(gui))
		menus[MP_PLAYER_CONVOYS] = gui->dynMenu;
	else
		menus[MP_CONVOYS] = gui->dynMenu;
}


void paintMenu(Gui* gui, Menupage page) {
	size_t x, y;
	size_t startx = gui->w - MENU_WIDTH;
	size_t maxx;
	size_t starty;
	
	if(gui->col != IC_MENU)
		return;
	
	if(menus[page] == NULL) {
		if(gui->dynMenu)
			free(gui->dynMenu);
		switch(page) {
			case MP_BRANCHES: case MP_PLAYER_BRANCHES:
				createBranchMenu(gui);
				break;
			case MP_CONVOYS: case MP_PLAYER_CONVOYS:
				createConvoyMenu(gui);
				break;
			default:
				break;
		}
	}
	
	console_setcolors(gui->term, MENU_BGCOLOR, MENU_BGCOLOR);

	starty = 1;
	
	for(y = starty; y < gui->h; y++) {
		for(x = startx; x < gui->w; x++) {
			console_goto(gui->term, x, y);
			console_addchar(gui->term, ' ', 0);
		}
	}
	
	startx++;
	
	for(y = starty; y < starty + menus[page]->numElems; y++) {
		rgb_t fg = (y == starty + (int) menus[page]->activeMenuEntry) ? MENU_HIGHLIGHT_FONTCOLOR : MENU_FONTCOLOR;
		console_setcolors(gui->term, MENU_BGCOLOR, fg);
		
		maxx = menus[page]->items[y - starty].text->size > MENU_WIDTH -2 ? MENU_WIDTH - 2 : menus[page]->items[y - starty].text->size;
		
		for(x = startx; x < startx + maxx; x++) {
			console_goto(gui->term, x, y);
			console_addchar(gui->term, menus[page]->items[y - starty].text->ptr[x - startx], (y == starty + menus[page]->activeMenuEntry) ? 0 : 0);
		}
	}
	// put cursor next to highlighted entry:
	//maxx = menus[page]->items[menus[page]->activeMenuEntry].text->size > MENU_WIDTH ? MENU_WIDTH : menus[page]->items[menus[page]->activeMenuEntry].text->size;
	//console_goto(gui->term, startx + maxx, starty + menus[page]->activeMenuEntry);
	
	gui->activeMenu = page;
}

void clearPage(Gui* gui) {
	size_t x, y;
	
	console_setcolors(gui->term, MAP_BG_COLOR, MAP_BG_COLOR);
	for(y = 1; y < gui->h; y++) {
		for(x = 0; x < gui->w - MENU_WIDTH; x++) {
			console_goto(gui->term, x, y);
			console_addchar(gui->term, ' ', 0);
		}
	}
}

void translateZoomedCoordsToGuiCoords(Gui* gui, size_t* x, size_t* y) {
	*x = *x - (gui->areas.map.x * gui->zoomFactor);
	*y = *y - (gui->areas.map.y * gui->zoomFactor);
}

void translateGameCoordsToZoomedCoords(Gui* gui, float orgX, float orgY, size_t* x, size_t* y) {
	size_t halfh = (gui->areas.page.h + (gui->areas.page.h % 2)) / 2;
	size_t halfw = (gui->areas.page.w + (gui->areas.page.w % 2)) / 2;
	
	// calculating the coordinates on our zoomed map, to have higher float precision
	*x = (size_t) (orgX * (float) gui->zoomFactor);
	*y = (size_t) (orgY * (float) gui->zoomFactor);
	*x += halfw;
	*y += halfh;

	fprintf(dbgf, "zoom: %d, guimap w: %d, h: %d\n", (int) gui->zoomFactor, (int)gui->map_resized->w, (int)gui->map_resized->h);
	//*y = (gui->map_resized->h) - *y;
}

inline int inVisibleZoomedMapArea(Gui* gui, size_t x, size_t y) {
	return	
		x > gui->areas.map.x * gui->zoomFactor &&
		x < (gui->areas.map.x * gui->zoomFactor) + gui->areas.page.w &&
		y > gui->areas.map.y * gui->zoomFactor &&
		y < (gui->areas.map.y * gui->zoomFactor) + gui->areas.page.h;
}

inline int blueish(rgb_t* col) {
	return col->b > col->r && col->b > col->g;
}
#define USE_CHECKER
void paintPage(Gui* gui, Guipage page) {
	size_t i, c;
	size_t x, y, starty;

	rgb_t* in;
	char* in2;

	switch(page) {
		case GP_MAP:
			// paint map
			console_setcolor(gui->term, 1, RGB(0, 80, 0));
			starty = gui->areas.page.y;
			for (y = 0; y < gui->areas.page.h; y++) {
				in2 = gui->map_resized->data + (((y + (gui->areas.map.y * gui->zoomFactor)) * (gui->map_resized->w))* 4);
				in2 += gui->areas.map.x * gui->zoomFactor * 4;
				in = (rgb_t*) in2;
				for (x = gui->areas.page.x; x < gui->areas.page.w; x++) {
					console_setcolor(gui->term, 0, *in);
					console_goto(gui->term, x, y + starty);
#ifdef USE_CHECKER					
					if (blueish(in))
						console_addchar(gui->term, ' ', 0);
					else
						console_addchar(gui->term, CC_CKBOARD, 0);
#else
						console_addchar(gui->term, ' ', 0);
#endif
					++in;
				}
			}
			
			//paint ships
			console_setcolors(gui->term, MAP_BG_COLOR, MAP_SHIP_COLOR);
			for(i = 0; i < numPlayers; i++) {
				for(c = 0; c < Players[i].numConvoys; c++) {
					if(Players[i].convoys[c].loc == SLT_SEA) {
						translateGameCoordsToZoomedCoords(gui, Players[i].convoys[c].coords.x, Players[i].convoys[c].coords.y, &x, &y);
						// now see if its in the visible area...
						if(inVisibleZoomedMapArea(gui, x, y)) {
							translateZoomedCoordsToGuiCoords(gui, &x, &y);
							console_goto(gui->term, x, y);
							console_addchar(gui->term, '+', 0);
						}
					}
				}
			}
			
			//paint cities
			console_setcolors(gui->term, RGB(0, 0, 0), MAP_CITY_COLOR);
			for(i = 0; i < numCities; i++) {
				translateGameCoordsToZoomedCoords(gui, Cities[i].coords.x, Cities[i].coords.y, &x, &y);
				fprintf(dbgf, "translated %f, %f to %d, %d\n", Cities[i].coords.x, Cities[i].coords.y, (int)x, (int)y);
				if(inVisibleZoomedMapArea(gui, x, y)) {
					translateZoomedCoordsToGuiCoords(gui, &x, &y);
					console_goto(gui->term, x, y);
					console_addchar(gui->term, 'X', 0);
					if(x + 2 + Cities[i].name->size < gui->areas.page.w)
						mvprintw(y, x + 2, "%s", Cities[i].name->ptr);
				}
			}
			
			break;
		case GP_CITY:
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			
			mvprintw(3, 3, "status for %s", Cities[gui->pageParam].name->ptr);
			y = 4;
			mvprintw(++y, 3, "money: %lld", Cities[gui->pageParam].money);
			mvprintw(++y, 3, "population:");
			for(i = 1; i < PT_MAX; i++) {
				mvprintw(++y, 3, "%s: count: %d, mood: %f", stringFromPopulationType(i)->ptr, (int) Cities[gui->pageParam].population[i], Cities[gui->pageParam].populationMood[i]);
			}
			y++;
			mvprintw(++y, 3, "market:");
			for(i = 1; i < GT_MAX; i++) {
				mvprintw(++y, 3, "%*s: %.3f", -11, stringFromGoodType(i)->ptr, Cities[gui->pageParam].market.stock[i]);
			}
			break;
		case GP_PLAYER:
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			x = 3;
			y = 3;
			mvprintw(y++, x, "status for %s [press 'i' to impersonate]", Players[gui->pageParam].name->ptr);
			mvprintw(y++, x, "money: %llu", Players[gui->pageParam].money);
			mvprintw(y++, x, "branches: %d", (int) Players[gui->pageParam].numBranches);
			for(i = 0; i < Players[gui->pageParam].numBranches; i++) {
				mvprintw(y++, x, "branch %s", Cities[Players[gui->pageParam].branchCity[i]].name->ptr);
				mvprintw(y++, x, "workers: %d", (int) Players[gui->pageParam].branchWorkers[i]);
				mvprintw(y++, x, "free stock: %d", (int) getPlayerFreeBranchStorage(i, gui->pageParam));
			}
			
			break;
		case GP_BRANCH:
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			x = 3;
			y = 3;
			mvprintw(y++, x, "Player %s, branch %s", Players[gui->pageParam2].name->ptr, Cities[Players[gui->pageParam2].branchCity[gui->pageParam]].name->ptr);
			mvprintw(y++, x, "money: %llu", Players[gui->pageParam2].money);
			mvprintw(y++, x, "workers: %d", (int) Players[gui->pageParam2].branchWorkers[gui->pageParam]);
			mvprintw(y++, x, "%s:", "factories");
			y++;
			c = 0;
			for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++)
				if(Players[gui->pageParam2].branchFactories[gui->pageParam][i]) 
					c++;
			if(c) {	
				for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
					mvprintw(y++, 3, "%*s: %d x %.3f/%.3f -> %.3f/%.3f", -11,
						stringFromGoodType(Cities[Players[gui->pageParam2].branchCity[gui->pageParam]].industry[i])->ptr,
						(int) Players[gui->pageParam2].branchFactories[gui->pageParam][i],
						(float) Players[gui->pageParam2].branchWorkers[gui->pageParam] / (float) c,
						(float) Players[gui->pageParam2].branchFactories[gui->pageParam][i] * factoryProps.maxworkersperfactory,
						 
						((float) Factories[Cities[Players[gui->pageParam2].branchCity[gui->pageParam]].industry[i]].yield * 
						(float) Players[gui->pageParam2].branchFactories[gui->pageParam][i]) *
						(
							
							((float) Players[gui->pageParam2].branchWorkers[gui->pageParam] / (float) c) /
							(factoryProps.maxworkersperfactory * Players[gui->pageParam2].branchFactories[gui->pageParam][i])
							
						),
						 
						(float) Factories[Cities[Players[gui->pageParam2].branchCity[gui->pageParam]].industry[i]].yield * 
						(int) Players[gui->pageParam2].branchFactories[gui->pageParam][i]
					);
				}
			}	

			y++;
			i = getPlayerFreeBranchStorage(gui->pageParam, gui->pageParam2);
			c = getPlayerMaxBranchStorage(gui->pageParam, gui->pageParam2);
			
			mvprintw(y++, x, "stock: %d/%d", (int) c - i, (int) c);
			y++;
			for(i = 1; i < GT_MAX; i++) {
				mvprintw(y++, 3, "%*s: %.3f", -11, stringFromGoodType(i)->ptr,  Players[gui->pageParam2].branchStock[gui->pageParam].stock[i]);
			}
			
			
			break;
			
		case GP_CONVOY:
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			x = 3;
			y = 3;
			mvprintw(y++, x, "Player %s, convoy %s", Players[gui->pageParam2].name->ptr, Players[gui->pageParam2].convoys[gui->pageParam].name ? Players[gui->pageParam2].convoys[gui->pageParam].name->ptr : "unnamed");
			mvprintw(y++, x, "sailors: %d", (int) Players[gui->pageParam2].convoys[gui->pageParam].numSailors);
			c = getConvoyMaxStorage(&Players[gui->pageParam2].convoys[gui->pageParam]);
			mvprintw(y++, x, "stock: %d/%d", (int) Players[gui->pageParam2].convoys[gui->pageParam].totalload, (int) c);
			if(Players[gui->pageParam2].convoys[gui->pageParam].loc == SLT_SEA) 
				mvprintw(y++, x, "location: sea, on its way to %s", Cities[Players[gui->pageParam2].convoys[gui->pageParam].locCity].name->ptr);
			else if(Players[gui->pageParam2].convoys[gui->pageParam].loc == SLT_CITY)
				mvprintw(y++, x, "location: port of %s", Cities[Players[gui->pageParam2].convoys[gui->pageParam].locCity].name->ptr);
			y++;
			if(Players[gui->pageParam2].convoys[gui->pageParam].totalload > 0.f) {
				for(i = 1; i < GT_MAX; i++) {
					if(Players[gui->pageParam2].convoys[gui->pageParam].load.stock[i] > 0.f)
						mvprintw(++y, 3, "%*s: stock %f", -11, stringFromGoodType(i)->ptr,  Players[gui->pageParam2].convoys[gui->pageParam].load.stock[i]);
				}
			}
			
			break;
			
		case GP_TRADE:
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			x = 3;
			y = 3;
			
			// FIXME add code
			
			break;
			
		default:
			break;
	}
	gui->activePage = page;
	
}

void paintTitlebar(Gui* gui) {
	int day, hour, min, sec;
	size_t x;
	console_setcolors(gui->term, TITLEBAR_BG_COLOR, TITLEBAR_BG_COLOR);
	console_goto(gui->term, 0, 0);
	
	for(x = 0; x < gui->w; x++) {
		console_printchar(gui->term, ' ', 0);
	}
	
	day = world.date / world._dayseconds;
	sec = world.date % world._dayseconds;
	
	//sec = world.hoursperday * world.minutesperhour * world.secondsperminute;
	
	hour = sec / (world.minutesperhour * world.secondsperminute);
	sec = sec - (hour * (world.minutesperhour * world.secondsperminute));
	min = sec / world.minutesperhour;

	console_setcolors(gui->term, TITLEBAR_BG_COLOR, TITLEBAR_FONT_COLOR);
	console_initoutput(gui->term);

	mvprintw(0, 0, "Day %d, %.2d:%.2d - Speed: %d", day, hour, min, GAME_SPEED);
	
}

void gui_repaint(Gui* gui) {
	if(!gui->_resize_in_progress) {
		paintMenu(gui, gui->activeMenu);
		paintPage(gui, gui->activePage);
		paintTitlebar(gui);
	}
}

void gui_resized(Gui* gui) {
	struct winsize termSize;
	int w, h;
	if(!gui->_resize_in_progress && ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &termSize) >= 0) {
		gui->_resize_in_progress = 1;
		resizeterm((int)termSize.ws_row, (int)termSize.ws_col);
		console_getbounds(gui->term, &w, &h);
		gui->w = w;
		gui->h = h;
		gui_adjust_areas(gui);
		//gui_resizeMap(gui, gui->zoomFactor);
		microsleep(10000);
		gui->_resize_in_progress = 0;
	}
}

void gui_resizeMap(Gui* gui, int scaleFactor) {
	rgb_t bgcol;
	int neww, newh;
	int halfh, halfw;
	Image* temp;
	if(gui->_resize_in_progress) 
		return;
	if(gui->map_resized != NULL) 
		free (gui->map_resized);
	
	gui->zoomFactor = scaleFactor;
	

	halfw = (gui->areas.page.w + (gui->areas.page.w % 2)) / 2;
	halfh = (gui->areas.page.h + (gui->areas.page.h % 2)) / 2;
	
	neww = (gui->map->w * scaleFactor) + (halfw * 2);
	newh = (gui->map->h * scaleFactor) + (halfh * 2);
	
	gui->map_resized = img_new(neww, newh);
	bgcol.asInt = *(int*) gui->map->data;
	
	img_fillcolor(gui->map_resized, bgcol);

	
	if(scaleFactor == 1)
		img_embed(gui->map_resized, gui->map, halfw, halfh);
	else {
		temp = img_scale(gui->map, scaleFactor, scaleFactor);
		img_embed(gui->map_resized, temp, halfw, halfh);
		free(temp);
	}
}

void gui_init(Gui* gui) {
	size_t i;
	int w, h;
	
	dbgf = fopen("debug.gui", "w");
	
	gui->term = &gui->term_struct.super;
	console_init(gui->term);
	
	gui->_resize_in_progress = 0;
	
	console_getbounds(gui->term, &w, &h);
	gui->w = w;
	gui->h = h;
	
	p_menu_cities = malloc(sizeof(Menu) + (sizeof(Menuitem) * (numCities + 1)));
	p_menu_cities->numElems = numCities + 1;
	p_menu_cities->activeMenuEntry = 0;
	
	p_menu_cities->items[0].text = (stringptr*) MENU_TEXT_BACK;
	p_menu_cities->items[0].action.type = MAT_SHOW_MENU;
	p_menu_cities->items[0].action.targetMenu = MP_MAIN;
	p_menu_cities->items[0].action.param1 = 0;
	
	for(i = 0; i < numCities; i++) {
		p_menu_cities->items[i + 1].text = Cities[i].name;
		p_menu_cities->items[i + 1].action.type = MAT_SHOW_PAGE;
		p_menu_cities->items[i + 1].action.targetPage = GP_CITY;
		p_menu_cities->items[i + 1].action.param1 = i;
	}
	p_menu_players = malloc(sizeof(Menu) + (sizeof(Menuitem) * (numPlayers + 1)));
	p_menu_players->numElems = numPlayers + 1;
	p_menu_players->activeMenuEntry = 0;

	p_menu_players->items[0].text = (stringptr*) MENU_TEXT_BACK;
	p_menu_players->items[0].action.type = MAT_SHOW_MENU;
	p_menu_players->items[0].action.targetMenu = MP_MAIN;
	p_menu_players->items[0].action.param1 = 0;
	
	
	for(i = 0; i < numPlayers; i++) {
		p_menu_players->items[i + 1].text = Players[i].name;
		p_menu_players->items[i + 1].action.type = MAT_SHOW_PAGE | MAT_SHOW_MENU;
		p_menu_players->items[i + 1].action.targetPage = GP_PLAYER;
		p_menu_players->items[i + 1].action.targetMenu = MP_PLAYER;
		p_menu_players->items[i + 1].action.param1 = i;
	}
	
	menus[MP_NONE] = (Menu*) &menu_empty;
	menus[MP_MAIN] = (Menu*) &menu_main;
	menus[MP_CITIES] = p_menu_cities;
	menus[MP_PLAYERS] = p_menu_players;
	menus[MP_PLAYER] = (Menu*) &menu_player;
	menus[MP_BRANCHES] = NULL;
	menus[MP_CONVOYS] = NULL;
	menus[MP_PLAYER_MAIN] = (Menu*) &menu_player_main;
	menus[MP_PLAYER_BRANCHES] = NULL;
	menus[MP_PLAYER_CONVOYS] = NULL;
	menus[MP_PLAYER_BRANCH] = (Menu*) &menu_player_branch;
	menus[MP_PLAYER_CONVOY] = (Menu*) &menu_player_convoy;
	
	gui->persona = (size_t) -1;
	
	gui->activePage = GP_MAP;
	gui->activeMenu = MP_MAIN;
	
	gui->col = IC_MENU;
	
	gui->map_resized = NULL;
	gui->map = getWorldImage();
	
	gui_adjust_areas(gui);
	
	gui->areas.map.x = (gui->areas.page.w + (gui->areas.page.w % 2)) / 2 ;
	gui->areas.map.y = (gui->areas.page.h + (gui->areas.page.h % 2)) / 2 ;
	
	gui_resizeMap(gui, 1);
	
	MAP_BG_COLOR = *(rgb_t*) gui->map->data;
	
	gui_repaint(gui);
	
	timeout(0); // set NCURSES getch to nonblocking
	nonl(); // get return key events
	
}

void gui_free(Gui* gui) {
	free(p_menu_cities);
	free(p_menu_players);
	if(gui->map) 
		free(gui->map);
	
	if(gui->map_resized)
		free(gui->map_resized);
	
	if(gui->dynMenu)
		free(gui->dynMenu);
	
	console_cleanup(gui->term);
}

int gui_processInput(Gui* gui) {
	size_t store;
	if(gui->_resize_in_progress) return 0;
	int c = console_getkey(gui->term);
	if(c == ERR) return 0;
	if(gui->col == IC_MENU) {
		switch(c) {
			case KEY_UP:
				if(menus[gui->activeMenu]->activeMenuEntry == 0)
					menus[gui->activeMenu]->activeMenuEntry = menus[gui->activeMenu]->numElems -1;
				else
					menus[gui->activeMenu]->activeMenuEntry--;
				break;
			case KEY_DOWN:	
				if(menus[gui->activeMenu]->activeMenuEntry == menus[gui->activeMenu]->numElems -1)
					menus[gui->activeMenu]->activeMenuEntry = 0;
				else
					menus[gui->activeMenu]->activeMenuEntry++;
				break;
			case 43: // plus on numeric pad
				GAME_SPEED = GAME_SPEED >= world.secondsperminute ? world.secondsperminute : GAME_SPEED * 2;
				break;
			case 45: // minus
				GAME_SPEED = GAME_SPEED > 2 ? GAME_SPEED / 2 : 1;
				break;
			case 'q': 
				return -1;
			case 'I':
				if(gui->activeMenu == MP_PLAYER) {
					gui->persona = menus[gui->activeMenu]->items[menus[gui->activeMenu]->activeMenuEntry].action.param2;
					Players[gui->persona].type = PLT_USER;
					gui->activeMenu = MP_PLAYER_MAIN;
				}
				break;
			case '\r':
				store = gui->activeMenu;
				if(menus[store]->items[menus[store]->activeMenuEntry].action.type & MAT_SHOW_MENU) {
					gui->menuParam = menus[store]->items[menus[store]->activeMenuEntry].action.param1;
					gui->activeMenu = menus[store]->items[menus[store]->activeMenuEntry].action.targetMenu;
					switch(gui->activeMenu) {
						case MP_BRANCHES: case MP_CONVOYS: case MP_PLAYER_BRANCHES: case MP_PLAYER_CONVOYS:
							menus[gui->activeMenu] = NULL; // flag to see we have to recreate the menu
						default:
							break;
					}
				}
				if(menus[store]->items[menus[store]->activeMenuEntry].action.type & MAT_SHOW_PAGE) {
					gui->activePage = menus[store]->items[menus[store]->activeMenuEntry].action.targetPage;
					gui->pageParam = menus[store]->items[menus[store]->activeMenuEntry].action.param1;
					gui->pageParam2 = menus[store]->items[menus[store]->activeMenuEntry].action.param2;
					paintPage(gui, gui->activePage);
				}
				break;
			case 9:
				gui->col = IC_PAGE;
				gui_adjust_areas(gui);
				gui_resizeMap(gui, gui->zoomFactor);
				break;
			default:
				console_setcolors(gui->term, TITLEBAR_BG_COLOR, TITLEBAR_FONT_COLOR);
				console_initoutput(gui->term);
				mvprintw(0, gui->w - MENU_WIDTH, "%d", c);
				break;
		}
		paintMenu(gui, gui->activeMenu);
	} else if (gui->col == IC_PAGE) {
		switch(c) {
			case 9: // tab
				gui->col = IC_MENU;
				gui_adjust_areas(gui);
				gui_resizeMap(gui, gui->zoomFactor);
				
				break;
			case 43: // plus on numeric pad
				if(gui->activePage == GP_MAP) {
					gui_resizeMap(gui, gui->zoomFactor * 2);
				}
				break;
			case 45: // minus
				if(gui->activePage == GP_MAP && gui->zoomFactor > 1) {
					gui_resizeMap(gui, gui->zoomFactor / 2);
				}
				break;
			case KEY_DOWN:
				if(gui->activePage == GP_MAP) {
					if((gui->areas.map.y + 8) < gui->map->h)
						gui->areas.map.y += 8;
					else 
						gui->areas.map.y = gui->map->h;
				}	
				break;
			case KEY_UP:
				if(gui->activePage == GP_MAP) {
					if(gui->areas.map.y > (8 * gui->zoomFactor)) 
						gui->areas.map.y -= 8;
					else	
						gui->areas.map.y = 0;
				}
				break;
			case KEY_RIGHT:
				if(gui->activePage == GP_MAP) {
					if((gui->areas.map.x + 8) < gui->map->w)
						gui->areas.map.x += 8;
					else 
						gui->areas.map.x = gui->map->w;
					}
				break;
			case KEY_LEFT:
				if(gui->activePage == GP_MAP) {
					if(gui->areas.map.x > (8 * gui->zoomFactor)) 
						gui->areas.map.x -= 8;
					else 
						gui->areas.map.x = 0;
				}
				break;
				
			default:
				break;
		}
		paintPage(gui, gui->activePage);
	}
	return 0;
}

