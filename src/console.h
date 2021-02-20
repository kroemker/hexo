#ifndef CONSOLE_H
#define CONSOLE_H

void consoleLoad();
void consoleDraw();
void clogf(const char* caller, const char* format, ...);
void cwarnf(const char* caller, const char* format, ...);
void cerrorf(const char* caller, const char* format, ...);

#endif