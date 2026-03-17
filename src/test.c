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
    //printf("Original size = %u\n",memoryUsage(listRep));
    printFT(listRep,0);
    printf("Lista con accessFT:\n");
    for(int i=1;i<9;i++) printf("%u ",accessFT(listRep,i));
    printf("\n");
    saveFT(listRep,"out.FT");

    uint *salida = decompressFT(listRep,listRep->listLength);

    printf("Lista Decomprimida:\n");
    for(int i=0;i<8;i++) printf("%u ",salida[i]);
    printf("\n");

    destroyFT(listRep);
    free(listOriginal);
    free(salida);
//    listRep = loadFT("out.FT");
//    insertFT(listRep,5,1024);
//    printFT(listRep,0);
//    printf("Lista con accessFT:\n");
//    for(int i=1;i<=listRep->listLength;i++) printf("%u ",accessFT(listRep,i));
//    printf("\n");
///*    deleteFT(listRep,5);
//    printFT(listRep,0);
//    printf("Lista con accessFT:\n");
//    for(int i=1;i<=listRep->listLength;i++) printf("%u ",accessFT(listRep,i));
//    printf("\n");*/
//    replaceFT(listRep,2,33);
//    printFT(listRep,0);
//    printf("Lista con accessFT:\n");
//    for(int i=1;i<=listRep->listLength;i++) printf("%u ",accessFT(listRep,i));
//    printf("\n");
//    replaceFT(listRep,3,130);
//    printFT(listRep,0);
//    printf("Lista con accessFT:\n");
//    for(int i=1;i<=listRep->listLength;i++) printf("%u ",accessFT(listRep,i));
//    printf("\n");
}
