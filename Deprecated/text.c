
#include <stdlib.h>
#include <curses.h>
#include "text.h"
#include "globals.h"

#define TAB_SIZE1 10
#define TAB_SIZE2 5

Editor* this;

unsigned long fileSize;
byte* fileContent;

char* textContent;
int   cursor;
int   textLength;
int   textCapacity;
int   textShift;

void writeChar(char ch);
void checkTextShift(WINDOW* win);

void textLoad(Editor* e, FileObject* file, WINDOW* win, WINDOW* statBar)
{
	fileSize = file->size;
	fileContent = file->content;

	textContent = malloc(1024);
	cursor = 0;
	textLength = 0;
	textCapacity = 1024;
	textShift = 0;
}

int textInput(int ch, WINDOW* win, WINDOW* statBar)
{
    int h, w, i, j, x;
	getmaxyx(win, h, w);
	
	switch (ch)
	{
	//cursor movement, arrow keys
	case KEY_UP:
		for (i = cursor; i > 0 && textContent[i - 1] != '\n'; --i);
		x = cursor - i;
		for (i = cursor - x - 1; i > 0 && textContent[i - 1] != '\n'; --i);
		if (i < 0)
			cursor = 0;
		else
		{
			for (j = 0; j < x && i + j < textLength && textContent[i + j] != '\n'; ++j);
			cursor = i + j;
		}
		break;
	case KEY_DOWN:
		for (i = cursor; i > 0 && textContent[i - 1] != '\n'; --i);
		x = cursor - i;
		for (i = cursor; i < textLength && textContent[i] != '\n'; ++i);
		if (i >= textLength)
			cursor = textLength;
		else
		{
			++i;
			for (j = 0; j < x && i + j < textLength && textContent[i + j] != '\n'; ++j);
			cursor = i + j;
		}
		break;
	case KEY_RIGHT:
		cursor = cursor > textLength ? textLength : cursor + 1;
		break;
	case KEY_LEFT:
		cursor = cursor <= 0 ? 0 : cursor - 1;	
		break;
	}

	if ((ch >= 33 && ch <= 126) || (ch == '\n') || (ch == ' '))
	{
		writeChar(ch);
	}
	else if (ch == 8) // backspace
	{
		cursor = cursor > 0 ? cursor - 1 : 0;
		textLength = textLength > 0 ? textLength - 1 : 0;
		for (i = cursor; i < textLength; i++)
			textContent[i] = textContent[i + 1];
	}
	else if (ch == KEY_DC) // delete
	{
		textLength = cursor < textLength ? textLength - 1 : textLength;
		for (i = cursor; i < textLength; i++)
			textContent[i] = textContent[i + 1];
	}
	else if (ch == '\t') // tab
	{
		for (i = cursor; i > 0 && textContent[i - 1] != '\n'; --i);
		x = cursor - i;
		if (x < TAB_SIZE1)
			for (i = x; i < TAB_SIZE1; ++i)
				writeChar(' ');
		else
			writeChar(' ');
		for (i = x + 1; i % TAB_SIZE2 != 0; ++i)
			writeChar(' ');
	}
	checkTextShift(win);
	return ch;
}

void textDraw(WINDOW* win, WINDOW* statBar)
{
    int w, h, i;
	int lm = this->margins[MARGIN_LEFT], rm = this->margins[MARGIN_RIGHT], tm = this->margins[MARGIN_TOP], bm = this->margins[MARGIN_BOTTOM];
	getmaxyx(win, h, w);
	int linelen = w - lm - rm - 1;

	int x = 0;
	int y = 0;
	for (i = 0; i < textLength; i++)
	{
		if (x >= textShift && x < linelen + textShift)
		{
			// print
			if (i == cursor)
				wattron(win, COLOR_PAIR(2));
			else
				wattron(win, COLOR_PAIR(1));

			if (textContent[i] != '\n')
				mvwaddch(win, y + 1 + tm, x + 1 + lm - textShift, textContent[i]);
			else
				mvwaddch(win, y + 1 + tm, x + 1 + lm - textShift, ' ');

			if (i == cursor)
				wattroff(win, COLOR_PAIR(2));
			else
				wattroff(win, COLOR_PAIR(1));
		}

		// advance draw pointer
		if (textContent[i] == '\n')
		{
			x = 0;
			++y;
		}
		else
			++x;
	}
	if (cursor >= textLength)
	{
		wattron(win, COLOR_PAIR(2));
		mvwaddch(win, y + 1 + tm, x + 1 + lm - textShift, ' ');
		wattroff(win, COLOR_PAIR(2));
	}

	mvwprintw(statBar, 0, 0, "%d", textShift);

    box(win, 0, 0);
    wrefresh(win);
	wrefresh(statBar);
}

void writeChar(char ch)
{
	if (textLength >= textCapacity)
	{
		textContent = realloc(textContent, textCapacity * 2);
		textCapacity *= 2;
	}

	for (int i = textLength; i > cursor; i--)
		textContent[i] = textContent[i - 1];
	textContent[cursor] = ch;
	++cursor;
	++textLength;
}

void checkTextShift(WINDOW* win)
{
	int h, w, i, j, x;
	getmaxyx(win, h, w);
	int lm = this->margins[MARGIN_LEFT], rm = this->margins[MARGIN_RIGHT], tm = this->margins[MARGIN_TOP], bm = this->margins[MARGIN_BOTTOM];
	int linelen = w - lm - rm - 1;

	for (i = cursor; i > 0 && textContent[i - 1] != '\n'; --i);
	x = cursor - i;
	if (x >= linelen - textShift)
		textShift = x - linelen;
	else if (x < textShift)
		textShift = x;
}