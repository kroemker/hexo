
#include "console.h"
#include "globals.h"
#include "ArrayList.h"

#include <curses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct tMessage {
	char caller[64];
	char content[128];
}Message;

static WINDOW* con_win;
ArrayList msgs;
void consoleLoad() {
	int h, w;
	getmaxyx(stdscr, h, w);
	con_win = newwin(h - 2, w, 1, 0);
	wbkgd(con_win, COLOR_NORMAL);
	msgs = ArrayList_New(10, sizeof(Message));
}

void consoleDraw()
{
	int h, w;
	getmaxyx(con_win, h, w);

	int line_start = msgs.size - (h - 2);
	line_start = line_start < 0 ? 0 : line_start;

	for (int i = 0; i < h-2 && i < msgs.size; i++)
	{
		Message* msg = ArrayList_Get(&msgs, i + line_start);
		wattron(con_win, COLOR_CALLER);
		mvwprintw(con_win, i + 1, 1, "%s: ", msg->caller);
		wattroff(con_win, COLOR_CALLER);
		wprintw(con_win, "%s", msg->content);
	}

	box(con_win, 0, 0);
	wrefresh(con_win);
}

void cprintf(const char* caller, const char* format, ...) {
	Message msg;
	strcpy(msg.caller, caller);

	va_list args;
	va_start(args, format);
	int n = vsprintf(&msg.content, format, args);
	va_end(args);
	ArrayList_Add(&msgs, &msg);
}