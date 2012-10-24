#include "../lib/include/stringptr.h"
#include "gui_menu.h"

const stringptr* MENU_TEXT_BACK = SPLITERAL("..");

void menu_set_item(Menu* m, size_t item, stringptr *text, int abbrev, enum MenuactionType type, int target) {
	m->items[item].text = text;
	m->items[item].abbrev = abbrev;
	m->items[item].type = type;
	m->items[item].target.menu = target; // hack hack
}

Menu* menu_alloc(size_t numElems, enum Menupage id, enum Menupage parent) {
	Menu *ret;
	size_t itemCnt = numElems;
	if(parent != MP_NONE) itemCnt++;
	ret = malloc(sizeof(Menu) + (sizeof(Menuitem) * itemCnt));
	if(ret) {
		ret->id = id;
		ret->numElems = itemCnt;
		ret->activeItem = 0;
		if(itemCnt > numElems) {
			menu_set_item(ret, 0, (stringptr*) MENU_TEXT_BACK, '.', MAT_SHOW_MENU, parent);
		}
	}
	return ret;
}

void menu_free(Menu* m) {
	free(m);
}
