#ifndef HEX_H
#define HEX_H

#include <curses.h>

void hexLoad(FileObject* _file, WINDOW* win, WINDOW* statBar);
int hexInput(int ch, WINDOW* win, WINDOW* statBar);
void hexDraw(WINDOW* win, WINDOW* statBar);


void getCursorWindowPosition(int* x, int* y);
void getCursorFilePosition(u64* pos);
void setCursorPosition(u64 pos);

extern HashMap highlights;

#endif // HEX_H
