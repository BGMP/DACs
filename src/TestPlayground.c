#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "constants.h"
#include "dacs.h"

void printElementsWithAccess(FTRep* rep) {
    uint n = rep->listLength;

    /* If small enough, print everything */
    if (n <= 200) {
        for (uint i = 1; i <= n; i++) {
            printf("%u ", accessFT(rep, i));
        }
        printf("\n");
        return;
    }

    /* First 100 */
    for (uint i = 1; i <= 100; i++) {
        printf("%u ", accessFT(rep, i));
    }

    printf("... ");

    /* Last 100 */
    for (uint i = n - 100 + 1; i <= n; i++) {
        printf("%u ", accessFT(rep, i));
    }

    printf("\n");
}

int main(int argc, char* argv[]) {
    char* input_file = argv[1];
    FTRep* rep = loadFT(input_file);

    printElementsWithAccess(rep);
    printFT(rep, 1);
}
