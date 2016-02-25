/*
	BlargSPC v0.1

	based on the SPC/DSP cores of blargSNES

	by Discostew (with the base cores made by StapleButter)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>

#include "hidext.h"
#include "dir.h"
#include "spc.h"




PrintConsole bottomScreen, titleScreen, buttonScreen, id666Screen;

int cursorPos = 0;
int scrollPos = 0;



int renderList(int dirPos)
{
	int i = 0;
	int cnt = dirGetEntryCount();
	consoleSelect(&bottomScreen);
	while((i < 30) && (i + dirPos < cnt))
	{
		char * name = dirGetEntry(i + dirPos, false);
		bool isDir = dirIsEntryDir(i + dirPos);
		size_t slen = strlen(name);
		if(slen > 38)
			iprintf("\x1b[%d;2H%s%.35s...", i, (isDir ? CONSOLE_YELLOW : CONSOLE_WHITE), name);
		else
			iprintf("\x1b[%d;2H%s%-38s", i, (isDir ? CONSOLE_YELLOW : CONSOLE_WHITE), name);
		i++;
	}
	while(i < 30)
	{
		iprintf("\x1b[%d;2H%-38c", i, ' ');
		i++;
	}
	return dirPos;
}

inline void clearCursor(u32 pos)
{
	iprintf("\x1b[%d;0H  ", (int)pos);
}

inline void renderCursor(u32 pos)
{
	iprintf("\x1b[%d;0H%s> ", (int)pos, CONSOLE_YELLOW);
}

void navigateDirectory(u32 press, u32 repeat)
{
	bool cursorMove = ((repeat & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) != 0);
	if(cursorMove)
	{
		consoleSelect(&bottomScreen);
		clearCursor(cursorPos - scrollPos);
		
		if(repeat & KEY_UP)
		{
			cursorPos--;
			if(cursorPos < 0) cursorPos = 0;
		}
		else if(repeat & KEY_DOWN)
		{
			cursorPos++;
			if(cursorPos >= dirGetEntryCount()) cursorPos = dirGetEntryCount() - 1;
		}
		else if(repeat & KEY_LEFT)
		{
			cursorPos -= 29;
			if(cursorPos < 0) cursorPos = 0;
		}
		else if(repeat & KEY_RIGHT)
		{
			cursorPos += 29;
			if(cursorPos >= dirGetEntryCount()) cursorPos = dirGetEntryCount() - 1;
		}

		if(cursorPos < scrollPos)
		{
			scrollPos = cursorPos;
			renderList(scrollPos);
		}
		else if(cursorPos >= scrollPos + 30)
		{
			scrollPos = cursorPos - 29;
			renderList(scrollPos);
		}

		renderCursor(cursorPos - scrollPos);
	}
	else if(press & KEY_A)
	{
		if(dirIsEntryDir(cursorPos))
		{
			// Entry is a directory
			if(dirEnterDirectory(cursorPos))
			{
				consoleSelect(&bottomScreen);
				clearCursor(cursorPos - scrollPos);
				cursorPos = scrollPos = 0;
				renderList(0);
				renderCursor(0);
			}
		}
		else
		{
			// Entry is a file

			char * filename = dirGetEntry(cursorPos, false);
			char * dirpath = dirGetCurrentDir(false);
			
			char * file = (char *)linearAlloc(strlen(filename) + strlen(dirpath) + 1);
			strcpy(file, dirpath);
			strcat(file, filename);
			consoleSelect(&id666Screen);
			consoleClear();
			if(SPC_Load(file, false))
			{
				iprintf("Song Title:\n   %s\n\nGame Title:\n   %s\n\nArtist:\n   %s\n\nDumper:\n   %s\n\nComments:\n   %s", SPC_GetSongTitle(), SPC_GetGameTitle(), SPC_GetArtist(), SPC_GetDumper(), SPC_GetComments());
				SPC_Play(false);
			}
			else
			{
				iprintf("Invalid SPC file.");
			}

			linearFree(file);
		}
	}
}

void initializeScreens(void)
{
	
	consoleInit(GFX_TOP, &titleScreen);
	consoleInit(GFX_TOP, &buttonScreen);
	consoleInit(GFX_TOP, &id666Screen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	consoleSetWindow(&titleScreen, 0, 1, 50, 4);
	consoleSetWindow(&buttonScreen, 41, 25, 9, 4);
	consoleSetWindow(&id666Screen, 0, 8, 36, 22);

		
	consoleSelect(&titleScreen);
	consoleClear();
	iprintf("%*s\n\n%*s", 25 + (13/2), "BlargSPC v0.1", 25 + (39/2), "based on the SPC/DSP cores of blargSNES");
	consoleSelect(&buttonScreen);
	consoleClear();
	iprintf("x - Exit\n\nA - Play\nB - Stop");
	consoleSelect(&id666Screen);
	consoleClear();
	consoleSelect(&bottomScreen);
	consoleClear();
	

}


int main(int argc, char **argv) 
{
	int state = 1;

	// Enable 804Mhz mode on New 3DS
	osSetSpeedupEnable(true);
	aptOpenSession();
	APT_SetAppCpuTimeLimit(30); // enables syscore usage
	aptCloseSession();

	gfxInitDefault();
	gfxSet3D(false);

	dirInit();

	initializeScreens();

	consoleSelect(&bottomScreen);
	if(!SPC_Init())
	{
		state = 2;
		
		iprintf("NDSP could not initialize.\n");
	}
	else
	{
		state = 1;
		dirSetExt("spc");
		dirReadDir("/");

		cursorPos = 0;
		scrollPos = 0;

		renderList(scrollPos);
		renderCursor(cursorPos - scrollPos);

		hidKeysSetRepeat(15, 4);
	}
	

	while(state && aptMainLoop())
	{
		//Wait for VBlank
		gspWaitForVBlank();

		hidScanInput();

		//u32 held = hidKeysHeld();
		u32 press = hidKeysDown();
		u32 repeat = hidKeysDownRepeat();
		//u32 release = hidKeysUp();
			
		if(press & KEY_X)
			state = 0;

		if(state == 1)
		{
			if(press & KEY_B)
				SPC_Stop();

			navigateDirectory(press, repeat);

			SPC_Update();
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();		
	}


	gfxExit();

	dirDeInit();

	SPC_DeInit();

    return 0;
}
