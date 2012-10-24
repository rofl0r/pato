#ifndef GUI_MENU_H
#define GUI_MENU_H
#include "gui_page.h"

//RcB: DEP "gui_menu.c"

enum Menupage {
	MP_NONE = 0,
	// spectator mode items
	MP_MAIN,
	MP_CITIES,
	MP_PLAYERS,
	MP_PLAYER,
	MP_BRANCHES,
	MP_CONVOYS,
	// player mode items
	MPP_MAIN,
	MPP_CONVOYS,
	MPP_CONVOY,
	MPP_BRANCHES,
	MPP_BRANCH,
	MP_MAX
};

typedef enum MenuactionType {
	MAT_NONE = 0,
	MAT_SHOW_MENU = 1,
	MAT_SHOW_PAGE = 2,
} MenuactionType;

typedef union {
	enum Menupage menu;
	enum Guipage page;
} MenuDest;

typedef struct Menuitem {
	stringptr* text;
	int abbrev; // shortcut key to press instead of navigating and pressing enter.
	enum MenuactionType type;
	MenuDest target;
} Menuitem;

typedef struct Menu {
	//enum Menupage parent;
	enum Menupage id;
	size_t numElems;
	size_t activeItem;
	struct Menuitem items[];
} Menu;

typedef struct GuiMenu {
	size_t player_id;
	size_t branch_id;
	size_t convoy_id;
	size_t city_id;
} GuiMenu;

Menu* menu_alloc(size_t numElems, enum Menupage id, enum Menupage parent);
void menu_free(Menu* m);
void menu_set_item(Menu* m, size_t item, stringptr *text, int abbrev, enum MenuactionType type, int target);

#endif
