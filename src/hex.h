#ifndef HEX_H
#define HEX_H

#include <curses.h>

void hexLoad(FileObject* _file, WINDOW* win, WINDOW* statBar);
int hexInput(int ch, WINDOW* win, WINDOW* statBar);
void hexDraw(WINDOW* win, WINDOW* statBar);


void getCursorWindowPosition(int* x, int* y);
void getCursorFilePosition(int* pos);

#endif // HEX_H
