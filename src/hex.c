#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "hex.h"

#define BIG_ENDIAN			0
#define LITTLE_ENDIAN		1

#define SEARCH_TEXT			0
#define SEARCH_DATA			1

#define UP					0
#define DOWN				1
#define RIGHT				2
#define LEFT				3
#define SHIFT_RIGHT			4
#define SHIFT_LEFT			5

#define CHANGE_INCREMENT	0
#define CHANGE_BASIC		1
#define CHANGE_INSERT		2
#define CHANGE_DELETE		3

typedef struct
{
    u64 address;
    u64 prev_val;
    u64 new_val;
    int endian;
    int val_size;
    int type;
    int saved;
}Change;

static FileObject* file;

static int fastTypeMode;
static u64 fastOverwriteValue;
static u64 clipboardValue;

static int  curX;
static int  curY;
static long long currentStartAddr;

static char endianness[] = {'B','L'};
static int  endianMode;
static int  sizeMode;

HashMap highlights;
static ArrayList undoList;
static numUndos = 0;
static numUndoRedos = 0;

void moveCursor(WINDOW* win, int direction);
u64 searchData(int cursorPos, byte* dat, int datSize);
void searchMenu(int searchMode, WINDOW* win, WINDOW* statBar);
void insertData(unsigned long position, int size);
void deleteData(unsigned long position, int size);
void gotoAddressMenu(WINDOW* win, WINDOW* statBar);
void addToUndoList(int type, u64 address, u64 prev_val, u64 new_val);
void undo(WINDOW* statBar);
void redo(WINDOW* statBar);
void createPatchFromUndoList(WINDOW* statBar);
void getCursorWindowPosition(int* x, int* y);
void getCursorFilePosition(u64* pos);

void hexLoad(FileObject* f, WINDOW* win, WINDOW* statBar)
{
	file = f;
    curX = 0;
    curY = 0;
    sizeMode = 1;
    endianMode = 0;
    currentStartAddr = 0;
    fastOverwriteValue = 0;
    fastTypeMode = 0;
    clipboardValue = 0;
	undoList = ArrayList_New(50, sizeof(Change));
	highlights = HashMap_New(100, sizeof(u64), sizeof(int));
}

