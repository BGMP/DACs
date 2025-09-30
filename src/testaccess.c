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

ushort * optimizationk(uint * list,int listLength, int * nkvalues);

int main(int argc, char* argv[]) {
    double time_acc;
    clock_t begin, end;
    int i;

    time_acc = 0;

    FTRep * rep = loadFT(argv[1]);
    // printFT(rep, 1);

    begin = clock();

    for (i = 0; i < rep->listLength; i++) {
        accessFT(rep,i+1);
    }

    end = clock();
    time_acc = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("%f\n", time_acc);
}
