#ifndef PLUGIN_SELECTOR_H
#define PLUGIN_SELECTOR_H

#include "globals.h"

void plugSelLoad(Plugin* _plugins, int _num_plugins);
int plugSelInput(int c);
void plugSelDraw();

#endif