int hexInput(int ch, WINDOW* win, WINDOW* statBar)
{
    int h, w, i, j, x;
    u64 c = 0;
    u64 prev = 0;

    getmaxyx(win, h, w);
    switch (ch)
    {
	case KEY_UP:
		moveCursor(win, UP);
		break;
	case KEY_DOWN:
		moveCursor(win, DOWN);
		break;
	case KEY_RIGHT:
		moveCursor(win, SHIFT_RIGHT);
		break;
	case KEY_LEFT:
		moveCursor(win, SHIFT_LEFT);
		break;
    //size mode functionality F1-F4
    case KEY_F(1):
        sizeMode = 1;
        fastTypeMode = 0;
        break;
    case KEY_F(2):
        if((currentStartAddr + curX + 1 + curY*0x10) < file->content.size)
        {
            sizeMode = 2;
            curX = (curX/sizeMode) * sizeMode;
            fastTypeMode = 0;
        }
        break;
    case KEY_F(3):
        if((currentStartAddr + curX + 3 + curY*0x10) < file->content.size)
        {
            sizeMode = 4;
            curX = (curX/sizeMode) * sizeMode;
            fastTypeMode = 0;
        }
        break;
    case KEY_F(4):
        if((currentStartAddr + curX + 7 + curY*0x10) < file->content.size)
        {
            sizeMode = 8;
            curX = (curX/sizeMode) * sizeMode;
            fastTypeMode = 0;
        }
        break;
    case '\t':
        endianMode = (endianMode + 1) & 1;
        break;
    case 'z':
        undo(statBar);
        break;
    case 'y':
        redo(statBar);
        break;
    //+-, inc,dec functionality
    //case PADPLUS:
    case '+':
		for (i = 0; i < sizeMode; i++) {
			u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
			c |= *bt << (i * 8);
		}
        prev = c;
        c++;
		for (i = 0; i < sizeMode; i++) {
			byte bt = (c >> (i * 8)) & 0xFF;
			ArrayList_Set(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10, &bt);
		}

        addToUndoList(CHANGE_INCREMENT, currentStartAddr + curX + curY * 0x10, prev, c);
        fastTypeMode = 0;
        file->saved = 0;
        break;
    //case PADMINUS:
    case '-':
		for (i = 0; i < sizeMode; i++) {
			u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
			c |= *bt << (i * 8);
		}
        prev = c;
        c--;
		for (i = 0; i < sizeMode; i++) {
			byte bt = (c >> (i * 8)) & 0xFF;
			ArrayList_Set(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10, &bt);
		}

        addToUndoList(CHANGE_INCREMENT, currentStartAddr + curX + curY * 0x10, prev, c);
        fastTypeMode = 0;
        file->saved = 0;
        break;
    //search text
    case 't':
        searchMenu(SEARCH_TEXT, win, statBar);
        break;
    //search raw data
    case 'r':
        searchMenu(SEARCH_DATA, win, statBar);
        break;
    //goto address
    case 'g':
        gotoAddressMenu(win, statBar);
        break;
    //insert byte
    case 'n':
		insertData(currentStartAddr + curX + curY * 0x10, sizeMode);
		addToUndoList(CHANGE_INSERT, currentStartAddr + curX + curY * 0x10, 0, 0);
        file->saved = 0;
        break;
    case 'm':
		insertData(currentStartAddr + curX + curY * 0x10 + sizeMode, sizeMode);
		addToUndoList(CHANGE_INSERT, currentStartAddr + curX + curY * 0x10 + sizeMode, 0, 0);
        file->saved = 0;
        break;
    case 'p':
        createPatchFromUndoList(statBar);
        break;
    //cut
    case 'x':
		clipboardValue = 0;
        for(i = 0; i < sizeMode; i++)
        {
			u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
            clipboardValue |= *bt << (i*8);
        }
        addToUndoList(CHANGE_DELETE, currentStartAddr + curX + curY * 0x10, clipboardValue, 0);

        deleteData(currentStartAddr + curX + curY * 0x10, sizeMode);
        moveCursor(win, SHIFT_LEFT);
        file->saved = 0;
        break;
    //copy
    case 'q':
		clipboardValue = 0;
		for (i = 0; i < sizeMode; i++) {
			u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
			clipboardValue |= *bt << (i * 8);

		}
        break;
    //paste
    case 'v':
        //prepare undo item
        prev = 0;
		for (i = 0; i < sizeMode; i++) {
			u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
			prev |= *bt << (i * 8);
		}

        addToUndoList(CHANGE_BASIC, currentStartAddr + curX + curY * 0x10, prev, clipboardValue);

        //write new value
        for(i = 0; i < sizeMode; i++)
        {
			if (endianMode) { //little endian
				byte bt = (clipboardValue >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, currentStartAddr + curX + i + curY * 0x10, &bt);
			}
			else {
				byte bt = (clipboardValue >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10, &bt);
			}
			file->saved = 0;
        }
        break;
    //fast overwrite
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        if(!fastTypeMode)
        {
            fastOverwriteValue = 0;
            fastTypeMode = sizeMode*2;
        }
        if(ch < 'A')
            fastOverwriteValue |= (u64)(ch - '0')  << ((fastTypeMode-1)*4);
        else
            fastOverwriteValue |= (u64)(ch - 'a' + 0x0A)  << ((fastTypeMode-1)*4);
        fastTypeMode--;
        if(!fastTypeMode)
        {
            //prepare undo item
            prev = 0;
			for (i = 0; i < sizeMode; i++) {
				u64* bt = ArrayList_Get(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10);
				prev |= *bt << (i * 8);
			}

            addToUndoList(CHANGE_BASIC, currentStartAddr + curX + curY * 0x10, prev, fastOverwriteValue);

            //write new value
            for(i = 0; i < sizeMode; i++)
            {
				if (endianMode) { //little endian
					byte bt = (fastOverwriteValue >> (i * 8)) & 0xFF;
					ArrayList_Set(&file->content, currentStartAddr + curX + i + curY * 0x10, &bt);
				}
				else {
					byte bt = (fastOverwriteValue >> (i * 8)) & 0xFF;
					ArrayList_Set(&file->content, currentStartAddr + curX + sizeMode - i - 1 + curY * 0x10, &bt);
				}
            }
            file->saved = 0;
            //shift cursor
            moveCursor(win, SHIFT_RIGHT);

            wclear(statBar);
            wrefresh(statBar);
        }
        break;
    }
    return ch;
}

