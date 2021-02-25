
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

#include "ArrayList.h"
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

#define GET_EXTENSION(f)			(strrchr(f, '.'))
#define HAS_EXTENSION(f, e)			(strcmp(GET_EXTENSION(f), e) == 0)
#define LITTLE_ENDIAN_PRINT_ARGS(x)	(x) & 0xFF, ((x) >> 8) & 0xFF, ((x) >> 16) & 0xFF, ((x) >> 24) & 0xFF	
#define BYTES_TO_U32(a,b,c,d)		(((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define LEFTROTATE(x, c)			(((x) << (c)) | ((x) >> (32 - (c))))

#define SUB_MENU_WIDTH_THRESHOLD	87

// general
void freePlugins();
int input();
void setupWindows();
void redraw();
void resize();
void tryQuit();
void toggleMode();
void initSubMenu();
void drawMenu();
void drawSubMenu();
int loadFile(char* filename);
void saveFile();
int patchFile(const char* patchname);
void computeMD5();
void drawProgressBar(float p);

// plugin
void getPluginDir(char* pluginDir);
void loadPlugins();
void invokePluginCallback(lua_State* L, char* name);
void invokeActivePluginCallbacks(char* name);
int tryInvokeLuaFunction(lua_State* L, char* name);
Plugin* getPluginByLuaState(lua_State* L);
void registerLuaCallables(lua_State* L);
int l_activatePlugin(lua_State* L);
int l_setMargins(lua_State* L);
int l_getContents(lua_State* L);
int l_setContents(lua_State* L);
int l_log(lua_State* L);
int l_getFileSize(lua_State* L);
int l_getMD5(lua_State* L);
int l_getFileName(lua_State* L);
int l_getFilePath(lua_State* L);
int l_getFileExtension(lua_State* L);
int l_registerMenuCallback(lua_State* L);
int l_showConsole(lua_State* L);
int l_getKey(lua_State* L);
int l_getTextInput(lua_State* L);
int l_newWindow(lua_State* L);
int l_setVisible(lua_State* L);
int l_printWindow(lua_State* L);
int l_clearWindow(lua_State* L);
int l_getEditorBounds(lua_State* L);
int l_getWindowBounds(lua_State* L);
int l_setWindowBounds(lua_State* L);
int l_getCursorWindowPosition(lua_State* L);
int l_getCursorFilePosition(lua_State* L);
int l_setCursorFilePosition(lua_State* L);
int l_mark(lua_State* L);
int l_unmark(lua_State* L);

static char* newfile = "new";

static FileObject file = { 0 };
static ArrayList plugins;
static int mode = HEX_EDITOR;
static char console[1024];
static int messages = 0;
static int quit = 0;
static int sub_menu_orientation = VERTICAL;
static int sub_menu_cursor = 0;
static ArrayList subMenuItems;
static char lastPressedKey[2] = { 0 };

static WINDOW* win = NULL;
static WINDOW* menuBar = NULL;
static WINDOW* subMenu = NULL;
static WINDOW* statBar = NULL;

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
	COLOR_INIT_CALLER();
	COLOR_INIT_ERROR();
	COLOR_INIT_WARN();
	COLOR_INIT_HIGHLIGHT1();
	COLOR_INIT_HIGHLIGHT2();
	COLOR_INIT_HIGHLIGHT3();

	setupWindows();

	keypad(statBar, TRUE);
	nodelay(statBar, FALSE);

	keypad(win, TRUE);
	nodelay(win, TRUE);
	wbkgd(win, COLOR_NORMAL);
	wbkgd(statBar, COLOR_NORMAL);
	wbkgd(menuBar, COLOR_NORMAL);

	initSubMenu();

	// init plugins
	consoleLoad();
	loadPlugins();

	// read arguments
	if (argc > 1) {
		if ((loadFile(argv[1]))) {
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
		byte data = 0;
		file.content = ArrayList_New(50, sizeof(byte));
		ArrayList_Add(&file.content, &data);
		file.saved = 0;
		file.loaded = 2;
		strcpy(file.name, newfile);
	}

	// prepare editor
	drawMenu();
	drawSubMenu();
	hexLoad(&file, win, statBar);
	hexDraw(win, statBar);
	plugSelLoad(&plugins);

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

		if (c != -1) {
			lastPressedKey[0] = c;
			invokeActivePluginCallbacks("onInput");
			redraw();
		}
	}

	invokeActivePluginCallbacks("onClose");
	endwin();
	ArrayList_Delete(&file.content);
	ArrayList_Delete(&subMenuItems);
	ArrayList_Delete(&highlights);
	return 0;
}

