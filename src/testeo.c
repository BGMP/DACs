#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

void printDataFT(FTRep* rep);
int main(int argc, char* argv[])
{
    FTRep* listRep = loadFT(argv[1]);
    printf("size = %u\n",listRep->listLength);
    printFT(listRep,1);
    destroyFT(listRep);
}

void printDataFT(FTRep* rep){
    int largo = rep->listLength;
    for(int i=1;i<=largo;i++){
        printf("%u ",accessFT(rep,i));
    }
    printf("\n");
}