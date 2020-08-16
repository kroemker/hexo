
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

#define HAS_EXTENSION(f, e)		(strcmp(strrchr(f, '.'), e) == 0)

// general
int input(WINDOW* win, WINDOW* statBar, WINDOW* menuBar);
void drawMenu(WINDOW* menu);
unsigned char* loadFile(char* filename);
void saveFile(const char* filename);
int patchFile(const char* patchname);
void drawProgressBar(WINDOW* statBar, float p);

// plugin
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

char* newfile = "new";
int colors[] = {COLOR_WHITE, COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_BLACK, COLOR_YELLOW,
                COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK, COLOR_GREEN,
                COLOR_WHITE, COLOR_BLUE, COLOR_MAGENTA, COLOR_BLUE, COLOR_BLACK, COLOR_YELLOW,
                COLOR_WHITE, COLOR_RED, COLOR_YELLOW, COLOR_RED, COLOR_WHITE, COLOR_BLUE};

FileObject file = { 0 };
Plugin plugins[16] = { 0 };
int num_plugins = 0;
int mode = HEX_EDITOR;
char console[1024];
int messages = 0;

int main(int argc, char* argv[])
{
    int width, height;
    patchFailed = 0;

	// setup pdcurses, colors, windows
	initscr();
	noecho();
	cbreak();
	curs_set(0);

	start_color();

	init_pair(1, colors[0], colors[1]);
	init_pair(2, colors[2], colors[3]);
	init_pair(3, colors[4], colors[5]);

	getmaxyx(stdscr, height, width);
	WINDOW* win = newwin(height - 2, width, 1, 0);
	WINDOW* menuBar = newwin(1, width, 0, 0);
	WINDOW* statBar = newwin(1, width, height - 1, 0);

	keypad(statBar, TRUE);
	nodelay(statBar, FALSE);

	keypad(win, TRUE);
	nodelay(win, TRUE);
	wbkgd(win, COLOR_PAIR(1));
	wbkgd(statBar, COLOR_PAIR(1));
	wbkgd(menuBar, COLOR_PAIR(1));

	// init plugins
	consoleLoad();
	loadPlugins();

    // read arguments
    if(argc > 1)
    {
		if ((file.content = loadFile(argv[1]))) {
			file.loaded = 1;
			file.saved = 1;
			invokeActivePluginCallbacks("onFileLoad");
		}
        if((argc > 2) && (file.loaded))
        {
            if((strcmp(argv[2], "-p") == 0) && (argc > 3))
                patchFailed = patchFile(argv[3]);
        }
    }

    // open up empty 1 byte file
    if(!file.loaded)
    {
        file.content = (byte*)malloc(50);
        file.size = 1;
		file.capacity = 50;
        file.saved = 0;
        file.loaded = 2;
		strcpy(file.name, newfile);
    }

	// prepare editor
    drawMenu(menuBar);
	hexLoad(&file, win, statBar);
	hexDraw(win, statBar);
	plugSelLoad(plugins, num_plugins);

    // main loop
    int c;
    while(1)
    {
        c = input(win, statBar, menuBar);
		if (mode == HEX_EDITOR)
			c = hexInput(c, win, statBar);
		else if (mode == PLUGIN_SELECTOR)
			c = plugSelInput(c);

        if(!c)
            break;
        else if(c != -1)
        {
			wclear(win);
			wrefresh(win);
			if (mode == HEX_EDITOR)
				hexDraw(win, statBar);
			else if (mode == PLUGIN_SELECTOR)
				plugSelDraw();
			else if (mode == CONSOLE)
				consoleDraw();
            drawMenu(menuBar);
        }
    }

    endwin();
	if(file.content)
        free(file.content);
    return 0;
}

