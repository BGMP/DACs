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
    appendFT(listRep,1050);
    end = clock();
    time_append = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("Append %f\n", time_append);

    uint limite = listRep->listLength;

    for(int i=limite-5;i<=limite;i++) accessFT(listRep,i);

    begin = clock();
    for(int i=1;i<=limite;i++) accessFT(listRep,i);
    end = clock();
    time_append = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("Access %f\n", time_append);

    for(int i=1;i<=limite;i++) printf("%u\n",accessFT(listRep,i));

    destroyFT(listRep);
}
