#ifndef PLUGIN_SELECTOR_H
#define PLUGIN_SELECTOR_H

#include "globals.h"
#include "ArrayList.h"

void plugSelLoad(ArrayList* _plugins);
int plugSelInput(int c);
void plugSelDraw();

#endif