void freePlugins() {
	for (int i = 0; i < subMenuItems.size; i++) {
		SubMenuItem* smi = ArrayList_Get(&subMenuItems, i);
		if (smi->handlerType == HANDLER_LUA) {
			ArrayList_Remove(&subMenuItems, i);
			i--;
		}
	}

	for (int i = 0; i < plugins.size; i++) {
		Plugin* p = ArrayList_Get(&plugins, i);
		ArrayList_Delete(&p->windows);
	}
	ArrayList_Delete(&plugins);
}

void setupWindows() {
	int w, h;
	getmaxyx(stdscr, h, w);

	int main_x = 0;
	int main_y = 1;
	if (w > SUB_MENU_WIDTH_THRESHOLD) {
		sub_menu_orientation = VERTICAL;
		main_x += 20;
		subMenu = newwin(h - 2, main_x, 1, 0);
	}
	else {
		sub_menu_orientation = HORIZONTAL;
		main_y += 4;
		subMenu = newwin(main_y - 1, w, 1, 0);
	}

	win = newwin(h - 1 - main_y, w - main_x, main_y, main_x);
	menuBar = newwin(1, w, 0, 0);
	statBar = newwin(1, w, h - 1, 0);
}

void resize() {
	int w, h;
	if (resize_term(0, 0) == ERR) {
		return;
	}
	getmaxyx(stdscr, h, w);

	int main_x = 1;
	int main_y = 1;
	if (w > SUB_MENU_WIDTH_THRESHOLD) {
		sub_menu_orientation = VERTICAL;
		main_x += 20;
		wresize(subMenu, h - 2, main_x - 1);
	}
	else {
		sub_menu_orientation = HORIZONTAL;
		main_y += 4;
		wresize(subMenu, main_y - 1, w);
	}
	mvwin(subMenu, 1, 0);

	wresize(win, h - 1 - main_y, w);
	mvwin(win, main_y, main_x);

	wresize(menuBar, 1, w);
	mvwin(menuBar, 0, 0);

	wresize(statBar, 1, w);
	mvwin(statBar, h - 1, 0);
}

int input() {
	int ch = wgetch(win);
	switch (ch) {
	case KEY_F(5):
		freePlugins();
		loadPlugins();
		break;
	case KEY_RESIZE:
		resize();
		break;
	case KEY_F(12):
		tryQuit();
		break;
	case 'l':
		toggleMode();
		break;
	case 's':
		saveFile();
		break;
	case KEY_NPAGE:
		sub_menu_cursor = (sub_menu_cursor + 1) % subMenuItems.size;
		break;
	case KEY_PPAGE:
		sub_menu_cursor = sub_menu_cursor == 0 ? subMenuItems.size - 1 : (sub_menu_cursor - 1) % subMenuItems.size;
		break;
	case '\n':
	{
		if (mode != HEX_EDITOR) {
			break;
		}
		SubMenuItem* smi = ArrayList_Get(&subMenuItems, sub_menu_cursor);
		if (smi->handlerType == HANDLER_C) {
			smi->cHandler();
		}
		else if (smi->handlerType == HANDLER_LUA) {
			invokePluginCallback(smi->luaState, smi->luaHandlerName);
		}
		break;
	}
	}
	return ch;
}