void hexDraw(WINDOW* win, WINDOW* statBar)
{
	int w, h, i;
	int lm = margins[MARGIN_LEFT], rm = margins[MARGIN_RIGHT], tm = margins[MARGIN_TOP], bm = margins[MARGIN_BOTTOM];
	(void)w;
	unsigned char content;

	wclear(win);
	wclear(statBar);

	getmaxyx(win, h, w);
	mvwprintw(win, 1 + tm, 1 + lm, "%c", endianness[endianMode]);

	//print vertical idents
	int x = 0;
	for (i = 12 + lm;; i += sizeMode * 2 + 1)
	{
		mvwprintw(win, 1 + tm, i, "%X", x);
		if ((x = x + sizeMode) >= 0x10)
			break;
	}

	int asciiOffset = lm + 12 + 0x10 / sizeMode * (sizeMode * 2 + 1);
	//print indices and data
	for (i = 2; i < h - 1 - bm - tm; i++)
	{
		//index
		mvwprintw(win, i + tm, 1 + lm, "%08X |", currentStartAddr + (i - 2) * 0x10);
		//data
		int j = 0, k;
		while (j < 0x10)
		{
			u64 curr = currentStartAddr + (i - 2) * 0x10 + j;
			int* highlightColor = HashMap_Get(&highlights, &curr);
			for (k = 0; k < sizeMode * 2; k += 2)
			{
				if (file->content.size >  curr + k / 2) {
					//cursor + highlight on
					if ((curX == j) && (curY == i - 2)) {
						wattron(win, COLOR_CURSOR);
					}
					else if (highlightColor != NULL) {
						wattron(win, COLOR_HIGHLIGHT(*highlightColor % 3));
					}

					//handle endianness
					if (endianMode) {
						content = *(unsigned char*)ArrayList_Get(&file->content, curr + (sizeMode - k / 2 - 1));
					}
					else {
						content = *(unsigned char*)ArrayList_Get(&file->content, curr + k / 2);
					}
					//print hex
					mvwprintw(win, i + tm, lm + 12 + j / sizeMode * (sizeMode * 2 + 1) + k, "%02X", content);
					//print ascii
					if ((content <= 0x1F) || (content == 0x7F) || (content == 0xFF)) //control character
						mvwaddch(win, i + tm, asciiOffset + j + k / 2, '.');
					else
						mvwaddch(win, i + tm, asciiOffset + j + k / 2, content);

					//cursor + highlight off
					if ((curX == j) && (curY == i - 2)) {
						wattroff(win, COLOR_CURSOR);
					}
					else if (highlightColor != NULL) {
						wattroff(win, COLOR_HIGHLIGHT(*highlightColor % 3));
					}
				}
				else {
					break;
				}
			}
			j += sizeMode;
		}
	}

	box(win, 0, 0);
	wrefresh(win);
	wrefresh(statBar);
}

void moveCursor(WINDOW* win, int direction)
{
	int h, w;
	int lm = margins[MARGIN_LEFT], rm = margins[MARGIN_RIGHT], tm = margins[MARGIN_TOP], bm = margins[MARGIN_BOTTOM];
	getmaxyx(win, h, w);
	(void)w;

	int k = 0x10;

	if (direction == UP)
	{
		curY--;
		if (curY < 0)
		{
			if (currentStartAddr > 0)
				currentStartAddr -= k;
			curY = 0;
		}
	}
	else if (direction == DOWN)
	{
		curY++;
		if ((currentStartAddr + curX + sizeMode - 1 + curY * k) < file->content.size)
		{
			if (curY > h - 4 - bm - tm)
			{
				if (currentStartAddr < file->content.size)
					currentStartAddr += k;
				curY = h - 4 - bm - tm;
			}
		}
		else
			curY--;
	}
	else if (direction == RIGHT)
	{
		curX += sizeMode;
		if ((currentStartAddr + curX + sizeMode - 1 + curY * k) < file->content.size)
		{
			if (curX >= k) curX = k - sizeMode;
		}
		else
			curX -= sizeMode;
	}
	else if (direction == LEFT)
	{
		curX -= sizeMode;
		if (curX < 0) curX = 0;
	}
	else if (direction == SHIFT_RIGHT)
	{
		curX += sizeMode;
		if ((currentStartAddr + curX + sizeMode - 1 + curY * k) < file->content.size)
		{
			if (curX >= k)
			{
				curX = 0;
				curY++;
				if ((currentStartAddr + curX + sizeMode - 1 + curY * k) < file->content.size)
				{
					if (curY > h - 4 - bm - tm)
					{
						if (currentStartAddr < file->content.size)
							currentStartAddr += k;
						curY = h - 4 - bm - tm;
					}
				}
				else
					curY--;
			}
		}
		else
			curX -= sizeMode;

	}
	else if (direction == SHIFT_LEFT)
	{
		curX -= sizeMode;
		if (curX < 0)
		{
			curY--;
			if ((currentStartAddr + curY * k) >= 0)
			{
				curX = k - sizeMode;
				if (curY < 0)
				{
					currentStartAddr -= k;
					curY = 0;
				}
			}
			else
			{
				curY++;
				curX += sizeMode;
			}
		}
	}
	fastTypeMode = 0;
}

