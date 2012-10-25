#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "gui.h"
#include "gui_menu.h"
#include "../concol/console.h"
#include "../concol/console_keys.h"


#define MVPRINTW(y, x, ...) console_printfxy(gui->term, (x), (y), __VA_ARGS__)

const rgb_t MENU_BGCOLOR = RGB(30, 30, 120);
const rgb_t MENU_FONTCOLOR = RGB(255, 255, 255);
const rgb_t MENU_HIGHLIGHT_FONTCOLOR = RGB(220, 30, 0);
const rgb_t MAP_CITY_COLOR = RGB(255, 0, 0);
const rgb_t MAP_SHIP_COLOR = RGB(220, 220, 0);
rgb_t MAP_BG_COLOR = RGB(0, 0, 0);
const rgb_t TITLEBAR_BG_COLOR = RGB(66, 66, 66);
const rgb_t TITLEBAR_FONT_COLOR = RGB(255, 255, 255);

FILE* dbgf;

static void paintPage(Gui* gui);
static void gui_resizeMap(Gui* gui, int scaleFactor);

const Menu menu_empty = {
	//.parent = MP_NONE,
	.id = MP_NONE,
	.numElems = 0,
	.activeItem = 0,
	.items = {},
};

Menu menu_main = {
	//.parent = MP_NONE,
	.id = MP_MAIN,
	.numElems = 3,
	.activeItem = 0,
	.items = {
		{
			.text = SPLITERAL("Map"),
			.abbrev = 'm',
			.type = MAT_SHOW_PAGE,
			.target = {GP_MAP},
		},
		
		{
			.text = SPLITERAL("Players"),
			.abbrev = 'p',
			.type = MAT_SHOW_MENU,
			.target = {MP_PLAYERS},
		},
		
		{
			.text = SPLITERAL("Cities"),
			.abbrev = 'c',
			.type = MAT_SHOW_MENU,
			.target = {MP_CITIES},
		},
	}
};

Menu menu_player = {
	//.parent = MP_PLAYERS,
	.id = MP_PLAYER,
	.numElems = 4,
	.activeItem = 0,
	.items = { 
		{
			.text = SPLITERAL(".."), //MENU_TEXT_BACK,
			.abbrev = '.',
			.type = MAT_SHOW_MENU,
			.target = {MP_PLAYERS},
		},
		{
			.text = SPLITERAL("Branches"),
			.abbrev = 'b',
			.type = MAT_SHOW_MENU,
			.target = {MP_BRANCHES},
		},
		{
			.text = SPLITERAL("Convoys"),
			.abbrev = 'c',
			.type = MAT_SHOW_MENU,
			.target = {MP_CONVOYS},
		},
		{
			.text = SPLITERAL("impersonate [!]"),
			.abbrev = 'i',
			.type = MAT_SHOW_MENU,
			.target = {MPP_MAIN},
		}
	}
};

Menu menu_player_main = {
	//.parent = MP_NONE,
	.id = MPP_MAIN,
	.numElems = 4,
	.activeItem = 0,
	.items = {
		{
			.text = SPLITERAL("Map"),
			.abbrev = 'm',
			.type = MAT_SHOW_PAGE,
			.target = {GP_MAP},
		},
		{
			.text = SPLITERAL("Branches"),
			.abbrev = 'b',
			.type = MAT_SHOW_MENU,
			.target =  {MPP_BRANCHES},
		},
		{
			.text = SPLITERAL("Convoys"),
			.abbrev = 'c',
			.type = MAT_SHOW_MENU,
			.target = {MPP_CONVOYS},
		},
		{
			.text = SPLITERAL("depersonate [!]"),
			.abbrev = 'i',
			.type = MAT_SHOW_MENU,
			.target = {MP_PLAYERS},
		},
	}
};

