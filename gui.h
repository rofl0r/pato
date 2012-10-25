#ifndef GUI_H
#define GUI_H

#include "pato.h"
#include "imginterface.h"
#include "gui_menu.h"
#include "../concol/console.h"

#define MENU_WIDTH 20

typedef struct Area {
	size_t x;
	size_t y;
	size_t w;
	size_t h;
} Area;

typedef enum Inputcolumn {
	IC_NONE,
	IC_MENU,
	IC_PAGE
} Inputcolumn;


typedef struct {
	Area toolbar;
	Area page;
	Area menu;
	Area map;
} Guiareas;

typedef struct {
	size_t persona;
	Guiareas areas;
	Console term_struct;
	Console* term;
	enum Guipage activePage;
	enum Menupage activeMenu;
	Menu* dynMenu;
	Image* map;
	Image* map_resized;
	size_t w;
	size_t h;
	size_t zoomFactor;
	Inputcolumn col;
	GuiMenu menudata;
} Gui;

void gui_init(Gui* gui);
void gui_free(Gui* gui);
int gui_processInput(Gui* gui);
void gui_resized(Gui* gui);
void gui_repaint(Gui* gui);
void gui_notify(Gui* gui, size_t player, Notification n);

//RcB: DEP "gui.c"

#endif
