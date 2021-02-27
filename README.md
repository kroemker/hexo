# Hexo
Hexo is a hex editor suited for creating, viewing and editing of binary files. It uses the *curses* library to run inside a console window and supports common functions for editing files. Additional functionality can be added using the *Lua* interface.

## Building

#### Windows

A Visual Studio solution is contained at the root of repo that can be used for compiling under Windows. The project has the following dependencies:

* [PDCurses](https://pdcurses.org) - curses library for Windows
* [dirent](https://github.com/tronkko/dirent) - folder access wrapper for Windows
* [Lua 5.4](http://www.lua.org) - scripting library for plugins

Pay attention to set the include and library paths correctly inside Visual Studio.

#### Linux

1. Install build dependencies

The following packages are required:
* ncurses
* Lua 5.4

Under Ubuntu they can be installed as follows:

```
sudo apt-get install libncurses5-dev libncursesw5-dev
wget -R -O http://www.lua.org/ftp/lua-5.4.0.tar.gz
tar zxf lua-5.4.0.tar.gz
cd lua-5.4.0
make all test
sudo make install
```

2. Build source
In the cloned repo directory:

```
make && sudo make install
```

## Usage

```
hexo [file [-p luapatch]]
```

* `file`:     open file
* `-p luapatch`: executes the luapatch file after loading

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
* L : Toggle plugin enabler/plugin console/hex editor
* PageUp/PageDown: Move the menu cursor
* Enter: Execute the selected menu item
* +/- : Increments/Decrements current selection
* TAB: Switch little/big endian
* 0-9,A-F : Overwrite data
* F1-F4 : Switch size mode options
* F12 : Quit

### Plugins
At the start of the program all `.lua` files in the subdirectory `plugins` are initialized. During that initialization the plugin can activate itself. Deactivated plugins can be activated in the program by pressing *L* to open the plugin dialog, selecting the plugin using the arrow keys and pressing *Space*. The plugin console can be accessed by pressing *L* again and is used to debug plugins. Plugins can execute their code using callbacks, menu items and keypresses. For more information view the [plugin documentation](plugins.md).
