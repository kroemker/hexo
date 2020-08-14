#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include "globals.h"

void textLoad(Editor* e, FileObject* file, WINDOW* win, WINDOW* statBar);
int textInput(int ch, WINDOW* win, WINDOW* statBar);
void textDraw(WINDOW* win, WINDOW* statBar);

#endif // CODE_EDITOR_H
