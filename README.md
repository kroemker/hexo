# Hexo
Hexo is a hex editor that is suited for viewing, editing and creating of binary files. The editor uses the *curses* library to run inside a console window and supports common functions for editing files. Additional functionality can be added using the *Lua* interface.

## Building
The program is designed to compile easily on Windows and Linux. A *Visual Studio* solution is contained at the root of repo that can be used for compiling under Windows.

### Prerequisites

* [PDCurses](https://pdcurses.org) - curses library for Windows
* [dirent](https://github.com/tronkko/dirent) - folder access wrapper for Windows
* [Lua 5.4](http://www.lua.org) - scripting library for plugins

When compiling under Linux only the *Lua* library is needed since *ncurses* and *dirent* are native to most systems.

## Usage

```
hexo [file [-p patch]]
```

* `file`:     open file
* `-p patch`: applies patch to the opened file

### Hotkeys
* X : Cut
* V : Paste
* Q : Copy
* N : Insert in front of cursor
* M : Insert behind cursor
* T : Search for text
* R : Search for raw data
* G : Go to address
* P : Create a patch with changes since the last save
* Z : Undo
* Y : Redo
* S : Save file
* +/- : Increments/Decrements current selection
* TAB: Switch little/big endian
* 0-9,A-F : Enables fast overwrite mode
* F1-F4 : Switch size mode options
* F5-F8 : Switch color schemes
* F10 : View plugin info
* F12 : Quit

### Plugins
At the start of the program all `.lua` files in the subdirectory `plugins` are initialized. During that initialization the plugin can activate itself. Deactivated plugins can be activated in the program by pressing *F10* to open the plugin dialog, selecting the plugin using the arrow keys and pressing *Space*. The plugin console can be accessed by pressing *F10* again and is used to debug plugins.
Aside from initialization a plugin is invoked using callbacks of the following functions:

* `onFileLoad()` - called when a file is loaded
* `onFileSave()` - called when the loaded file is saved

A plugin can interact with the editor by
* array `file` that represents the content of the file
* function `margins(left, right, top, bottom)` that sets margins for the editor
* function `autoload()` that sets the plugin as active during the initialization process
* function `getsize()` that returns the file size
* function `log(str)` that prints `str` to the plugin console 
