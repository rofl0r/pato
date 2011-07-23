#include "pato.h"
#include "imginterface.h"
#include "../concol/console.h"

#define MENU_WIDTH 20

typedef struct {
	size_t x;
	size_t y;
	size_t w;
	size_t h;
} Area;

typedef enum {
	IC_NONE,
	IC_MENU,
	IC_PAGE
} Inputcolumn;

typedef enum {
	GP_NONE = 0,
	GP_MAP,
	GP_CITY,
	GP_PLAYER,
	GP_BRANCH,
	GP_CONVOY,
	GP_TRADE,
} Guipage;

typedef enum {
	MP_NONE = 0,
	// spectator mode items
	MP_MAIN,
	MP_CITIES,
	MP_PLAYERS,
	MP_PLAYER,
	MP_BRANCHES,
	MP_CONVOYS,
	// player mode items
	MP_PLAYER_MAIN,
	MP_PLAYER_CONVOYS,
	MP_PLAYER_CONVOY,
	MP_PLAYER_BRANCHES,
	MP_PLAYER_BRANCH,
	MP_MAX
} Menupage;

typedef enum {
	MAT_NONE = 0,
	MAT_SHOW_MENU = 1,
	MAT_SHOW_PAGE = 2,
	MAT_EXEC = 4
} MenuactionType;

typedef struct {
	MenuactionType type;
	Menupage targetMenu;
	Guipage targetPage;
	size_t param1;
	size_t param2;
} Menuaction;

typedef struct {
	stringptr* text;
	Menuaction action;
} Menuitem;

typedef struct {
	size_t numElems;
	size_t activeMenuEntry;
	Menuitem items[];
} Menu;

typedef struct {
	Area toolbar;
	Area page;
	Area menu;
	Area map;
} Guiareas;

typedef struct {
	size_t persona;
	Guiareas areas;
	console term_struct;
	console* term;
	Guipage activePage;
	size_t pageParam;
	size_t pageParam2;
	size_t menuParam;
	Menupage activeMenu;
	Menu* dynMenu;
	Image* map;
	Image* map_resized;
	size_t w;
	size_t h;
	size_t zoomFactor;
	Inputcolumn col;
	int _resize_in_progress;
} Gui;

void gui_init(Gui* gui);
void gui_free(Gui* gui);
int gui_processInput(Gui* gui);
void gui_resized(Gui* gui);
void gui_repaint(Gui* gui);

