
#include <curses.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <locale.h>

#include "globals.h"
#include "hex.h"
#include "plugsel.h"
#include "console.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef linux
#include <unistd.h>
#endif

#define HAS_EXTENSION(f, e)			(strcmp(strrchr(f, '.'), e) == 0)
#define LITTLE_ENDIAN_PRINT_ARGS(x)	(x) & 0xFF, ((x) >> 8) & 0xFF, ((x) >> 16) & 0xFF, ((x) >> 24) & 0xFF	
#define BYTES_TO_U32(a,b,c,d)		(((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define LEFTROTATE(x, c)			(((x) << (c)) | ((x) >> (32 - (c))))

// general
int input();
void setupWindows();
void redraw();
void drawMenu();
unsigned char* loadFile(char* filename);
void saveFile(const char* filename);
int patchFile(const char* patchname);
void computeMD5();
void drawProgressBar(float p);

// plugin
void getPluginDir(char* pluginDir);
void loadPlugins();
void invokeActivePluginCallbacks(char* name);
Plugin* getPluginByLuaState(lua_State* L);
void registerLuaCallables(lua_State* L);
int l_activatePlugin(lua_State* L);
int l_setMargins(lua_State* L);
int l_getContents(lua_State* L);
int l_setContents(lua_State* L);
int l_log(lua_State* L);
int l_getFileSize(lua_State* L);
int l_getMD5(lua_State* L);

static char* newfile = "new";

static FileObject file = { 0 };
static Plugin plugins[16] = { 0 };
static int num_plugins = 0;
static int mode = HEX_EDITOR;
static char console[1024];
static int messages = 0;
static int quit = 0;

static WINDOW *win = NULL;
static WINDOW* statBar = NULL;
static WINDOW* menuBar = NULL;

int main(int argc, char* argv[]) {
	patchFailed = 0;

	// setup pdcurses, colors, windows
	initscr();
	noecho();
	cbreak();
	curs_set(0);

	start_color();

	COLOR_INIT_NORMAL();
	COLOR_INIT_CURSOR();
	COLOR_INIT_HIGHLIGHT();

	setupWindows();

	keypad(statBar, TRUE);
	nodelay(statBar, FALSE);

	keypad(win, TRUE);
	nodelay(win, TRUE);
	wbkgd(win, COLOR_NORMAL);
	wbkgd(statBar, COLOR_NORMAL);
	wbkgd(menuBar, COLOR_NORMAL);

	// init plugins
	consoleLoad();
	loadPlugins();

	// read arguments
	if (argc > 1) {
		if ((file.content = loadFile(argv[1]))) {
			file.loaded = 1;
			file.saved = 1;
			invokeActivePluginCallbacks("onFileLoad");
		}
		if ((argc > 2) && (file.loaded)) {
			if ((strcmp(argv[2], "-p") == 0) && (argc > 3))
				patchFailed = patchFile(argv[3]);
		}
	}

	// open up empty 1 byte file
	if (!file.loaded) {
		file.content = (byte*)malloc(50);
		file.size = 1;
		file.capacity = 50;
		file.saved = 0;
		file.loaded = 2;
		strcpy(file.name, newfile);
	}

	// prepare editor
	drawMenu();
	hexLoad(&file, win, statBar);
	hexDraw(win, statBar);
	plugSelLoad(plugins, num_plugins);

	// main loop
	int c;
	while (!quit) {
		c = input();
		if (mode == HEX_EDITOR) {
			c = hexInput(c, win, statBar);
		}
		else if (mode == PLUGIN_SELECTOR) {
			c = plugSelInput(c);
		}
		redraw(win, statBar, menuBar);
	}

	endwin();
	if (file.content)
		free(file.content);
	return 0;
}

void setupWindows() {
	int width, height;
	getmaxyx(stdscr, height, width);
	win = newwin(height - 2, width, 1, 0);
	menuBar = newwin(1, width, 0, 0);
	statBar = newwin(1, width, height - 1, 0);
}

void resize() {
	resize_term(0, 0);
	int width, height;
	getmaxyx(stdscr, height, width);
	wresize(win, height - 2, width);
	wresize(menuBar, 1, width);
	wresize(statBar, 1, width);
	mvwin(win, 1, 0);
	mvwin(menuBar, 0, 0);
	mvwin(statBar, height - 1, 0);
}

int input() {
	int ch = wgetch(win);
	switch (ch) {
		case KEY_F(12):
		{
			char answer[64] = { 'y' };
			if (!file.saved) {
				wclear(statBar);
				echo();
				nocbreak();
				mvwprintw(statBar, 0, 0, "File was modified. Quit anyway? (y or n) ");
				wscanw(statBar, "%s", answer);
				noecho();
				cbreak();
				wrefresh(statBar);
			}
			if (strcmp(answer, "y") == 0) {
				quit = 1;
			}
			break;
		}
		case 'l':
			mode = (mode + 1) % NUM_MODES;
			break;
			//save
		case 's':
			saveFile(file.name);
			break;
	}
	return ch;
}

