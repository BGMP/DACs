#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

int main(int argc, char* argv[]) {
    FILE* f;
    f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    uint size = ftell(f) / 4;
    fseek(f, 0, SEEK_SET);
    printf("list size: %u\n", size);
    uint* list, i = 0;
    list = (uint*)malloc(sizeof(uint) * size);
    while (feof(f) == 0 && i < size) {
        fread(&list[i], sizeof(uint), 1, f);
        i++;
    }
    FTRep* listRep = createFT(list, size);
    printf("Created FT\n");

    // Verification flag for first 100 elements
    int first100_ok = 1;
    uint limit_first = (size < 100) ? size : 100;

    printf("Accessing first %u elements:\n", limit_first);
    for (i = 0; i < limit_first; i++) {
        uint ft_value = accessFT(listRep, i + 1);  // accessFT uses 1-based indexing
        printf("list[%u]=%u vs accessFT(listRep,%u)=%u\n", i, list[i], i + 1, ft_value);
        if (list[i] != ft_value) {
            first100_ok = 0;
        }
    }

    // Verification flag for last 100 elements
    int last100_ok = 1;
    uint start_last = (size > 100) ? (size - 100) : 0;

    printf("Accessing last %u elements:\n", size - start_last);
    for (i = start_last; i < size; i++) {
        uint ft_value = accessFT(listRep, i + 1);  // accessFT uses 1-based indexing
        printf("list[%u]=%u vs accessFT(listRep,%u)=%u\n", i, list[i], i + 1, ft_value);
        if (list[i] != ft_value) {
            last100_ok = 0;
        }
    }

    printf("\n--- VERIFICATION RESULTS ---\n");
    printf("First %u elements: %s\n", limit_first, first100_ok ? "OK" : "FAILED");
    printf("Last %u elements: %s\n", size - start_last, last100_ok ? "OK" : "FAILED");

    if (first100_ok && last100_ok) {
        printf("\nOK - All verifications passed!\n");
    }
    else {
        printf("\nFAILED - Some verifications failed!\n");
    }

    printf("\n--- ADDITIONAL CHECKS ---\n");
    printf("Position 0 of list: %u\n", list[0]);
    printf("First element of FT: %u\n", accessFT(listRep, 1));
    printf("Last position of list: %u\n", list[size - 1]);
    printf("Last element of FT: %u\n", accessFT(listRep, size));

    printFT(listRep, 1);
    saveFT(listRep, argv[2]);

    free(list);
    fclose(f);

    return 0;
}
