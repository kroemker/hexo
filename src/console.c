
#include "console.h"
#include <curses.h>
#include <stdarg.h>
#include <stdlib.h>

WINDOW* con_win;
char* msgs;
int msgs_size = 0;
int msgs_capacity = 1024;
int line = 0;
int line_start = 0;

void consoleLoad()
{
	int h, w;
	getmaxyx(stdscr, h, w);
	con_win = newwin(h - 2, w, 1, 0);
	wbkgd(con_win, COLOR_PAIR(1));
	msgs = malloc(msgs_capacity);
}

void consoleDraw()
{
	int h, w, pos = 0;
	getmaxyx(con_win, h, w);

	for (int i = 0; i < line && pos < msgs_size; i++)
	{
		mvwprintw(con_win, i + 1, 1, "%s", &msgs[pos]);

		while (pos < msgs_size && msgs[pos] != 0)
			pos++;
		pos++;
	}

	box(con_win, 0, 0);
	wrefresh(con_win);
}

void cprintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int n = vsprintf(&msgs[msgs_size], format, args);
	va_end(args);

	line++;
	msgs_size += n + 1;
	if (msgs_size > msgs_capacity - 128)
	{
		msgs = realloc(msgs, msgs_capacity * 2);
		msgs_capacity *= 2;
	}
}