void gotoAddressMenu(WINDOW* win ,WINDOW* statBar)
{
    int h,w;
    (void) w;
    getmaxyx(win, h, w);

    wclear(statBar);
    echo();
    nocbreak();

    u64 addr = currentStartAddr;
    mvwprintw(statBar, 0, 0, "Go to address: ");
    wscanw(statBar, "%X", &addr);

    if(addr < file->content.size)
    {
		setCursorPosition(addr);
    }
    else
    {
        currentStartAddr = (file->content.size - (h-3)*0x10) & ~0xF;
        curX = 0;
        curY = 0;
        wclear(statBar);
        mvwprintw(statBar, 0, 0, "This address is not in file space! Jumping to end!");
    }

    noecho();
    cbreak();
    wrefresh(statBar);
}

void setCursorPosition(u64 position) {
	if (position < file->content.size) {
		currentStartAddr = position & ~0xF;
		curX = position & 0xF & ~(sizeMode - 1);
		curY = 0;
	}
}

void insertData(unsigned long position, int size)
{
	char data = 0;
	for (long i = position; i < position + size; i++) {
		ArrayList_Insert(&file->content, i, &data);
	}
}

void deleteData(unsigned long position, int size)
{
	if (file->content.size - size < 1)
		return;

	for (int i = position; i < file->content.size - size; i++) {
		ArrayList_Remove(&file->content, position);
	}
}

void searchMenu(int searchMode, WINDOW* win, WINDOW* statBar)
{
	wclear(statBar);
	echo();
	nocbreak();

	char text[TEXT_BUFFER_SIZE];
	int len = TEXT_BUFFER_SIZE;
	u64 addr;
	if (searchMode == SEARCH_TEXT)
	{
		mvwprintw(statBar, 0, 0, "Search text: ");
		wscanw(statBar, MATCH_ANY_PATTERN, text);
		len = strlen(text);
		addr = searchData(currentStartAddr + curX + curY * 0x10, (unsigned char*)text, len);
	}
	else
	{
		mvwprintw(statBar, 0, 0, "Search data: ");
		wscanw(statBar, "%s", text);
		len = strlen(text);
		unsigned char t[64];
		unsigned int i, h1, h2, r;
		for (i = 0; i < len / 2; i++)
		{
			h1 = hexCharToInteger(text[i * 2]);
			h2 = hexCharToInteger(text[i * 2 + 1]);
			r = (h1 << 4) + h2;
			t[i] = r;
		}

		addr = searchData(currentStartAddr + curX + curY * 0x10, t, len / 2);
	}

	if (addr < file->content.size)
	{
		currentStartAddr = addr & ~0xF;
		curX = ((addr & 0xF) / sizeMode)*sizeMode;
		curY = 0;
	}
	else
	{
		wclear(statBar);
		mvwprintw(statBar, 0, 0, "Data was not found!");
	}
	noecho();
	cbreak();
	wrefresh(statBar);
}

u64 searchData(int cursorPos, byte* dat, int datSize)
{
	u64 i, j, t;
	i = cursorPos;
	while (i < file->content.size - datSize)
	{
		j = 0;
		t = i;
		byte* content = ArrayList_Get(&file->content, t);
		while (dat[j] == *content)
		{
			t++;
			j++;
			if (j == datSize)
				return i;
		}
		i++;
	}
	return file->content.size + 1;
}