int input(WINDOW* win, WINDOW* statBar, WINDOW* menuBar)
{
    int ch = wgetch(win);
    switch(ch)
    {
    //color schemes F5-F8
    case KEY_F(5):
        init_pair(1, colors[0], colors[1]);
        init_pair(2, colors[2], colors[3]);
        init_pair(3, colors[4], colors[5]);
        wbkgd(win, COLOR_PAIR(1));
        wbkgd(statBar, COLOR_PAIR(1));
        wbkgd(menuBar, COLOR_PAIR(1));
        wrefresh(win);
        wrefresh(statBar);
        wrefresh(menuBar);
        break;
    case KEY_F(6):
        init_pair(1, colors[6], colors[7]);
        init_pair(2, colors[8], colors[9]);
        init_pair(3, colors[10], colors[11]);
        wbkgd(win, COLOR_PAIR(1));
        wbkgd(statBar, COLOR_PAIR(1));
        wbkgd(menuBar, COLOR_PAIR(1));
        wrefresh(win);
        wrefresh(statBar);
        wrefresh(menuBar);
        break;
    case KEY_F(7):
        init_pair(1, colors[12], colors[13]);
        init_pair(2, colors[14], colors[15]);
        init_pair(3, colors[16], colors[17]);
        wbkgd(win, COLOR_PAIR(1));
        wbkgd(statBar, COLOR_PAIR(1));
        wbkgd(menuBar, COLOR_PAIR(1));
        wrefresh(win);
        wrefresh(statBar);
        wrefresh(menuBar);
        break;
    case KEY_F(8):
        init_pair(1, colors[18], colors[19]);
        init_pair(2, colors[20], colors[21]);
        init_pair(3, colors[22], colors[23]);
        wbkgd(win, COLOR_PAIR(1));
        wbkgd(statBar, COLOR_PAIR(1));
        wbkgd(menuBar, COLOR_PAIR(1));
        wrefresh(win);
        wrefresh(statBar);
        wrefresh(menuBar);
        break;
	case KEY_F(10):
		mode = (mode + 1) % NUM_MODES;
		break;
	case KEY_F(12):
	{
		char answer[64] = { 'y' };
		if (!file.saved)
		{
			wclear(statBar);
			echo();
			nocbreak();
			mvwprintw(statBar, 0, 0, "File was modified. Quit anyway? (y or n) ");
			wscanw(statBar, "%s", answer);
			noecho();
			cbreak();
			wrefresh(statBar);
		}
		if (strcmp(answer, "y") == 0)
			return 0;
		break;
	}
    //save
    case 's':
        saveFile(file.name);
        break;
    }
    return ch;
}

void saveFile(const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    if(fp)
    {
		invokeActivePluginCallbacks("onFileSave");
        fwrite(file.content, sizeof(char), file.size, fp);
        fclose(fp);
        file.saved = 1;
    }
    else
    {
        file.saved = 2;
    }
}

void drawMenu(WINDOW* menu)
{
    int w,h;
    getmaxyx(menu, h, w);
    (void) h;
    mvwprintw(menu, 0, 2, "%s", file.name);
    mvwprintw(menu, 0, w-19, "Size: %8X Byte", file.size);

    if(patchFailed)
    {
        if(patchFailed == 1)
            mvwprintw(menu, 0, w-34, "Patch failed!");
        else if(patchFailed == 2)
            mvwprintw(menu, 0, w-34, "File patched!");
    }
	else
        if(file.loaded == 1)
            mvwprintw(menu, 0, w-34, "File loaded!");
        else if(file.loaded == 2)
            mvwprintw(menu, 0, w-34, "New file!");

    wattron(menu, COLOR_PAIR(2));
    if(file.saved == 0)
    {
        mvwaddch(menu, 0, 0, 'M');
    }
    else if(file.saved == 1)
    {
        mvwaddch(menu, 0, 0, 'S');
    }
    else if(file.saved == 2)
    {
        mvwaddch(menu, 0, 0, 'F');
    }
    wattroff(menu, COLOR_PAIR(2));
    wrefresh(menu);
}

