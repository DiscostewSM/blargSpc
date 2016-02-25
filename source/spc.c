#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "spc.h"

#include <stdlib.h>

static const char	spcID[] =	{"SNES-SPC700 Sound File Data v0.30"};
static const char	spcTerm[] =	{0x1a, 0x1a, 0x1a, 0x1e};

static const int	spcMixBufSize = 4096;

int spcInited		= 0;
int spcLoaded		= 0;
int spcPrepped		= 0;
int spcPlaying		= 0;


ndspWaveBuf	spcWaveBuf;

s16 * spcBuffer		= NULL;
u32 spcCurSample	= 0;
u32 spcLastSample	= 0;

u8 spcID666[0x100];
id666Data id666;
SPC_Regs_t spcStartRegs;
u8 spcStartDSP[0x100] = {0};
u8 spcStartRAM[0x10040] = {0};

u32 lastSPCPC = 0;



/*void _spcThread(void * arg)
{
	while(!spcExit)
	{
		if(spcPlaying)
		{
		}
		else
		{
		}
	}
	threadExit(0);
}*/

int _isText(char * str, int len)
{
	int c = 0;
	while((c < len && (str[c] >= 0x30 && str[c] <= 0x39)) || str[c]=='/') c++;
	if(c == len || str[c] == '\0')
		return c;
	else
		return -1;
}

void _decodeID666(bool preferBin)
{

	bool isBin = false;
	char song[3];
	char fade[5];
	char date[11];
	int d, m, y;
	memcpy(id666.songTitle, &spcID666[0x2E], 32);
	id666.songTitle[32] = '\0';
	memcpy(id666.gameTitle, &spcID666[0x4E], 32);
	id666.gameTitle[32] = '\0';
	memcpy(id666.dumper, &spcID666[0x6E], 16);
	id666.dumper[16] = '\0';
	memcpy(id666.comments, &spcID666[0x7E], 32);
	id666.comments[32] = '\0';

	memcpy(date, &spcID666[0x9E], 11);
	memcpy(song, &spcID666[0xA9], 3);
	memcpy(fade, &spcID666[0xAC], 5);
	
	int i = _isText(song, 3);
	int j = _isText(fade, 5);
	int k = _isText(date, 11);

	if(!(i | j | k))
	{
		if(spcID666[0] == 1 && spcID666[0] == 1)
			isBin = true;
		else
			isBin = preferBin;
	}
	else if(i != -1 && j != -1)
	{
		if(k > 0)
			isBin = false;
		else if(k == 0)
			isBin = preferBin;
		else if(k == -1)
		{
			if(!((u32*)date)[1])
				isBin = true;
			else
				isBin = false;
		}
	}
	else
		isBin = true;

	if(isBin)
	{
		i = *(u16*)(&song[0]);
		if(i > 959) i = 959;

		j = *(u32*)(&fade[0]);
		if(j > 59999) j = 59999;

		id666.playTime = i;
		id666.fadeTime = j;

		memcpy(id666.artist, &spcID666[0xB0], 32);
		id666.artist[32] = '\0';

		y = *(u16*)(&date[2]);
		m = date[1];
		d = date[0];
	}
	else
	{
		i = atoi(song);
		j = atoi(fade);

		if(i > 959)	i = 959;
		if(j > 59999) j = 59999;

		id666.playTime = i;
		id666.fadeTime = j;

		memcpy(id666.artist, &spcID666[0xB1], 32);
		id666.artist[32] = '\0';

		u8 * t;
		y = m = d = 0;
		if(date[0] != 0)
		{
			t = strchr(date, '/');
			if(t && *(++t) != 0)
			{
				d = atoi(t);
				t = strchr(t, '/');
				if(t && *(++t) != 0)
				{
					y = atoi(t);
					m = atoi(date);
				}
			}
		}
	}
	sprintf(id666.date, "%d/%d/%d", m, d, y);	
}

void _spcPrep(void)
{
	SPC_Reset();	// Also calls DSP_Reset()

	memcpy(SPC_RAM, spcStartRAM, 0x10040);
	memcpy(DSP_MEM, spcStartDSP, 0x100);
	SPC_Regs = spcStartRegs;
	//memcpy(&SPC_Regs, &spcStartRegs, sizeof(SPC_Regs_t));

	spcPrepped = 1;
}

char * SPC_GetSongTitle(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.songTitle;
}

char * SPC_GetGameTitle(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.gameTitle;
}

char * SPC_GetDumper(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.dumper;
}

char * SPC_GetComments(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.comments;
}

char * SPC_GetDate(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.date;
}

int SPC_GetSongLength(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.playTime;
}

int SPC_GetFadeTime(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.fadeTime;
}

char * SPC_GetArtist(void)
{
	if(!SPC_IsLoaded())
		return 0;

	return id666.artist;
}



int SPC_IsInited(void)
{
	return spcInited;
}

int SPC_IsPlaying(void)
{
	return spcPlaying;
}

int SPC_IsLoaded(void)
{
	return spcLoaded;
}

void SPC_Update(void)
{
	if(!SPC_IsPlaying())
		return;

	u32 curSample = ndspChnGetSamplePos(0);
	if(curSample != spcLastSample)
	{
		u32 sampleCnt = curSample;
		if(sampleCnt < spcLastSample)
			sampleCnt += spcMixBufSize;
		sampleCnt -= spcLastSample;
		spcLastSample = curSample;

		if(SPC_Regs.PC != lastSPCPC)
		{
			//printf("PC - %02x, RAM - %01X\n", SPC_Regs.PC, SPC_RAM[SPC_Regs.PC]);
			//printf("A - %01x, X - %01X, Y - %01X, PSW - %01X, SP - %01X\n\n", SPC_Regs.A, SPC_Regs.X, SPC_Regs.Y, SPC_Regs.PSW, SPC_Regs.SP);
			lastSPCPC = SPC_Regs.PC;
		}
		int i;
		for(i = 0; i < sampleCnt; i++)
		{
			DspMixSamplesStereoAsm(1, &spcBuffer[spcCurSample << 1]);
			SPC_Run(6400 * 32);	//CycleRatio * number of cycles per sample
			spcCurSample = (spcCurSample + 1) % spcMixBufSize;
		}
		
	}
}