Menu menu_player_branch = {
	//.parent = MPP_BRANCHES,
	.id = MPP_BRANCH,
	.numElems = 6,
	.activeItem = 0,
	.items =  {
		{
			.text = SPLITERAL(".."), //MENU_TEXT_BACK,
			.abbrev = '.',
			.type = MAT_SHOW_MENU,
			.target = {MPP_BRANCHES},
		},
		{
			.text = SPLITERAL("Trade"),
			.abbrev = 't',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
		{
			.text = SPLITERAL("Buy Buildings"),
			.abbrev = 'b',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
		{
			.text = SPLITERAL("Buy Ships"),
			.abbrev = 's',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
		{
			.text = SPLITERAL("Build Ships"),
			.abbrev = 'r',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
		{
			.text = SPLITERAL("Make Convoy"),
			.abbrev = 'c',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
	}
};

Menu menu_player_convoy = {
	//.parent = MPP_CONVOYS,
	.id = MPP_CONVOY,
	.numElems = 2,
	.activeItem = 0,
	.items = {
		{
			.text = SPLITERAL(".."), //MENU_TEXT_BACK,
			.abbrev = '.',
			.type = MAT_SHOW_MENU,
			.target = {MPP_CONVOYS},
		},
		{
			.text = SPLITERAL("Trade"),
			.abbrev = 't',
			.type = MAT_SHOW_PAGE,
			.target = {GP_TRADE},
		},
	}
};

Menu* p_menu_cities;
Menu* p_menu_players;
Menu* menus[MP_MAX];

/*
Menu* menu_copy(Menu* source) {
	size_t i;
	Menu* result = malloc(sizeof(Menu) + (source->numElems * sizeof(Menuitem)));
	result->activeItem = source->activeItem;
	result->numElems = source->numElems;
	for(i = 0; i < result->numElems; i++) {
		result->items[i] = source->items[i];
	}
	return result;
}
*/


static inline int havePlayer(Gui* gui) {
	return (ptrdiff_t) gui->persona != -1;
}

static void gui_adjust_areas(Gui* gui) {
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

static Menu* createBranchesMenu(Gui* gui) {
	size_t b, player = gui->menudata.player_id;
	Menu* ret = menu_alloc(Players[player].numBranches, 
			       havePlayer(gui) ? MPP_BRANCHES : MP_BRANCHES, 
			       havePlayer(gui) ? MPP_MAIN : MP_PLAYER);
	
	if(ret) {
		for (b = 0; b < Players[player].numBranches; b++) {
			menu_set_item(ret, b + 1, Cities[Players[player].branchCity[b]].name, b < 10 ? '0' + b : 0,
				havePlayer(gui) ? MAT_SHOW_MENU : MAT_SHOW_PAGE, 
				havePlayer(gui) ?  MPP_BRANCH : GP_BRANCH
			);
		}
	}
	return ret;
}

static Menu* createConvoysMenu(Gui* gui) {
	size_t b, player = gui->menudata.player_id;
	Menu* ret = menu_alloc(Players[player].numConvoys, 
			       havePlayer(gui) ? MPP_CONVOYS : MP_CONVOYS, 
			       havePlayer(gui) ? MPP_MAIN : MP_PLAYER);
	
	if(ret) {
		for (b = 0; b < Players[player].numConvoys; b++) {
			menu_set_item(ret, b + 1, Players[player].convoys[b].name, b < 10 ? '0' + b : 0,
				      havePlayer(gui) ? MAT_SHOW_MENU : MAT_SHOW_PAGE,
				      havePlayer(gui) ? MPP_CONVOY : GP_CONVOY);
		}
	}
	return ret;
}

static Menu* createCitiesMenu(Gui* gui) {
	Menu* ret = menu_alloc(numCities, MP_CITIES, MP_MAIN);
	if(ret) {
		size_t i;
		for(i = 0; i < numCities; i++) {
			menu_set_item(ret, i + 1, Cities[i].name, i < 10 ? '0' + i : 0,
				      MAT_SHOW_PAGE, GP_CITY);
		}
	}
	return ret;
}

static Menu* createPlayersMenu(Gui *gui) {
	Menu *ret = menu_alloc(numPlayers, MP_PLAYERS, MP_MAIN);
	if(ret) {
		size_t i;
		for(i = 0; i < numPlayers; i++) {
			menu_set_item(ret, i + 1, Players[i].name, i < 10 ? '0' + i : 0,
				      /*MAT_SHOW_PAGE | */MAT_SHOW_MENU,
				      MP_PLAYER); //TODO this one should also show *page* GP_PLAYER
		}
	}
	return ret;
}

static void initMenus(Gui* gui) {
	p_menu_cities = createCitiesMenu(gui);
	p_menu_players = createPlayersMenu(gui);
	
	menus[MP_NONE] = (Menu*) &menu_empty;
	menus[MP_MAIN] = (Menu*) &menu_main;
	menus[MP_CITIES] = p_menu_cities;
	menus[MP_PLAYERS] = p_menu_players;
	menus[MP_PLAYER] = (Menu*) &menu_player;
	menus[MP_BRANCHES] = NULL;
	menus[MP_CONVOYS] = NULL;
	menus[MPP_MAIN] = (Menu*) &menu_player_main;
	menus[MPP_BRANCHES] = NULL;
	menus[MPP_CONVOYS] = NULL;
	menus[MPP_BRANCH] = (Menu*) &menu_player_branch;
	menus[MPP_CONVOY] = (Menu*) &menu_player_convoy;
}

static void clearMenu(Gui* gui) {
	size_t x, y, starty = 1, startx = gui->w - MENU_WIDTH;
	console_setcolors(gui->term, MENU_BGCOLOR, MENU_BGCOLOR);
	for(y = starty; y < gui->h; y++) {
		console_goto(gui->term, startx, y);
		for(x = startx; x < gui->w; x++) {
			console_printchar(gui->term, ' ', 0);
		}
	}
}

static void printMenuAbbrev(Gui* gui, int abbrev, size_t y) {
	console_goto(gui->term, (gui->w - MENU_WIDTH)+1, y);
	
	console_setcolors(gui->term, MENU_BGCOLOR, MENU_HIGHLIGHT_FONTCOLOR);
	console_printchar(gui->term, '[', 0);
	
	console_setcolors(gui->term, MENU_BGCOLOR, MENU_FONTCOLOR);
	console_printchar(gui->term, abbrev, 0);
	
	console_setcolors(gui->term, MENU_BGCOLOR, MENU_HIGHLIGHT_FONTCOLOR);
	console_printchar(gui->term, ']', 0);
}

static void paintMenu(Gui* gui) {
	if(gui->col != IC_MENU) return;
	clearMenu(gui);
	struct Menu *m = menus[gui->activeMenu];
	if(!m) m = gui->dynMenu;
	size_t x, y;
	size_t startx = (gui->w - MENU_WIDTH) + 1;
	size_t maxx, off = 3;
	size_t starty = 1;
	
	
	for(y = starty; y < starty + m->numElems; y++) {
		int abbrev = m->items[y - starty].abbrev;
		if(abbrev) printMenuAbbrev(gui, abbrev, y);

		rgb_t fg = (y == starty + (int) m->activeItem)
			? MENU_HIGHLIGHT_FONTCOLOR 
			: MENU_FONTCOLOR;
			
		
		maxx = (m->items[y - starty].text->size > MENU_WIDTH -2 -off) 
			? MENU_WIDTH - 2 -off 
			: m->items[y - starty].text->size;
		
		
		console_setcolors(gui->term, MENU_BGCOLOR, fg);
		
		for(x = startx + off; x < startx + off + maxx; x++) {
			console_goto(gui->term, x, y);
			console_addchar(gui->term, 
					m->items[y - starty].text->ptr[x - (startx + off)], 
					(y == starty + m->activeItem) ? 0 : 0);
		}
	}
	// put cursor next to highlighted entry:
	//maxx = menus[page]->items[menus[page]->activeItem].text->size > MENU_WIDTH ? MENU_WIDTH : menus[page]->items[menus[page]->activeItem].text->size;
	//console_goto(gui->term, startx + maxx, starty + menus[page]->activeItem);
}

static void freeDynMenu(Gui *gui) {
	if(gui->dynMenu) {
		menu_free(gui->dynMenu);
		gui->dynMenu = NULL;
	}
}

static void resetDynMenu(Gui *gui, struct Menu* newm) {
	freeDynMenu(gui);
	gui->dynMenu = newm;
}

static void createMenu(Gui* gui) {
	if(!menus[gui->activeMenu]) {
		switch(gui->activeMenu) {
			case MP_BRANCHES:
			case MPP_BRANCHES:
				resetDynMenu(gui, createBranchesMenu(gui));
				break;
			case MP_CONVOYS:
			case MPP_CONVOYS:
				resetDynMenu(gui, createConvoysMenu(gui));
				break;
			default:
				break;
		}
	}
}

static void doMenu(Gui* gui) {
	size_t a = gui->activeMenu;
	struct Menu* m = menus[a];
	if(!m) m = gui->dynMenu;
	struct Menuitem *mi = &m->items[m->activeItem];
	
	if(m->activeItem) {
		size_t *dest = NULL;
		switch(a) {
		case MPP_BRANCHES:
		case MP_BRANCHES:
				dest = &gui->menudata.branch_id;
			break;
		case MP_CITIES:
				dest = &gui->menudata.city_id;
			break;
		case MPP_CONVOYS:
		case MP_CONVOYS:
				dest = &gui->menudata.convoy_id;
			break;
		case MP_PLAYERS:
				dest = &gui->menudata.player_id;
			break;
		case MP_PLAYER:
			/* impersonate entry */
			if(mi->abbrev == 'i') {
				gui->persona = gui->menudata.player_id;
				Players[gui->persona].type = PLT_USER;
			}
			break;
		case MPP_MAIN:
			/* depersonate entry */
			if(mi->abbrev == 'i') {
				gui->persona = (size_t) -1;
				Players[gui->persona].type = PLT_CPU;
			}
			break;
			
		default :
			break;
		}
		if(dest) *dest =  m->activeItem - 1;
	}
	

	if(mi->type == MAT_SHOW_PAGE) {
		gui->activePage = mi->target.page;
		paintPage(gui);
	} else if (mi->type == MAT_SHOW_MENU) {
		gui->activeMenu = mi->target.menu;
		createMenu(gui);
	}
	
	/* special code for menus that set another menu AND a page.
	 * those have to point to menu code by default */
	a = gui->activeMenu;
	switch(a) {
		case MP_PLAYER:
			gui->activePage = GP_PLAYER;
			paintPage(gui);
			break;
		case MPP_CONVOY:
			gui->activePage = GP_CONVOY;
			paintPage(gui);
			break;
		case MPP_BRANCH:
			gui->activePage = GP_BRANCH;
			paintPage(gui);
			break;
		default:
			break;
	}
}

static int checkMenuAbbrev(Gui *gui, int c) {
	size_t i;
	Menu* m = menus[gui->activeMenu];
	if(!m) m = gui->dynMenu;
	Menuitem *mi;
	for(i = 0; i < m->numElems; i++) {
		mi = &m->items[i];
		if(c == mi->abbrev) {
			m->activeItem = i;
			doMenu(gui);
			return 1;
		}
	}
	return 0;
}

static int processMenuInput(Gui* gui, int c) {
	Menu* m = menus[gui->activeMenu];
	if(!m) m = gui->dynMenu;
	
	c &= CK_MASK;
	
	if(!checkMenuAbbrev(gui, c)) switch(c) {
		case CK_CURSOR_UP:
			if(m->activeItem == 0)
				m->activeItem = m->numElems -1;
			else
				m->activeItem--;
			break;
		case CK_CURSOR_DOWN:
			if(m->activeItem == m->numElems -1)
				m->activeItem = 0;
			else
				m->activeItem++;
			break;
		case CK_PLUS:
			GAME_SPEED = GAME_SPEED >= world.secondsperminute ? world.secondsperminute : GAME_SPEED * 2;
			break;
		case CK_MINUS:
			GAME_SPEED = GAME_SPEED > 2 ? GAME_SPEED / 2 : 1;
			break;
		case 'q':
			return -1;
		case CK_RETURN:
			doMenu(gui);
			break;
		case CK_TAB:
			gui->col = IC_PAGE;
			gui_adjust_areas(gui);
			gui_resizeMap(gui, gui->zoomFactor);
			break;
		default:
			console_setcolors(gui->term, TITLEBAR_BG_COLOR, TITLEBAR_FONT_COLOR);
			console_initoutput(gui->term);
			MVPRINTW(0, gui->w - MENU_WIDTH, "%d", c);
			break;
	}
	paintMenu(gui);
	return 0;
}


static void clearPage(Gui* gui) {
	size_t x, y;
	
	console_setcolors(gui->term, MAP_BG_COLOR, MAP_BG_COLOR);
	for(y = 1; y < gui->h; y++) {
		for(x = 0; x < gui->w - MENU_WIDTH; x++) {
			console_goto(gui->term, x, y);
			console_addchar(gui->term, ' ', 0);
		}
	}
}

static void translateZoomedCoordsToGuiCoords(Gui* gui, size_t* x, size_t* y) {
	*x = *x - (gui->areas.map.x * gui->zoomFactor);
	*y = *y - (gui->areas.map.y * gui->zoomFactor);
}

static void translateGameCoordsToZoomedCoords(Gui* gui, float orgX, float orgY, size_t* x, size_t* y) {
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

static inline int inVisibleZoomedMapArea(Gui* gui, size_t x, size_t y) {
	return
		x > gui->areas.map.x * gui->zoomFactor &&
		x < (gui->areas.map.x * gui->zoomFactor) + gui->areas.page.w &&
		y > gui->areas.map.y * gui->zoomFactor &&
		y < (gui->areas.map.y * gui->zoomFactor) + gui->areas.page.h;
}

static inline int blueish(rgb_t* col) {
	return col->b > col->r && col->b > col->g;
}
#define USE_CHECKER
/* this is responsible for painting the content of the left column;
 * map view, statistics and so on. */
static void paintPage(Gui* gui) {
	size_t i, c;
	size_t x, y, starty;
	enum Guipage page = gui->activePage;
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
						console_addchar(gui->term, CCT(cc_ckboard), 0);
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
						MVPRINTW(y, x + 2, "%s", Cities[i].name->ptr);
				}
			}
			
			break;
		case GP_CITY: {
			size_t cid = gui->menudata.city_id;
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			
			MVPRINTW(3, 3, "status for %s", Cities[cid].name->ptr);
			y = 4;
			MVPRINTW(++y, 3, "money: %lld", Cities[cid].money);
			MVPRINTW(++y, 3, "population:");
			for(i = 1; i < PT_MAX; i++) {
				MVPRINTW(++y, 3, "%s: count: %d, mood: %f", 
					 stringFromPopulationType(i)->ptr, 
					 (int) Cities[cid].population[i], 
					 Cities[cid].populationMood[i]);
			}
			y++;
			MVPRINTW(++y, 3, "market:");
			for(i = 1; i < GT_MAX; i++) {
				MVPRINTW(++y, 3, "%*s: %.3f", -11, 
					 stringFromGoodType(i)->ptr, 
					 Cities[cid].market.stock[i]);
			}
			break;
		} 
		case GP_PLAYER: {
			size_t pid = gui->menudata.player_id;
			Player* p = &Players[pid];
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			x = 3;
			y = 3;
			MVPRINTW(y++, x, "status for %s", p->name->ptr);
			MVPRINTW(y++, x, "money: %llu", p->money);
			MVPRINTW(y++, x, "branches: %d", (int) p->numBranches);
			for(i = 0; i < p->numBranches; i++) {
				MVPRINTW(y++, x, "branch %s", Cities[p->branchCity[i]].name->ptr);
				MVPRINTW(y++, x, "workers: %d", (int) p->branchWorkers[i]);
				MVPRINTW(y++, x, "free stock: %d", (int) getPlayerFreeBranchStorage(i, pid));
			}
			
			break;
		}
		case GP_BRANCH: {
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			size_t brid = gui->menudata.branch_id;
			size_t pid = gui->menudata.player_id;
			Player* p = &Players[pid];
			
			x = 3;
			y = 3;
			MVPRINTW(y++, x, "Player %s, branch %s", p->name->ptr, Cities[p->branchCity[brid]].name->ptr);
			MVPRINTW(y++, x, "money: %llu", p->money);
			MVPRINTW(y++, x, "workers: %d", (int) p->branchWorkers[brid]);
			MVPRINTW(y++, x, "%s:", "factories");
			y++;
			c = 0;
			for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++)
				if(p->branchFactories[brid][i]) 
					c++;
			if(c) {
				for(i = 0; i < CITY_MAX_INDUSTRYTYPES; i++) {
					MVPRINTW(y++, 3, "%*s: %d x %.3f/%.3f -> %.3f/%.3f", -11,
						stringFromGoodType(Cities[p->branchCity[brid]].industry[i])->ptr,
						(int) p->branchFactories[brid][i],
						(float) p->branchWorkers[brid] / (float) c,
						(float) p->branchFactories[brid][i] * factoryProps.maxworkersperfactory,
						 
						((float) Factories[Cities[p->branchCity[brid]].industry[i]].yield * 
						(float) p->branchFactories[brid][i]) *
						(
							
							((float) p->branchWorkers[brid] / (float) c) /
							(factoryProps.maxworkersperfactory * p->branchFactories[brid][i])
							
						),
						 
						(float) Factories[Cities[p->branchCity[brid]].industry[i]].yield * 
						(int) p->branchFactories[brid][i]
					);
				}
			}

			y++;
			i = getPlayerFreeBranchStorage(brid, pid);
			c = getPlayerMaxBranchStorage(brid, pid);
			
			MVPRINTW(y++, x, "stock: %d/%d", (int) c - i, (int) c);
			y++;
			for(i = 1; i < GT_MAX; i++) {
				MVPRINTW(y++, 3, "%*s: %.3f", -11, stringFromGoodType(i)->ptr,  p->branchStock[brid].stock[i]);
			}
			
			break;
		}
		case GP_CONVOY: {
			clearPage(gui);
			console_setcolors(gui->term, MAP_BG_COLOR, MENU_FONTCOLOR);
			console_initoutput(gui->term);
			size_t pid = gui->menudata.player_id;
			size_t cid = gui->menudata.convoy_id;
			Convoy *cnv = &Players[pid].convoys[cid];
			x = 3;
			y = 3;
			MVPRINTW(y++, x, "Player %s, convoy %s", Players[pid].name->ptr, cnv->name ? cnv->name->ptr : "unnamed");
			MVPRINTW(y++, x, "%s: %zu | %s: %zu | %s: %zu | %s: %zu", 
				 shipProps[ST_A].name->ptr, cnv->shipcounter.numShips[ST_A], 
				 shipProps[ST_B].name->ptr, cnv->shipcounter.numShips[ST_B], 
				 shipProps[ST_C].name->ptr, cnv->shipcounter.numShips[ST_C],
				 shipProps[ST_D].name->ptr, cnv->shipcounter.numShips[ST_D]);
			MVPRINTW(y++, x, "sailors: %d", (int) cnv->numSailors);
			c = getConvoyMaxStorage(cnv);
			MVPRINTW(y++, x, "stock: %d/%d", (int) cnv->totalload, (int) c);
			if(cnv->loc == SLT_SEA) 
				MVPRINTW(y++, x, "location: sea, on its way to %s", Cities[cnv->locCity].name->ptr);
			else if(cnv->loc == SLT_CITY)
				MVPRINTW(y++, x, "location: port of %s", Cities[cnv->locCity].name->ptr);
			y++;
			if(cnv->totalload > 0.f) {
				for(i = 1; i < GT_MAX; i++) {
					if(cnv->load.stock[i] > 0.f)
						MVPRINTW(++y, 3, "%*s: stock %f", -11, stringFromGoodType(i)->ptr,  cnv->load.stock[i]);
				}
			}
			
			break;
		}
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
}

static void paintTitlebar(Gui* gui) {
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

	MVPRINTW(0, 0, "Day %d, %.2d:%.2d - Speed: %d", day, hour, min, GAME_SPEED);
}

void gui_repaint(Gui* gui) {
	paintMenu(gui);
	paintPage(gui);
	paintTitlebar(gui);
	console_refresh(gui->term); //repaint
}

static void gui_resizeMap(Gui* gui, int scaleFactor) {
	rgb_t bgcol;
	int neww, newh;
	int halfh, halfw;
	Image* temp;

	if(gui->map_resized != NULL) {
		free (gui->map_resized);
		gui->map_resized = NULL;
	}
	
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

void gui_resized(Gui* gui) {
	int w, h;
	console_getbounds(gui->term, &w, &h);
	gui->w = w;
	gui->h = h;
	gui_adjust_areas(gui);
	gui_resizeMap(gui, gui->zoomFactor);
	//microsleep(10000);
	gui_repaint(gui);
}

#include "../concol/fonts/allfonts.h"
void gui_init(Gui* gui) {
	int w, h;
	
	dbgf = fopen("debug.gui", "w");
	
	gui->dynMenu = NULL;
	gui->map = NULL;
	gui->map_resized = NULL;
	
	gui->term = &gui->term_struct;
	console_init(gui->term);
	point res = {800, 600};
	console_init_graphics(&gui->term_struct, res, FONT);
	//kill(getpid(), SIGSTOP);
	
	console_getbounds(gui->term, &w, &h);
	gui->w = w;
	gui->h = h;
	
	initMenus(gui);
	
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
}

void gui_free(Gui* gui) {
	free(p_menu_cities);
	free(p_menu_players);
	if(gui->map) 
		free(gui->map);
	
	if(gui->map_resized)
		free(gui->map_resized);
	
	if(gui->dynMenu)
		menu_free(gui->dynMenu);
	
	console_cleanup(gui->term);
}

int gui_processInput(Gui* gui) {
	int c = console_getkey_nb(gui->term);
	if(c == CK_ERR || c == CK_MOUSE_EVENT) return 0;
	if(c == CK_RESIZE_EVENT) {
		gui_resized(gui);
		return 0;
	}
	
	switch(c & CK_MASK) {
		case CK_QUIT:
			return -1;
	}
	
	if(gui->col == IC_MENU) {
		if(processMenuInput(gui, c) == -1)
			return -1;
	} else if (gui->col == IC_PAGE) {
		switch(c & CK_MASK) {
			case CK_TAB:
				gui->col = IC_MENU;
				gui_adjust_areas(gui);
				gui_resizeMap(gui, gui->zoomFactor);
				
				break;
			case CK_PLUS:
				if(gui->activePage == GP_MAP) {
					gui_resizeMap(gui, gui->zoomFactor * 2);
				}
				break;
			case CK_MINUS:
				if(gui->activePage == GP_MAP && gui->zoomFactor > 1) {
					gui_resizeMap(gui, gui->zoomFactor / 2);
				}
				break;
			case CK_CURSOR_DOWN:
				if(gui->activePage == GP_MAP) {
					if((gui->areas.map.y + 8) < gui->map->h)
						gui->areas.map.y += 8;
					else 
						gui->areas.map.y = gui->map->h;
				}
				break;
			case CK_CURSOR_UP:
				if(gui->activePage == GP_MAP) {
					if(gui->areas.map.y > (8 * gui->zoomFactor)) 
						gui->areas.map.y -= 8;
					else	
						gui->areas.map.y = 0;
				}
				break;
			case CK_CURSOR_RIGHT:
				if(gui->activePage == GP_MAP) {
					if((gui->areas.map.x + 8) < gui->map->w)
						gui->areas.map.x += 8;
					else 
						gui->areas.map.x = gui->map->w;
					}
				break;
			case CK_CURSOR_LEFT:
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
		paintPage(gui);
	}
	//console_refresh(gui->term); //repaint
	gui_repaint(gui);
	return 0;
}

