
#include "console.h"
#include "globals.h"
#include "ArrayList.h"

#include <curses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define MESSAGE_LOG		0
#define MESSAGE_ERROR	1
#define MESSAGE_WARN	2

typedef struct tMessage {
	char caller[64];
	char content[128];
	char type;
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

		int msgLength = strlen(msg->caller) + 2 + strlen(msg->content);

		switch (msg->type) {
		case MESSAGE_LOG:
			wattron(con_win, COLOR_CALLER);
			mvwprintw(con_win, i + 1, 1, "%s: ", msg->caller);
			wattroff(con_win, COLOR_CALLER);
			wprintw(con_win, "%s", msg->content);
			break;
		case MESSAGE_WARN:
			wattron(con_win, COLOR_WARN);
			mvwprintw(con_win, i + 1, 1, "%s: ", msg->caller);
			wprintw(con_win, "%s", msg->content);
			wattroff(con_win, COLOR_WARN);
			break;
		case MESSAGE_ERROR:
			wattron(con_win, COLOR_ERROR);
			mvwprintw(con_win, i + 1, 1, "%s: ", msg->caller);
			wprintw(con_win, "%s", msg->content);
			wattroff(con_win, COLOR_ERROR);
			break;
		}
	}

	box(con_win, 0, 0);
	wrefresh(con_win);
}


void clogf(const char* caller, const char* format, ...) {
	Message msg;
	msg.type = MESSAGE_LOG;
	strcpy(msg.caller, caller);

	va_list args;
	va_start(args, format);
	int n = vsprintf(&msg.content, format, args);
	va_end(args);
	ArrayList_Add(&msgs, &msg);
}

void cwarnf(const char* caller, const char* format, ...) {
	Message msg;
	msg.type = MESSAGE_WARN;
	strcpy(msg.caller, caller);

	va_list args;
	va_start(args, format);
	int n = vsprintf(&msg.content, format, args);
	va_end(args);
	ArrayList_Add(&msgs, &msg);
}

void cerrorf(const char* caller, const char* format, ...) {
	Message msg;
	msg.type = MESSAGE_ERROR;
	strcpy(msg.caller, caller);

	va_list args;
	va_start(args, format);
	int n = vsprintf(&msg.content, format, args);
	va_end(args);
	ArrayList_Add(&msgs, &msg);
}