void redraw() {
	if (is_termresized()) {
		resize();
	}
	if (mode == HEX_EDITOR) {
		hexDraw(win, statBar);
	}
	else if (mode == PLUGIN_SELECTOR) {
		plugSelDraw();
	}
	else if (mode == CONSOLE) {
		consoleDraw();
	}
	drawMenu(menuBar);
}

void saveFile(const char* filename) {
	FILE* fp = fopen(filename, "wb");
	if (fp) {
		invokeActivePluginCallbacks("onFileSave");
		fwrite(file.content, sizeof(char), file.size, fp);
		fclose(fp);
		file.saved = 1;
	}
	else {
		file.saved = 2;
	}
}

void computeMD5() {
	u32 s[] = { 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
				 5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
				 4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
				 6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21 };

	u32 K[] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
				 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
				 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
				 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
				 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
				 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
				 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
				 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
				 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
				 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
				 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
				 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
				 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
				 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
				 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
				 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

	u32 a0 = 0x67452301;
	u32 b0 = 0xefcdab89;
	u32 c0 = 0x98badcfe;
	u32 d0 = 0x10325476;

	size_t len = ((((file.size + 8) / 64) + 1) * 64) - 8;
	byte* message = calloc(len + 64, sizeof(byte));
	memcpy(message, file.content, file.size);

	message[file.size] = 0x80;
	u32 bitlen = file.size * 8;
	memcpy(message + len, &bitlen, 4);

	for (size_t chunk = 0; chunk < len; chunk += 64) {
		u32* M = (u32*)(message + chunk);

		u32 A = a0;
		u32 B = b0;
		u32 C = c0;
		u32 D = d0;

		for (size_t i = 0; i < 64; i++) {
			u32 F, g;
			if (i < 16) {
				F = (B & C) | ((~B) & D);
				g = i;
			}
			else if (i < 32) {
				F = (D & B) | ((~D) & C);
				g = (5 * i + 1) % 16;
			}
			else if (i < 48) {
				F = B ^ C ^ D;
				g = (3 * i + 5) % 16;
			}
			else {
				F = C ^ (B | (~D));
				g = (7 * i) % 16;
			}

			F += A + K[i] + M[g];

			A = D;
			D = C;
			C = B;
			B += LEFTROTATE(F, s[i]);
		}
		a0 += A;
		b0 += B;
		c0 += C;
		d0 += D;
	}

	file.MD5[0] = a0;
	file.MD5[1] = b0;
	file.MD5[2] = c0;
	file.MD5[3] = d0;
	free(message);
}

void drawMenu() {
	int w, h;
	getmaxyx(menuBar, h, w);
	(void)h;

	wclear(menuBar);
	mvwprintw(menuBar, 0, 2, "%s", file.name);
	mvwprintw(menuBar, 0, w - 19, "Size: %8X Byte", file.size);

	if (patchFailed) {
		if (patchFailed == 1)
			mvwprintw(menuBar, 0, w - 34, "Patch failed!");
		else if (patchFailed == 2)
			mvwprintw(menuBar, 0, w - 34, "File patched!");
	}
	else
		if (file.loaded == 1)
			mvwprintw(menuBar, 0, w - 34, "File loaded!");
		else if (file.loaded == 2)
			mvwprintw(menuBar, 0, w - 34, "New file!");

	wattron(menuBar, COLOR_CURSOR);
	if (file.saved == 0) {
		mvwaddch(menuBar, 0, 0, 'M');
	}
	else if (file.saved == 1) {
		mvwaddch(menuBar, 0, 0, 'S');
	}
	else if (file.saved == 2) {
		mvwaddch(menuBar, 0, 0, 'F');
	}
	wattroff(menuBar, COLOR_CURSOR);
	wrefresh(menuBar);
}

