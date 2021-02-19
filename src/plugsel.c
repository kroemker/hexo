
#include "plugsel.h"
#include "globals.h"
#include "ArrayList.h"

#include <curses.h>
#include <string.h>

static WINDOW* sel_win;
static ArrayList* plugins;
static int cursor;
static int win_width = 40;

void plugSelLoad(ArrayList* _plugins)
{
	int h, w;
	getmaxyx(stdscr, h, w);
	sel_win = newwin(_plugins->size + 2, win_width, h / 2 - 5, w / 2 - win_width / 2);
	keypad(sel_win, TRUE);
	nodelay(sel_win, TRUE);
	wbkgd(sel_win, COLOR_NORMAL);
	plugins = _plugins;
	cursor = 0;
}

int plugSelInput(int c)
{
	Plugin* curr = ArrayList_Get(plugins, cursor);
	switch (c)
	{
	case KEY_DOWN:
		cursor = (cursor + 1) % plugins->size;
		break;
	case KEY_UP:
		cursor = (cursor - 1) % plugins->size;
		break;
	case ' ':
		curr->active = !(curr->active);
		break;
	}

	return c;
}

void plugSelDraw()
{
	for (int i = 0; i < plugins->size; i++)
	{
		if (i == cursor)
			wattron(sel_win, COLOR_CURSOR);
		else
			wattron(sel_win, COLOR_NORMAL);

		Plugin* curr = ArrayList_Get(plugins, i);
		int len = strlen(curr->name);

		mvwprintw(sel_win, i + 1, 1, "%s", curr->name);
		for (int j = len; j < win_width - 4; j++)
			mvwaddch(sel_win, i + 1, j + 1, ' ');

		if (curr->active)
			wattron(sel_win, COLOR_HIGHLIGHT1);
		else
			wattron(sel_win, COLOR_NORMAL);
			
		mvwaddch(sel_win, i + 1, win_width - 2, ' ');

		if (curr->active)
			wattroff(sel_win, COLOR_HIGHLIGHT1);
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
