#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"
//#define LIMIT 512
//#define SIZE 10

int main(int argc, char* argv[])
{
    double time_append;
    clock_t begin, end;
    FTRep* listRep;

    listRep = loadFT("dblp.FT");

    begin = clock();
    replaceFT(listRep,200,520);
    end = clock();
    time_append = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Replace %f\n", time_append);

    //uint limite = 200;

    //for(int i=1;i<listRep->listLength+1;i++) printf("%u\n",accessFT(listRep,i));
    begin = clock();
    decompressFT(listRep,listRep->listLength);
    end = clock();
    time_append = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Decomp. %f\n", time_append);

    destroyFT(listRep);
}