void addToUndoList(int type, u64 address, u64 prev_val, u64 new_val)
{
	Change* lastUndo = numUndos <= 0 ? NULL : ArrayList_Get(&undoList, numUndos - 1);
    if (type == CHANGE_INCREMENT && lastUndo != NULL && lastUndo->type == CHANGE_INCREMENT && lastUndo->address == address && lastUndo->val_size == sizeMode) {
        lastUndo->new_val = new_val;
    }
    else
    {
		Change newChange;
        newChange.address = address;
		newChange.prev_val = prev_val;
		newChange.new_val = new_val;
		newChange.type = type;
		newChange.val_size = sizeMode;
		newChange.endian = endianMode;
		newChange.saved = file->saved;
		ArrayList_Add(&undoList, &newChange);
        numUndos++;
        numUndoRedos++;
    }
}

void undo(WINDOW* statBar)
{
    if (numUndos <= 0)
        return;

	Change* change = ArrayList_Get(&undoList, numUndos - 1);

	if (change->type == CHANGE_INSERT)
	{
		deleteData(change->address, change->val_size);
	}
	else
	{
		if (change->type == CHANGE_DELETE)
		{
			insertData(change->address, change->val_size);
		}

		//write new value
		int i;
		for (i = 0; i < change->val_size; i++)
		{
			if (change->endian) { //little endian
				byte bt = (change->prev_val >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, change->address + i, &bt);
			}
			else {
				byte bt = (change->prev_val >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, change->address + change->val_size - i - 1, &bt);
			}
		}
	}
    file->saved = change->saved;
    numUndos--;
    mvwprintw(statBar, 0, 0, "Undone!");
    wrefresh(statBar);
}

void redo(WINDOW* statBar)
{
    if (numUndoRedos <= numUndos)
        return;

	Change* change = ArrayList_Get(&undoList, numUndos - 1);

	if (change->type == CHANGE_DELETE)
	{
		deleteData(change->address, change->val_size);
	}
	else
	{
		if (change->type == CHANGE_INSERT)
		{
			insertData(change->address, change->val_size);
		}

		//write new value
		int i;
		for (i = 0; i < change->val_size; i++)
		{
			if (change->endian) { //little endian
				byte bt = (change->new_val >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, change->address + i, &bt);
			}
			else {
				byte bt = (change->new_val >> (i * 8)) & 0xFF;
				ArrayList_Set(&file->content, change->address + change->val_size - i - 1, &bt);
			}
		}
	}
    file->saved = 0;
    numUndos++;
    mvwprintw(statBar, 0, 0, "Redone!");
    wrefresh(statBar);
}

void createPatchFromUndoList(WINDOW* statBar)
{
    if (file->saved > 0 || numUndos == 0)
        return;

    wclear(statBar);
    echo();
    nocbreak();

    char fname[48];
    char path[128];

    mvwprintw(statBar, 0, 0, "Patch name: ");
    wscanw(statBar, "%s", fname);
    strcpy(path, fname);
    strcat(path, ".lua");
    FILE* f = fopen(path, "w");

    // create patch from changes after saving
    int i = numUndos - 1;
    char* lastFile = NULL;
	Change* lastUndo = ArrayList_Get(&undoList, i);
    // find i where we saved
    while (i >= 0)
    {
        if (lastUndo->saved)
            break;
        i--;
    }
    while (i < numUndos)
    {
        int j;
        for(j = 0; j < lastUndo->val_size; j++)
        {
            u64 pos;
            unsigned char content;

			if (lastUndo->endian) { //little endian
				pos = lastUndo->address + j;
			}
			else {
				pos = lastUndo->address + lastUndo->val_size - j - 1;
			}

            content = (lastUndo->new_val >> (j*8)) & 0xFF;
            fprintf(f, "file[0x%llX] = 0x%02X\n", pos, content);
        }
        i++;
    }
    fclose(f);

    cbreak();
    noecho();
    wrefresh(statBar);
}


void getCursorWindowPosition(int* x, int* y) {
	*x = curX / sizeMode * (sizeMode * 2 + 1) + 12;
	*y = curY + 2;
}

void getCursorFilePosition(u64* pos) {
	*pos = currentStartAddr + curX + curY * 0x10;
}