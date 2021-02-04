
#include "plugsel.h"
#include "globals.h"

#include <curses.h>
#include <string.h>

WINDOW* sel_win;
Plugin* plugs;
int num_plugins;
int cursor;
int win_width = 40;

void plugSelLoad(Plugin* _plugins, int _num_plugins)
{
	int h, w;
	getmaxyx(stdscr, h, w);
	sel_win = newwin(num_plugins + 2, win_width, h / 2 - 5, w / 2 - win_width / 2);
	keypad(sel_win, TRUE);
	nodelay(sel_win, TRUE);
	wbkgd(sel_win, COLOR_NORMAL);
	plugs = _plugins;
	num_plugins = _num_plugins;
	cursor = 0;
}

int plugSelInput(int c) 
{
	switch (c)
	{
	case KEY_DOWN:
		cursor = (cursor + 1) % num_plugins;
		break;
	case KEY_UP:
		cursor = (cursor - 1) % num_plugins;
		break;
	case ' ':
		plugs[cursor].active = !plugs[cursor].active;
		break;
	}

	return c;
}

void plugSelDraw()
{
	for (int i = 0; i < num_plugins; i++)
	{
		if (i == cursor)
			wattron(sel_win, COLOR_CURSOR);
		else
			wattron(sel_win, COLOR_NORMAL);

		int len = strlen(plugs[i].name);

		mvwprintw(sel_win, i + 1, 1, "%s", plugs[i].name);
		for (int j = len; j < win_width - 4; j++)
			mvwaddch(sel_win, i + 1, j + 1, ' ');

		if (plugs[i].active)
			wattron(sel_win, COLOR_HIGHLIGHT);
		else
			wattron(sel_win, COLOR_NORMAL);
			
		mvwaddch(sel_win, i + 1, win_width - 2, ' ');

		if (plugs[i].active)
			wattroff(sel_win, COLOR_HIGHLIGHT);
		else
			wattroff(sel_win, COLOR_NORMAL);

		if (i == cursor)
			wattroff(sel_win, COLOR_CURSOR);
		else
			wattroff(sel_win, COLOR_NORMAL);

	}

	box(sel_win, 0, 0);
	mvwprintw(sel_win, 0, 1, "Plugins");
	wrefresh(sel_win);
}
