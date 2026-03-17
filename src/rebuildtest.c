#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

int main(int argc, char* argv[])
{
    //First Test: Append
    int i, numero;
    uint value, conterrors;
    uint *listOriginal, *listaux;
    double time_decompress, time_compress, time_append;
    clock_t begin, end;
    FTRep* listRep;
    listOriginal = (uint *) malloc(sizeof(uint)*8);
    listOriginal[0] = 1;
    listOriginal[1] = 1;
    listOriginal[2] = 130;
    listOriginal[3] = 2;
    listOriginal[4] = 60;
    listOriginal[5] = 30;
    listOriginal[6] = 30;
    listOriginal[7] = 2;

    listRep = createFT(listOriginal,8);
    printFT(listRep,0);
    printf("Lista con accessFT:\n");
    for(int i=1;i<9;i++) printf("%u ",accessFT(listRep,i));
    printf("\n");

    replaceFT(listRep,1,4);
    replaceFT(listRep,2,4);
    replaceFT(listRep,3,3000);
    replaceFT(listRep,4,2);
    replaceFT(listRep,5,3006);
    replaceFT(listRep,6,3020);
    replaceFT(listRep,7,3);
    replaceFT(listRep,8,2);
    rebuildFT(listRep);
    replaceFT(listRep,6,5020);
    printFT(listRep,0);
    rebuildFT(listRep);
    printFT(listRep,0);
    printf("Lista con accessFT:\n");
    for(int i=1;i<=listRep->listLength;i++) printf("%u ",accessFT(listRep,i));
    printf("\n");
}
