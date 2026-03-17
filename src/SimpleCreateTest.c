/*
 * SimpleCreateTest.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "constants.h"
#include "dacs.h"

int main(int argc, char* argv[]) {
	uint initial[] = { 194033291, 3844714040, 3327769746, 2880706986, 2986595132 };
	FTRep* rep = createFT(initial, 5);

	printf("Accessing position 1: %u\n", accessFT(rep, 1));
	printf("Accessing position 2: %u\n", accessFT(rep, 2));
	printf("Accessing position 3: %u\n", accessFT(rep, 3));
	printf("Accessing position 4: %u\n", accessFT(rep, 4));
	printf("Accessing position 5: %u\n", accessFT(rep, 5));
}
