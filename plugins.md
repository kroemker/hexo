# Plugin Documentation
Plugins can use system callbacks, menu items and/or key presses to execute their code. The [plugin directory](plugins/) contains sample plugins that can be used to understand the plugin system.

## Callbacks
* `onFileLoad()` - called when a file is loaded
* `onFileSave()` - called when the loaded file is saved
* `onClose()` - called when the program is closing
* `onInput()` - called when a key press occurred

## File
The array `file` represents the content of the file. Changing `file[10]` will directly change the content of `10`-th byte of the current file.

## Functions

### `log(str)`
* Arguments:
    1. `str`: string to log
* Returns: nothing
* Description: prints `str` to the plugin console

### `autoload()`
* Description: sets the plugin as active during the initialization process

### `getFileSize()`
* Returns: 
    1. `number`: file size of the current file

### `getMD5()`
* Returns: 
    1. `string`: MD5 hash the state of the current file

### `getFileName()`
* Returns:
    1. `string`: file name of the current file

### `getFilePath()`
* Returns:
    1. `string`: file path of the current file

### `getFileExtension()`
* Returns:
    1. `string`: file extension (incl. the dot) of the current file

### `registerMenuCallback(displayName, functionName)`
* Arguments:
    1. `displayName`: string to display on the menu
    2. `functionName`: function name as string (encapsulated in ")
* Description: registers a menu item with text `displayName`, that calls function `functionName` upon click


### `showConsole()`
* Description: shows the plugin console

### `getFileExtension()`
* Returns:
    1. `string`: last pressed key; to be used in the `onInput()` callback function

### `getTextInput(text)`
* Arguments:
    1. `text`: string that is displayed in front of the prompt
* Description: shows a string and a text input prompt

### `newWindow(x, y, w, h)`
* Arguments:
    1. `x`: number denoting the x position of the new window
    2. `y`: number denoting the y position of the new window
    3. `w`: number denoting the width of the new window
    4. `h`: number denoting the height of the new window
* Returns:
    1. `number`: window index to use with window functions
* Description: creates a new window inside the hex editor, that can be used to show arbitrary data

### `setVisible(window, visible)`
* Arguments:
    1. `window`: number denoting the window to set visible
    2. `visible`: boolean denoting whether to set the window in-/visible
* Description: sets the window visible or invisible

### `printWindow(window, x, y, message, color)`
* Arguments:
    1. `window`: number denoting the window to print on
    2. `x`: number denoting the x position to print from
    3. `y`: number denoting the y position to print from
    4. `message`: string to print
    5. `color`: number denoting the color index (must be between 1 and 8 inclusive)
* Description: prints a colored string on a window at a specific position

### `clearWindow(window)`
* Arguments:
    1. `window`: number denoting the window index
* Description: clears the window from all prints

### `getEditorBounds()`
* Returns:
    1. `number`: x position of the hex editor window
    2. `number`: y position of the hex editor window
    3. `number`: width of the hex editor window
    4. `number`: height of the hex editor window
* Description: returns the hex editor window bounds; useful for positioning new windows, since their coordinates are relativ to the hex editor window

### `getWindowBounds(window)`
* Arguments:
    1. `window`: number denoting the window index
* Returns:
    1. `number`: x position of the window
    2. `number`: y position of the window
    3. `number`: width of the window
    4. `number`: height of the window
* Description: returns window bounds relative to the hex editor window bounds

### `setWindowBounds(x, y, w, h)`
* Arguments:
    1. `window`: number denoting the window index
    2. `x`: number denoting the new x position of the window
    3. `y`: number denoting the new y position of the window
    4. `w`: number denoting the new width of the window
    5. `h`: number denoting the new height of the window
* Description: sets window bounds relative to the hex editor window bounds

### `getCursorWindowPosition()`
* Returns:
    1. `number`: x position of the cursor in the hex editor window
    2. `number`: y position of the cursor in the hex editor window
* Description: returns the UI cursor position inside the hex editor window


### `getCursorFilePosition()`
* Returns:
    1. `number`: position of the cursor inside file bounds
* Description: returns the current index of the cursor in the file

### `setCursorFilePosition(position)`
* Arguments:
    1. `position`: number denoting the new cursor position inside file bounds
* Description: sets the current index of the cursor in the file

### `mark(position, color)`
* Arguments:
    1. `position`: number denoting the position to mark a byte inside file bounds
    2. `color`: color to use (must be between 1 and 3 inclusive)
* Description: marks a byte at a position with a color

### `unmark(position)`
* Arguments:
    1. `position`: number denoting the position to unmark a byte inside file bounds
* Description: unmarks a byte at a position

### `getSizeMode()`
* Returns:
    1. `number`: number denoting how many bytes are selected (always one of 1, 2, 4, 8)

### `disableInternalKeys()`
* Description: prevents most actions from key presses; useful if keys should be used that are in use by the hex editor

### `enableInternalKeys()`
* Description: enables actions from key presses

