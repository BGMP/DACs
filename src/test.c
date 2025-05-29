/*-----------------------------------------------------------------------
 File       : Test. main file with timing measurements.
 ------------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/time.h>

#include "constants.h"
#include "dacs.h"

/*------------------------------------------------------------------
 Obtains the size of the file to compress.
 ------------------------------------------------------------------ */
unsigned long fileSize (char *filename) {
  FILE *fpText;
  unsigned long fsize;
  fpText = fopen(filename,"rb");
  fsize=0;
  if (fpText) {
    fseek(fpText,0,2);
    fsize= ftell(fpText);
    fclose(fpText);
  }
  return fsize;
}

/* Shows a message with info of this version of the code */
void showMessage() {
  printf("\n*Directly Addressable Codes (DACs)");
  printf("\n*CopyRight(c)2011  Nieves R. Brisaboa, Susana Ladra and Gonzalo Navarro\n");

  printf("\n  #DACs come with ABSOLUTELY NO WARRANTY; ");
  printf("\n  #This is free software, and you are welcome to redistribute it ");
  printf("\n  #under certain conditions\n\n");
}

/*------------------------------------------------------------------
 Get high precision time in seconds
 ------------------------------------------------------------------ */
double getTime() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/*------------------------------------------------------------------
 Saves timing results to CSV file
 ------------------------------------------------------------------ */
void saveTimingResults(const char* filename, uint listLength, uint ftrepSize, double decompressTime, double accessTime) {
  FILE *csvFile;
  int fileExists = 0;

  // Check if file exists
  csvFile = fopen("timing_results.csv", "r");
  if (csvFile != NULL) {
    fileExists = 1;
    fclose(csvFile);
  }

  // Open file for appending
  csvFile = fopen("timing_results.csv", "a");
  if (csvFile == NULL) {
    fprintf(stderr, "Error: Cannot open CSV file for writing\n");
    return;
  }

  // Write header if file is new
  if (!fileExists) {
    fprintf(csvFile, "Input_File,List_Length,FTRep_Size_Bytes,decompressFT_Time_Seconds,accessFT_Time_Seconds\n");
  }

  // Write timing data
  fprintf(csvFile, "%s,%u,%u,%.9f,%.9f\n", filename, listLength, ftrepSize, decompressTime, accessTime);

  fclose(csvFile);
  printf("Timing results saved to timing_results.csv\n");
}

/*------------------------------------------------------------------ */
/*
 *  TEST
 */
/*------------------------------------------------------------------ */

int main(int argc, char* argv[])
{

  uint * list, bytesFile, listLength,i;
  FILE * flist;
  double start_time, end_time;
  double decompress_time, access_time;

  showMessage();

  if (argc < 3) {
    fprintf(stderr,"\n Wrong call to main program\n");
    fprintf(stderr," Use:  %s  <integer list file>  <out file>\n",argv[0]);
    return 0;
  }

  bytesFile = fileSize(argv[1]);
  listLength = bytesFile / sizeof(uint);
  if (!bytesFile) {
    fprintf(stderr, "\nFILE EMPTY OR FILE NOT FOUND !!\nConstruction aborted\n");
    exit(0);
  }

  flist = fopen(argv[1],"r");
  list = (uint *) malloc(sizeof(uint)*listLength);
  fread(list,sizeof(uint),listLength,flist);
  fclose(flist);

  printf("Creating DACs structure for %u elements...\n", listLength);
  FTRep* listRep = createFT(list,listLength);

  // Get structure size
  uint ftrepSize = memoryUsage(listRep);
  printf("FTRep structure size: %u bytes\n", ftrepSize);

  uint * listaux;

  /*Test: decompression with timing*/
  printf("Starting decompression timing test...\n");
  start_time = getTime();
  listaux = decompressFT(listRep,listLength);
  end_time = getTime();

  decompress_time = end_time - start_time;
  printf("Decompression completed in %.9f seconds\n", decompress_time);

  uint conterrors = 0;
  for(i=0;i<listLength;i++)
    if(listaux[i]!=list[i]){
      fprintf(stderr,"Error on decompressFT, the %d-th decompressed element does not match with the original list\n",i+1);
      conterrors ++;
    }
  fprintf(stderr," Test: decompression -> %d mismatches\n", conterrors);

  free(listaux);

  /*Test: direct access with timing*/
  printf("Starting direct access timing test...\n");
  uint value;
  conterrors = 0;

  start_time = getTime();
  for(i=0;i<listLength;i++){
    value = accessFT(listRep, i+1);
    if(value!=list[i]){
      conterrors ++;
      fprintf(stderr,"Error on accessFT, the %d-th accessed element does not match with the original list\n",i+1);
    }
  }
  end_time = getTime();

  access_time = end_time - start_time;
  printf("Direct access test completed in %.9f seconds\n", access_time);
  fprintf(stderr," Test: direct access -> %d mismatches\n", conterrors);

  // Save timing results to CSV
  saveTimingResults(argv[1], listLength, ftrepSize, decompress_time, access_time);

  saveFT(listRep,argv[2]);
  fprintf(stderr,"\n\n Representation saved.\n");

  destroyFT(listRep);
  free(list);

  return 0;
}