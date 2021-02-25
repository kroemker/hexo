#ifndef GLOBALS_H
#define GLOBALS_H

#include <curses.h>
#include <lua.h>
#include <lualib.h>
#include "ArrayList.h"
#include "HashMap.h"

#define TEXT_BUFFER_SIZE	64
#define MARGIN_TOP			0
#define MARGIN_LEFT			1
#define MARGIN_BOTTOM		2
#define MARGIN_RIGHT		3

#define VERTICAL			0
#define HORIZONTAL			1

#define COLOR_INIT_NORMAL()			init_pair(1, COLOR_WHITE, COLOR_BLACK)
#define COLOR_INIT_CURSOR()			init_pair(2, COLOR_WHITE, COLOR_BLUE)
#define COLOR_INIT_CALLER()			init_pair(3, COLOR_GREEN, COLOR_BLACK)
#define COLOR_INIT_ERROR()			init_pair(4, COLOR_WHITE, COLOR_RED)
#define COLOR_INIT_WARN()			init_pair(5, COLOR_BLACK, COLOR_YELLOW)
#define COLOR_INIT_HIGHLIGHT1()		init_pair(6, COLOR_BLACK, COLOR_YELLOW)
#define COLOR_INIT_HIGHLIGHT2()		init_pair(7, COLOR_WHITE, COLOR_GREEN)
#define COLOR_INIT_HIGHLIGHT3()		init_pair(8, COLOR_WHITE, COLOR_RED)

#define COLOR_NORMAL				COLOR_PAIR(1)
#define COLOR_CURSOR				COLOR_PAIR(2)
#define COLOR_CALLER				COLOR_PAIR(3)
#define COLOR_ERROR					COLOR_PAIR(4)
#define COLOR_WARN					COLOR_PAIR(5)
#define COLOR_HIGHLIGHT1			COLOR_PAIR(6)
#define COLOR_HIGHLIGHT2			COLOR_PAIR(7)
#define COLOR_HIGHLIGHT3			COLOR_PAIR(8)
#define COLOR_HIGHLIGHT(h)			COLOR_PAIR(5 + h)

#define HANDLER_C			0
#define HANDLER_LUA			1

#define MATCH_ANY_PATTERN			"%[^\n]%*c"

typedef unsigned char byte;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef void (*SubMenuItemHandler)();

#define NUM_MODES			3
enum {
	HEX_EDITOR,
	PLUGIN_SELECTOR,
	CONSOLE
};

typedef struct tHighlight {
	u64 pos;
	char mode;
}Highlight;

typedef struct tFileObject {
	char name[64];
	ArrayList content;
	int loaded;
	int saved;
	u32 MD5[4];
}FileObject;

typedef struct tPrintCommand {
	int x;
	int y;
	char content[128];
	int color;
} PrintCommand;

typedef struct tPluginWindow {
	int visible;
	WINDOW* cursesWindow;
	ArrayList printCommands;
} PluginWindow;

typedef struct tPlugin {
	char name[64];
	lua_State* L;
	int active;
	int error;
	int baseEditor;
	ArrayList windows;
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