unsigned char* loadFile(char* filename) {
	FILE* fp;
	long lSize;
	unsigned char* buffer;
	size_t result;

	fp = fopen(filename, "rb");
	if (fp == NULL) { return NULL; }

	fseek(fp, 0, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	buffer = (unsigned char*)malloc(sizeof(unsigned char) * 2 * lSize);
	if (buffer == NULL) { return NULL; }

	result = fread(buffer, 1, lSize, fp);
	if (result != lSize) { return NULL; }

	strcpy(file.name, filename);
	file.size = lSize;
	file.capacity = lSize * 2;

	fclose(fp);
	return buffer;
}

int patchFile(const char* patchname) {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	registerLuaCallables(L);
	if (luaL_dofile(L, patchname) != LUA_OK) {
		cprintf("Error patching %s\n", lua_tostring(L, -1));
		lua_close(L);
		return 1;
	}
	lua_close(L);
	return 2;
}

void drawProgressBar(float p) {
	wtimeout(statBar, 1);
	mvwaddch(statBar, 0, 0, '[');
	float k;
	for (k = 0; k < p; k++) {
		mvwaddch(statBar, 0, (int)(k + 1), '#');
	}
	mvwaddch(statBar, 0, 101, ']');
	mvwprintw(statBar, 0, 103, "%.2f%%", p);
	wrefresh(statBar);
	wgetch(statBar);
	wtimeout(statBar, -1);
}

void getPluginDir(char* pluginDir) {
	// get plugin directory
	int bytes;
#ifdef _WIN32
	bytes = GetModuleFileName(NULL, pluginDir, 256);
#endif

#ifdef linux
	bytes = readlink("/proc/self/exe", pluginDir, 256);
	if (bytes >= 0) {
		pluginDir[bytes] = '\0';
	}
#endif
	int i;
	for (i = bytes - 1; i >= 0 && pluginDir[i] != '/' && pluginDir[i] != '\\'; i--);
	if (i > 0) {
		pluginDir[i + 1] = '\0';
	}
	strcat(pluginDir, "plugins/");
}

void loadPlugins() {
	DIR* dir;
	struct dirent *ent;

	char pluginDir[256];
	getPluginDir(pluginDir);

	// read plugin dir
	if (dir = opendir(pluginDir)) {
		while (ent = readdir(dir)) {
			if (ent->d_type == DT_REG && HAS_EXTENSION(ent->d_name, ".lua")) {
				Plugin* p = &plugins[num_plugins];
				num_plugins++;
				// initialize plugin
				char path[128] = { 0 };
				strcat(path, pluginDir);
				strcat(path, ent->d_name);
				strcpy(p->name, ent->d_name);
				p->L = luaL_newstate();
				luaL_openlibs(p->L);
				registerLuaCallables(p->L);
				if (luaL_dofile(p->L, path) != LUA_OK) {
					p->error = 1;
					cprintf("Error loading %s\n", lua_tostring(p->L, -1));
				}
				else {
					p->error = 0;
					cprintf("Loaded %s\n", ent->d_name);
				}
			}
		}
		closedir(dir);
	}
	else
		cprintf("Plugin directory \'%s\' not found!\n", pluginDir);
}

void invokeActivePluginCallbacks(char* name) {
	for (int i = 0; i < num_plugins; i++) {
		if (!plugins[i].error && plugins[i].active && lua_getglobal(plugins[i].L, name) == LUA_TFUNCTION) {
			if (lua_pcall(plugins[i].L, 0, 0, 0) != LUA_OK) {
				plugins[i].error = 1;
				plugins[i].active = 0;
				cprintf("Error in %s\n", lua_tostring(plugins[i].L, -1));
				cprintf("%s is removed from the list of active plugins\n", plugins[i].name);
			}
		}
	}
}

void registerLuaCallables(lua_State* L) {
	lua_register(L, "log", l_log);
	lua_register(L, "autoload", l_activatePlugin);
	lua_register(L, "margins", l_setMargins);
	lua_register(L, "getsize", l_getFileSize);
	lua_register(L, "getMD5", l_getMD5);

	luaL_newmetatable(L, "array");
	lua_pushcfunction(L, l_getContents);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_setContents);
	lua_setfield(L, -2, "__newindex");

	lua_newuserdata(L, sizeof(void*));
	luaL_getmetatable(L, "array");
	lua_setmetatable(L, -2);
	lua_setglobal(L, "file");
}

Plugin* getPluginByLuaState(lua_State* L) {
	for (int i = 0; i < num_plugins; i++)
		if (plugins[i].L == L)
			return &plugins[i];
	return NULL;
}

// LUA CALLABLES
int l_activatePlugin(lua_State* L)	// autoload()
{
	Plugin* p = getPluginByLuaState(L);
	p->active = 1;
	return 0;
}

int l_setMargins(lua_State* L)		// margins(l, r, t, b)
{
	margins[MARGIN_LEFT] = luaL_checknumber(L, 1);
	margins[MARGIN_RIGHT] = luaL_checknumber(L, 2);
	margins[MARGIN_TOP] = luaL_checknumber(L, 3);
	margins[MARGIN_BOTTOM] = luaL_checknumber(L, 4);
	return 0;
}

int l_getContents(lua_State* L)		// file[index]
{
	int index = luaL_checknumber(L, 2);
	if (index < file.size)
		lua_pushnumber(L, file.content[index]);
	else
		lua_pushnil(L);
	return 1;
}

int l_setContents(lua_State* L)		// file[index] = value
{
	int index = luaL_checknumber(L, 2);
	byte value = luaL_checknumber(L, 3);
	if (index < file.size) {
		file.content[index] = value;
		file.saved = 0;
	}
	return 0;
}

int l_log(lua_State* L) {
	char* c = luaL_checkstring(L, 1);
	cprintf("%s", c);
	return 0;
}

int l_getFileSize(lua_State* L) {
	lua_pushnumber(L, file.size);
	return 1;
}

int l_getMD5(lua_State* L) {
	computeMD5();
	char md5str[35];
	sprintf(md5str, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		LITTLE_ENDIAN_PRINT_ARGS(file.MD5[0]), LITTLE_ENDIAN_PRINT_ARGS(file.MD5[1]),
		LITTLE_ENDIAN_PRINT_ARGS(file.MD5[2]), LITTLE_ENDIAN_PRINT_ARGS(file.MD5[3]));
	lua_pushstring(L, md5str);
	return 1;
}