void drawPluginWindows() {
	for (int i = 0; i < plugins.size; i++) {
		Plugin* p = ArrayList_Get(&plugins, i);
		for (int w = 0; w < p->windows.size; w++) {
			PluginWindow* pw = ArrayList_Get(&p->windows, w);
			if (pw->visible) {
				wclear(pw->cursesWindow);
				for (int k = 0; k < pw->printCommands.size; k++) {
					PrintCommand* pc = ArrayList_Get(&pw->printCommands, k);
					wattron(pw->cursesWindow, COLOR_PAIR(pc->color));
					mvwprintw(pw->cursesWindow, pc->y, pc->x, "%s", pc->content);
					wattroff(pw->cursesWindow, COLOR_PAIR(pc->color));
				}
				box(pw->cursesWindow, 0, 0);
				wrefresh(pw->cursesWindow);
			}
		}
	}
}

void redraw() {
	if (mode == HEX_EDITOR) {
		hexDraw(win, statBar);
		drawPluginWindows();
		drawSubMenu();
	}
	else if (mode == PLUGIN_SELECTOR) {
		plugSelDraw();
	}
	else if (mode == CONSOLE) {
		consoleDraw();
	}
	drawMenu();
}

void initSubMenu() {
	subMenuItems = ArrayList_New(6, sizeof(SubMenuItem));

	SubMenuItem saveMenuItem = { .name = "Save", .handlerType = HANDLER_C, .cHandler = saveFile, .plugin = NULL };
	SubMenuItem pluginsMenuItem = { .name = "Plugins", .handlerType = HANDLER_C, .cHandler = toggleMode, .plugin = NULL };
	SubMenuItem quitMenuItem = { .name = "Quit", .handlerType = HANDLER_C, .cHandler = tryQuit, .plugin = NULL };
	ArrayList_Add(&subMenuItems, &saveMenuItem);
	ArrayList_Add(&subMenuItems, &pluginsMenuItem);
	ArrayList_Add(&subMenuItems, &quitMenuItem);
}

void tryQuit() {
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
}

void toggleMode() {
	mode = (mode + 1) % NUM_MODES;
}