int SPC_Play(bool restart)
{
	if(!SPC_IsLoaded())
		return 0;

	if(SPC_IsPlaying())
	{
		//if(!restart)
			return 1;
		//else
		//	SPC_Stop();
	}

	//if(restart && !spcPrepped)
	//	_spcPrep();

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;

	memset(&spcWaveBuf, 0, sizeof(ndspWaveBuf));
	ndspChnReset(0);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, 32000.0f);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetMix(0, mix);

	spcWaveBuf.data_vaddr = spcBuffer;
	spcWaveBuf.nsamples = spcMixBufSize;
	spcWaveBuf.looping  = true;
	spcWaveBuf.status = NDSP_WBUF_FREE;

	memset(spcBuffer, 0, spcMixBufSize*2*2);
	DSP_FlushDataCache(spcBuffer, spcMixBufSize*2*2);

	// Set up the new thread

	ndspChnWaveBufAdd(0, &spcWaveBuf);

	spcCurSample = spcMixBufSize >> 1;
	spcLastSample = 0;
	spcPrepped = 0;
	spcPlaying = 1;

	lastSPCPC = 0xFFFFFFFF;

	return 1;
}

int SPC_Stop(void)
{
	if(!SPC_IsPlaying())
		return 0;

	ndspChnWaveBufClear(0);

	memset(spcBuffer, 0, spcMixBufSize*2*2);
	DSP_FlushDataCache(spcBuffer, spcMixBufSize*2*2);

	spcPlaying = 0;

	return 1;
}

int SPC_Load(char * filename, bool preferBin)
{
	if(!SPC_IsInited())
		return 0;
	
	char header[0x25];
	char registers[0x9];
	long fsize = 0;

	FILE * pFile = fopen(filename, "rb");
	if(pFile == NULL)
		return 0;

	fseek(pFile, 0, SEEK_END);
	fsize = ftell(pFile);
	if(fsize < 0x10200)
	{
		fclose(pFile);
		return false;
	}

	fseek(pFile, 0x0, SEEK_SET);
	fread(header, sizeof(char), 0x25, pFile);
	if((strncmp(header, spcID, 28) != 0) || (strncmp(&header[0x21], spcTerm, 2) != 0))
	{
		fclose(pFile);
		return 0;
	}

	// At this point, the file is valid

	SPC_Unload();

	SPC_Reset();

	fseek(pFile, 0x25, SEEK_SET);
	fread(registers, sizeof(char), 0x9, pFile);
	fseek(pFile, 0x0, SEEK_SET);
	fread(spcID666, sizeof(char), 0x100, pFile);
	fseek(pFile, 0x100, SEEK_SET);
	fread(SPC_RAM, sizeof(char), 0x10000, pFile);
	fseek(pFile, 0x10100, SEEK_SET);
	fread(DSP_MEM, sizeof(char), 0x80, pFile);
	fseek(pFile, 0x101C0, SEEK_SET);
	fread(&SPC_RAM[0x10000], sizeof(char), 0x40, pFile);
	fclose(pFile);

	_decodeID666(preferBin);
	
	SPC_Regs.PC = *((u16*)&registers[0x0]);
	SPC_Regs.A = registers[0x2];
	SPC_Regs.X = registers[0x3];
	SPC_Regs.Y = registers[0x4];
	SPC_Regs.PSW = registers[0x5];
	SPC_Regs.SP = registers[0x6] | 0x100;
	SPC_Regs._memoryMap = (u32)SPC_RAM;
	SPC_Regs.nCycles = 0;

	SpcPrepareStateAfterReload();
	DspPrepareStateAfterReload();
	


	/*spcStartRegs.PC = registers[0x0] | (registers[0x1] << 8);
	spcStartRegs.A = registers[0x2];
	spcStartRegs.X = registers[0x3];
	spcStartRegs.Y = registers[0x4];
	spcStartRegs.PSW = registers[0x5];
	spcStartRegs.SP = registers[0x6];
	spcStartRegs._memoryMap = (u32)SPC_RAM;
	spcStartRegs.nCycles = 0;

	_spcPrep();
	*/
	spcLoaded = 1;
	
	return 1;
}

int SPC_Unload(void)
{
	if(SPC_IsLoaded())
		SPC_Stop();

	spcLoaded = 0;

	return 1;
}

int SPC_Init(void)
{
	if(SPC_IsInited())
		return 1;

	if(R_FAILED(ndspInit()))
		return 0;

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspSetOutputCount(1);
	ndspSetMasterVol(1.0f);

	spcBuffer = (s16*)linearAlloc(spcMixBufSize*2*2);
	memset(spcBuffer, 0, spcMixBufSize*2*2);

	spcCurSample = 0;

	spcInited = 1;
	spcLoaded = 0;
	spcPrepped = 0;
	spcPlaying = 0;
	return 1;
}

int SPC_DeInit(void)
{
	if(!SPC_IsInited())
		return 0;

	SPC_Unload();

	linearFree(spcBuffer);
	ndspExit();

	spcInited = 0;
	spcLoaded = 0;
	spcPrepped = 0;
	spcPlaying = 0;

	return 1;
}