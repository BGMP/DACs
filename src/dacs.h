#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <limits.h>
#include "basics.h"
#include "bitrankw32int.h"

typedef struct sFTRep {
    uint listLength;
    byte nLevels;
    uint ** levels;
    bitRankW32Int ** bS;
    uint sizeLastLevel;
    uint * base;
    ushort * base_bits;
} FTRep;

// public:
	FTRep* createFT(uint *list,uint listLength);
	uint accessFT(FTRep * listRep,uint param);
	void saveFT(FTRep * listRep, char * filename);
	uint * decompressFT(FTRep * listRep, uint n);
	FTRep* loadFT(char * filename);
	void destroyFT(FTRep * listRep);
    //dynamics:
    void appendFT(FTRep * rep,uint value);
    void insertFT(FTRep * rep,uint pos, uint value);
    void deleteFT(FTRep * rep,uint pos);
    void replaceFT(FTRep * rep,uint pos, uint value);
    void rebuildFT(FTRep * rep);
    void printFT(FTRep * rep, int breve);
