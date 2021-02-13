#ifndef GLOBALS_H
#define GLOBALS_H

#include <curses.h>
#include <lua.h>
#include <lualib.h>

#define TEXT_BUFFER_SIZE	32
#define MARGIN_TOP			0
#define MARGIN_LEFT			1
#define MARGIN_BOTTOM		2
#define MARGIN_RIGHT		3

#define VERTICAL			0
#define HORIZONTAL			1

#define COLOR_INIT_NORMAL()			init_pair(1, COLOR_WHITE, COLOR_BLACK)
#define COLOR_INIT_CURSOR()			init_pair(2, COLOR_WHITE, COLOR_BLUE)
#define COLOR_INIT_HIGHLIGHT()		init_pair(3, COLOR_YELLOW, COLOR_BLACK)
#define COLOR_INIT_CALLER()			init_pair(4, COLOR_GREEN, COLOR_BLACK)

#define COLOR_NORMAL				COLOR_PAIR(1)
#define COLOR_CURSOR				COLOR_PAIR(2)
#define COLOR_HIGHLIGHT1			COLOR_PAIR(3)
#define COLOR_CALLER				COLOR_PAIR(4)

#define HANDLER_C			0
#define HANDLER_LUA			1

typedef unsigned char byte;
typedef unsigned int u32;

typedef void (*SubMenuItemHandler)();

#define NUM_MODES			3
enum {
	HEX_EDITOR,
	PLUGIN_SELECTOR,
	CONSOLE
};

typedef struct tFileObject {
	char name[64];
	byte* content;
	size_t size;
	size_t capacity;
	int loaded;
	int saved;
	u32 MD5[4];
}FileObject;

typedef struct tPlugin {
	char name[64];
	lua_State* L;
	int active;
	int error;
	int baseEditor;
}Plugin;

typedef struct tSubMenuItem {
	char name[64];
	int handlerType;
	SubMenuItemHandler cHandler;
	char luaHandlerName[64];
	lua_State* luaState;
	Plugin* plugin;
}SubMenuItem;


int patchFailed;
int margins[4];

unsigned int readInt(unsigned char* data, unsigned int pos);
unsigned int hexCharToInteger(char hc);

#endif // GLOBALS_H