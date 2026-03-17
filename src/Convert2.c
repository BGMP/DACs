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
    //First Test: Append
    int i=-1, size;
    uint *list;
    FILE *fptr;
    fptr = fopen(argv[1], "r");
    int buffer;
    char enter;
    while(!feof(fptr)){
        fscanf(fptr,"%u%c",&buffer,&enter);
        i++;
    }
    size = i;
    printf("size=%u\n",size);
    list = (uint *) malloc(sizeof(uint)*size);
    rewind(fptr);
    for(i = 0; i< size; i++){
        fscanf(fptr,"%u%c",&list[i],&enter);
    }
    printf("FLAG3 - Malloc correcto list[0]=%u\n",list[0]);
    
    FTRep* listRep = createFT(list,size);
    printf("FLAG32 - Archivo cargado\n");
    saveFT(listRep,argv[2]);
    printf("FLAG2 - Archivo cargado\n");
    printf("Archivo llenado correctamente\n");
    fclose(fptr);
    free(list);
    destroyFT(listRep);
}

void printDataFT(FTRep* rep){
    int largo = rep->listLength;
    for(int i=1;i<=largo;i++){
        printf("%u ",accessFT(rep,i));
    }
    printf("\n");
}