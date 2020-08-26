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

typedef unsigned char byte;
typedef unsigned int u32;

#define NUM_MODES			3
enum
{
	HEX_EDITOR,
	PLUGIN_SELECTOR,
	CONSOLE
};

typedef struct tFileObject
{
	char name[64];
	byte* content;
	size_t size;
	size_t capacity;
	int loaded;
	int saved;
	u32 MD5[4];
}FileObject;

typedef struct tPlugin
{
	char name[64];
	lua_State* L;
	int active;
	int error;
	int baseEditor;
}Plugin;

int patchFailed;
int margins[4];

unsigned int readInt(unsigned char* data, unsigned int pos);
unsigned int hexCharToInteger(char hc);

#endif // GLOBALS_H