unsigned char* loadFile(char* filename)
{
    FILE* fp;
    long lSize;
    unsigned char* buffer;
    size_t result;

    fp = fopen(filename, "rb");
	if (fp == NULL) { return NULL; }

    fseek (fp , 0 , SEEK_END);
    lSize = ftell (fp);
    rewind (fp);

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

int patchFile(const char* patchname)
{
    FILE* fp = fopen(patchname, "rt");
    if(!fp)
        return 1;
    unsigned long long addr = 0;
    unsigned int val = 0;
    while(!feof(fp))
    {
        fscanf(fp, "%llX %X\n", &addr, &val);
        if(addr < file.size)
        {
            file.content[addr] = val;
        }
        else
            return 1;
    }
    file.saved = 0;
    fclose(fp);
    return 2;
}

void drawProgressBar(WINDOW* statBar, float p)
{
    wtimeout(statBar,1);
    mvwaddch(statBar, 0, 0, '[');
    float k;
    for(k = 0; k < p; k++)
    {
        mvwaddch(statBar, 0, (int)(k+1), '#');
    }
    mvwaddch(statBar, 0, 101, ']');
    mvwprintw(statBar, 0, 103, "%.2f%%", p);
    wrefresh(statBar);
    wgetch(statBar);
    wtimeout(statBar,-1);
}

void loadPlugins()
{
	DIR* dir;
	struct dirent *ent;

	// read plugin dir
	if (dir = opendir("plugins"))
	{
		while (ent = readdir(dir))
		{
			if (ent->d_type == DT_REG && HAS_EXTENSION(ent->d_name, ".lua"))
			{
				Plugin* p = &plugins[num_plugins];
				num_plugins++;
				// initialize plugin
				char path[128] = { 0 };
				strcat(path, "plugins/");
				strcat(path, ent->d_name);
				strcpy(p->name, ent->d_name);
				p->L = luaL_newstate();
				luaL_openlibs(p->L);
				registerLuaCallables(p->L);
				if (luaL_dofile(p->L, path) != LUA_OK)
				{
					p->error = 1;
					cprintf("Error loading %s\n", lua_tostring(p->L, -1));
				}
				else
				{
					p->error = 0;
					cprintf("Loaded %s\n", ent->d_name);
				}
			}
		}
		closedir(dir);
	}
	else
		cprintf("Plugin directory not found!\n");
}

void invokeActivePluginCallbacks(char* name)
{
	for (int i = 0; i < num_plugins; i++)
	{
		if (!plugins[i].error && plugins[i].active && lua_getglobal(plugins[i].L, name) == LUA_TFUNCTION)
		{
			if (lua_pcall(plugins[i].L, 0, 0, 0) != LUA_OK)
			{
				plugins[i].error = 1;
				plugins[i].active = 0;
				cprintf("Error in %s\n", lua_tostring(plugins[i].L, -1));
				cprintf("%s is removed from the list of active plugins\n", plugins[i].name);
			}
		}
	}
}

void registerLuaCallables(lua_State* L)
{
	lua_register(L, "log", l_log);
	lua_register(L, "autoload", l_activatePlugin);
	lua_register(L, "margins", l_setMargins);
	lua_register(L, "getsize", l_getFileSize);

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

Plugin* getPluginByLuaState(lua_State* L)
{
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
	lua_pushnumber(L, file.content[index]);
	return 1;
}

int l_setContents(lua_State* L)		// file[index] = value
{
	int index = luaL_checknumber(L, 2);
	byte value = luaL_checknumber(L, 3);
	file.content[index] = value;
	file.saved = 0;
	return 0;
}

int l_log(lua_State* L)
{
	char* c = luaL_checkstring(L, 1);
	cprintf("%s", c);
	return 0;
}

int l_getFileSize(lua_State* L)
{
	lua_pushnumber(L, file.size);
	return 1;
}