void saveFile() {
	FILE* fp = fopen(file.name, "wb");
	if (fp) {
		invokeActivePluginCallbacks("onFileSave");
		fwrite(file.content.ptr, sizeof(char), file.content.size, fp);
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

	size_t len = ((((file.content.size + 8) / 64) + 1) * 64) - 8;
	byte* message = calloc(len + 64, sizeof(byte));
	memcpy(message, file.content.ptr, file.content.size);

	message[file.content.size] = 0x80;
	u32 bitlen = file.content.size * 8;
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
	mvwprintw(menuBar, 0, w - 19, "Size: %8X Byte", file.content.size);

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

void drawSubMenu() {
	int w, h;
	wclear(subMenu);

	getmaxyx(subMenu, h, w);
	(void)h;

	int x = 1, y = 0;
	for (int i = 0; i < subMenuItems.size && y < h - 1; i++) {
		SubMenuItem* smi = ArrayList_Get(&subMenuItems, i);
		if (smi->plugin != NULL && !smi->plugin->active) {
			continue;
		}

		int len = strlen(smi->name);

		if (sub_menu_cursor == i) {
			wattron(subMenu, COLOR_CURSOR);
		}

		if (sub_menu_orientation == VERTICAL) {
			y++;
		}
		else if (sub_menu_orientation == HORIZONTAL) {
			if (x + len + 4 < w - 1) {
				x = 0;
				y++;
			}
			waddch(subMenu, ' ');
			waddch(subMenu, ' ');
		}


		mvwprintw(subMenu, y, x, "%s", smi->name);

		if (sub_menu_orientation == VERTICAL) {
			for (int j = len; j < w - 1; j++) {
				waddch(subMenu, ' ');
			}
		}
		else if (sub_menu_orientation == HORIZONTAL) {
			waddch(subMenu, ' ');
			waddch(subMenu, ' ');
			x += len + 4;
		}

		if (sub_menu_cursor == i) {
			wattroff(subMenu, COLOR_CURSOR);
		}
	}

	box(subMenu, 0, 0);
	wrefresh(subMenu);
}

int loadFile(char* filename) {
	FILE* fp;
	long size;
	byte* buffer;
	size_t result;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		return 0; 
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	buffer = malloc(sizeof(byte) * size * 2);
	if (buffer == NULL) { 
		return 0; 
	}

	result = fread(buffer, 1, size, fp);
	if (result != size) {
		return 0;
	}

	file.content = ArrayList_NewFromArray(buffer, size, sizeof(byte));
	strcpy(file.name, filename);

	free(buffer);
	fclose(fp);
	return 1;
}

int patchFile(const char* patchname) {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	registerLuaCallables(L);
	if (luaL_dofile(L, patchname) != LUA_OK) {
		cerrorf("Patch", "Error patching %s\n", lua_tostring(L, -1));
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

	plugins = ArrayList_New(6, sizeof(Plugin));
	// read plugin dir
	if (dir = opendir(pluginDir)) {
		while (ent = readdir(dir)) {
			if (ent->d_type == DT_REG && HAS_EXTENSION(ent->d_name, ".lua")) {
				Plugin p;
				// initialize plugin
				char path[128] = { 0 };
				p.active = 0;
				strcat(path, pluginDir);
				strcat(path, ent->d_name);
				strcpy(p.name, ent->d_name);
				p.L = luaL_newstate();
				luaL_openlibs(p.L);
				registerLuaCallables(p.L);
				p.windows = ArrayList_New(1, sizeof(PluginWindow));
				ArrayList_Add(&plugins, &p);
				Plugin* pptr = ArrayList_GetLast(&plugins);
				if (luaL_dofile(p.L, path) != LUA_OK) {
					pptr->error = 1;
					cerrorf(p.name, "Error loading %s\n", lua_tostring(p.L, -1));
				}
				else {
					pptr->error = 0;
					clogf(p.name, "Loaded %s\n", ent->d_name);
				}
			}
		}
		closedir(dir);
	}
	else {
		cwarnf("Plugin Loader", "Plugin directory \'%s\' not found!\n", pluginDir);
	}
}

void invokePluginCallback(lua_State* L, char* name) {
	if (lua_getglobal(L, name) == LUA_TFUNCTION) {
		lua_pcall(L, 0, 0, 0);
	}
}

void invokeActivePluginCallbacks(char* name) {
	for (int i = 0; i < plugins.size; i++) {
		Plugin* curr = ArrayList_Get(&plugins, i);
		if (!curr->error && curr->active) {
			if (!tryInvokeLuaFunction(curr->L, name)) {
				curr->error = 1;
				curr->active = 0;
				cerrorf(curr->name, "Error in %s\n", lua_tostring(curr->L, -1));
				cerrorf(curr->name, "%s is removed from the list of active plugins\n", curr->name);
			}
		}
	}
}

int tryInvokeLuaFunction(lua_State* L, char* name) {
	if (lua_getglobal(L, name) == LUA_TFUNCTION) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			return 0;
		}
	}
	else {
		lua_pop(L, 1);
	}
	return 1;
}

void registerLuaCallables(lua_State* L) {
	lua_register(L, "log", l_log);
	lua_register(L, "autoload", l_activatePlugin);
	lua_register(L, "margins", l_setMargins);
	lua_register(L, "getFileSize", l_getFileSize);
	lua_register(L, "getMD5", l_getMD5);
	lua_register(L, "getFileName", l_getFileName);
	lua_register(L, "getFilePath", l_getFilePath);
	lua_register(L, "getFileExtension", l_getFileExtension);
	lua_register(L, "registerMenuCallback", l_registerMenuCallback);
	lua_register(L, "showConsole", l_showConsole);
	lua_register(L, "getKey", l_getKey);
	lua_register(L, "getTextInput", l_getTextInput);
	lua_register(L, "newWindow", l_newWindow);
	lua_register(L, "setVisible", l_setVisible);
	lua_register(L, "printWindow", l_printWindow);
	lua_register(L, "clearWindow", l_clearWindow);
	lua_register(L, "getEditorBounds", l_getEditorBounds);
	lua_register(L, "getWindowBounds", l_getWindowBounds);
	lua_register(L, "setWindowBounds", l_setWindowBounds);
	lua_register(L, "getCursorWindowPosition", l_getCursorWindowPosition);
	lua_register(L, "getCursorFilePosition", l_getCursorFilePosition);
	lua_register(L, "setCursorFilePosition", l_setCursorFilePosition);
	lua_register(L, "mark", l_mark);
	lua_register(L, "unmark", l_unmark);

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
	for (int i = 0; i < plugins.size; i++) {
		Plugin* curr = ArrayList_Get(&plugins, i);
		if (curr->L == L) {
			return curr;
		}
	}
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
	if (index < file.content.size) {
		byte* value = ArrayList_Get(&file.content, index);
		lua_pushnumber(L, *value);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

int l_setContents(lua_State* L)		// file[index] = value
{
	int index = luaL_checknumber(L, 2);
	byte value = luaL_checknumber(L, 3);
	if (index < file.content.size) {
		ArrayList_Set(&file.content, index, &value);
		file.saved = 0;
	}
	return 0;
}

int l_log(lua_State* L) {
	char* c = luaL_checkstring(L, 1);
	clogf(getPluginByLuaState(L)->name, "%s", c);
	return 0;
}

int l_getFileSize(lua_State* L) {
	lua_pushnumber(L, file.content.size);
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

int l_getFileName(lua_State* L) {
	char* filename = strrchr(file.name, '/');
	if (filename == NULL) {
		filename = strrchr(file.name, '\\');
	}
	if (filename == NULL) {
		filename = file.name;
	}

	if (filename[0] == '/' || filename[0] == '\\') {
		filename = &filename[1];
	}

	lua_pushstring(L, filename);
	return 1;
}

int l_getFilePath(lua_State* L) {
	lua_pushstring(L, file.name);
	return 1;
}

int l_getFileExtension(lua_State* L) {
	char* extension = GET_EXTENSION(file.name);
	lua_pushstring(L, extension);
	return 1;
}

int l_registerMenuCallback(lua_State* L) {
	char* name = luaL_checkstring(L, 1);
	char* handlerName = luaL_checkstring(L, 2);
	SubMenuItem smi;
	smi.handlerType = HANDLER_LUA;
	smi.luaState = L;
	smi.plugin = getPluginByLuaState(L);
	strcpy(&smi.name, name);
	strcpy(&smi.luaHandlerName, handlerName);
	ArrayList_Add(&subMenuItems, &smi);
	return 0;
}

int l_showConsole(lua_State* L) {
	mode = CONSOLE;
	return 0;
}

int l_getKey(lua_State* L) {
	lua_pushstring(L, lastPressedKey);
	return 1;
}

int l_getTextInput(lua_State* L) {
	char* message = luaL_checkstring(L, 1);
	char text[TEXT_BUFFER_SIZE];

	echo();
	nocbreak();

	mvwprintw(statBar, 0, 0, "%s: ", message);
	wscanw(statBar, MATCH_ANY_PATTERN, text);
	lua_pushstring(L, text);

	noecho();
	cbreak();
	return 1;
}

int l_newWindow(lua_State* L) {
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	int w = luaL_checkinteger(L, 3);
	int h = luaL_checkinteger(L, 4);
	Plugin* p = getPluginByLuaState(L);
	PluginWindow pwindow;
	pwindow.visible = 1;
	pwindow.cursesWindow = newwin(y, x, h, w);
	pwindow.printCommands = ArrayList_New(4, sizeof(PrintCommand));
	ArrayList_Add(&p->windows, &pwindow);
	lua_pushinteger(L, p->windows.size - 1);
	return 1;
}

int l_setVisible(lua_State* L) {
	int window = luaL_checkinteger(L, 1);
	int visible = lua_toboolean(L, 2);

	Plugin* p = getPluginByLuaState(L);
	if (window >= 0 && window < p->windows.size) {
		PluginWindow* pwindow = ArrayList_Get(&p->windows, window);
		pwindow->visible = visible;
	}
	return 0;
}

int l_printWindow(lua_State* L) {
	int window = luaL_checkinteger(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	char* message = luaL_checkstring(L, 4);
	int color = luaL_checkinteger(L, 5);

	Plugin* p = getPluginByLuaState(L);
	if (window >= 0 && window < p->windows.size) {
		PluginWindow* pwindow = ArrayList_Get(&p->windows, window);
		PrintCommand pcommand;
		pcommand.x = x;
		pcommand.y = y;
		pcommand.color = color;
		strcpy(pcommand.content, message);
		ArrayList_Add(&pwindow->printCommands, &pcommand);
	}
	return 0;
}

int l_clearWindow(lua_State* L) {
	int window = luaL_checkinteger(L, 1);

	Plugin* p = getPluginByLuaState(L);
	if (window >= 0 && window < p->windows.size) {
		PluginWindow* pwindow = ArrayList_Get(&p->windows, window);
		ArrayList_Clear(&pwindow->printCommands);
	}
	return 0;
}

int l_getEditorBounds(lua_State* L) {
	lua_pushinteger(L, win->_begx);
	lua_pushinteger(L, win->_begy);
	lua_pushinteger(L, win->_maxx);
	lua_pushinteger(L, win->_maxy);
	return 4;
}

int l_getWindowBounds(lua_State* L) {
	int window = luaL_checkinteger(L, 1);
	Plugin* p = getPluginByLuaState(L);
	if (window >= 0 && window < p->windows.size) {
		PluginWindow* pwindow = ArrayList_Get(&p->windows, window);
		lua_pushinteger(L, pwindow->cursesWindow->_begx);
		lua_pushinteger(L, pwindow->cursesWindow->_begy);
		lua_pushinteger(L, pwindow->cursesWindow->_maxx);
		lua_pushinteger(L, pwindow->cursesWindow->_maxy);
	}
	return 4;
}

int l_setWindowBounds(lua_State* L) {
	int window = luaL_checkinteger(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int w = luaL_checkinteger(L, 4);
	int h = luaL_checkinteger(L, 5);
	Plugin* p = getPluginByLuaState(L);
	if (window >= 0 && window < p->windows.size) {
		PluginWindow* pwindow = ArrayList_Get(&p->windows, window);
		wresize(pwindow->cursesWindow, h, w);
		mvwin(pwindow->cursesWindow, y, x);
	}
	return 0;
}

int l_getCursorWindowPosition(lua_State* L) {
	int x, y;
	getCursorWindowPosition(&x, &y);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

int l_getCursorFilePosition(lua_State* L) {
	u64 pos;
	getCursorFilePosition(&pos);
	lua_pushinteger(L, pos);
	return 1;
}

int l_setCursorFilePosition(lua_State* L) {
	u64 pos = luaL_checkinteger(L, 1);
	setCursorPosition(pos);
	return 0;
}

int l_mark(lua_State* L) {
	u64 pos = luaL_checkinteger(L, 1);
	int color = luaL_checkinteger(L, 2);
	HashMap_Set(&highlights, &pos, &color);
	return 0;
}

int l_unmark(lua_State* L) {
	u64 pos = luaL_checkinteger(L, 1);
	HashMap_Remove(&highlights, &pos);
	return 0;
}