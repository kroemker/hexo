#ifndef PATCHER_H
#define PATCHER_H

#include <curses.h>
#include "globals.h"

int patcherInput(int, WINDOW*, WINDOW*); //ch, win and statbar
void patcherDraw(WINDOW*, WINDOW*);
void patcherLoad(Editor*, FileObject*, WINDOW*, WINDOW*);
void patcherUnload(WINDOW*, WINDOW*);

#endif // PATCHER_H
