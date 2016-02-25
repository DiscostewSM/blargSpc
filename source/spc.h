#include <3ds.h>

#include "spc700.h"
#include "dsp.h"

typedef struct id666Tag
{
	char songTitle[33];
	char gameTitle[33];
	char dumper[17];
	char comments[33];
	char date[12];
	int playTime;
	int fadeTime;
	char artist[33];
} id666Data;


char * SPC_GetSongTitle(void);
char * SPC_GetGameTitle(void);
char * SPC_GetDumper(void);
char * SPC_GetComments(void);
char * SPC_GetDate(void);
int SPC_GetSongLength(void);
int SPC_GetFadeTime(void);
char * SPC_GetArtist(void);


int SPC_IsInited(void);
int SPC_IsPlaying(void);
int SPC_IsLoaded(void);

void SPC_Update(void);

int SPC_Play(bool restart);
int SPC_Stop(void);

int SPC_Load(char * filename, bool preferBin);
int SPC_Unload(void);

int SPC_Init(void);
int SPC_DeInit(void);