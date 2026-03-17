#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

ushort * optimizationk(uint * list,int listLength, int * nkvalues);

int main(int argc, char* argv[])
{
    FILE *f;
    f = fopen(argv[1],"rb");
    fseek(f, 0, SEEK_END);
    uint size = ftell(f)/4;
    fseek(f, 0, SEEK_SET);
    printf("size %u\n",size);

    uint *list,i=0;
    list = (uint*) malloc(sizeof(uint)*size);

    while(feof(f)==0 && i < size){
        fread(&list[i], sizeof(uint), 1, f);
        i++;
    }

    FTRep * listRep = createFT(list,size);
    printFT(listRep,1);
    saveFT(listRep,argv[2]);
}
