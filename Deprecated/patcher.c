#include "patcher.h"
#include "globals.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_PATCHES 100
#define MAX_PATCHNAME_LEN 96

char* patchfiles[MAX_PATCHES];
int num_files;
int start;
int cur;
int* saved;
unsigned char* fileContent;
char* filename;
int fileSize;

int patch(const char* patchname)
{
    FILE* fp = fopen(patchname, "rt");
    if(!fp)
        return 0;
    unsigned long long offset = 0;
    unsigned int val = 0;
    char line[128];
    while(fgets(line, 96, fp))
    {
        // line comment
        if (line[0] == '#')
            continue;
    
        // parse offset value pair
        sscanf(line, "%llX %X", &offset, &val);

		if (offset < fileSize)
			fileContent[offset] = val;
		else
			return 0;
    }
    *saved = 0;
    fclose(fp);
    return 2;
}

int patcherInput(int ch, WINDOW* win, WINDOW* statBar)
{
    int w, h;
    (void) w;
    getmaxyx(win, h, w);
    switch(ch)
    {
    //cursor movement, arrow keys
    case KEY_UP:
        cur--;
        if(cur < 1)
        {
            start--;
            if(start < 0)
                start = 0;
            cur = 0;
        }
        break;
    case KEY_DOWN:
        cur++;
        if(cur >= num_files)
            cur--;
        if(cur >= h)
        {
            start++;
            cur = h-1;
        }
        break;
    case 'a':
    case '\n':
        wclear(statBar);
        echo();
        nocbreak();
        char* text = (char*) malloc(sizeof(char)*TEXT_BUFFER_SIZE);
        mvwprintw(statBar, 0, 0, "Are you sure to apply the patch? (y or n)");
        wscanw(statBar, "%s", text);
        if(strcmp(text,"y") == 0)
        {
            wclear(statBar);
            if(patch(patchfiles[start+cur]))
                mvwprintw(statBar, 0, 0, "Patched!");
            else
                mvwprintw(statBar, 0, 0, "Patch failed!");
        }
        noecho();
        cbreak();
        wrefresh(statBar);
    }
    return ch;
}

void patcherDraw(WINDOW* win, WINDOW* statBar)
{
    int w, h, i;
    (void) w;

    getmaxyx(win, h, w);

    for(i = 1; i < h; i++)
    {
        if(start + i-1 >= num_files)
            break;

        if(cur == i-1)
            wattron(win, COLOR_PAIR(2));
        else
            wattron(win, COLOR_PAIR(1));

        mvwprintw(win, i, 1, "%s", patchfiles[start + i-1]);

        if(cur == i-1)
            wattroff(win, COLOR_PAIR(2));
        else
            wattroff(win, COLOR_PAIR(1));
    }

    wrefresh(win);
    box(win, 0, 0);
}

void patcherLoad(Editor* e, FileObject* file, WINDOW* win, WINDOW* statBar)
{
    saved = file->saved;
    fileContent  = file->content;
    filename = file->name;
	fileSize = file->size;
    FILE* patchlist = NULL;
    patchlist = fopen("patches/patches","rt");
    if(!patchlist)
        return;

    num_files = 0;
    start = 0;
    cur = 0;
    while(!feof(patchlist))
    {
        char* line = malloc(sizeof(char) * MAX_PATCHNAME_LEN);
        fscanf(patchlist, "%s\n", line);
        patchfiles[num_files] = line;
        num_files++;
    }
    fclose(patchlist);
}

void patcherUnload(WINDOW* win, WINDOW* statBar)
{
	/*for (int i = 0; i < num_files; i++)
	{
		free(patchfiles[i]);
	}*/
}
