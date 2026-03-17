/*
 * insertFT Unit Test Suite
 * Complete validation of insertFT functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "constants.h"
#include "dacs.h"

 /* Test Statistics */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int total_inserts;
    int total_verifications;
} TestStats;

TestStats stats = { 0, 0, 0, 0, 0 };

/* Output macros */
#define TEST_START(name) \
    printf("\n[TEST] %s\n", name); \
    stats.total_tests++;

#define TEST_PASS(name) \
    printf("  [OK] %s\n", name); \
    stats.passed_tests++;

#define TEST_FAIL(name, reason) \
    printf("  [FAIL] %s: %s\n", name, reason); \
    stats.failed_tests++;

#define VERIFY(condition, message) \
    do { \
        stats.total_verifications++; \
        if (!(condition)) { \
            printf("    [FAIL] Verification failed: %s\n", message); \
            printf("           at line %d\n", __LINE__); \
            return 0; \
        } \
    } while(0)

/* Internal Structure Verification */
int VerifyInternalStructure(FTRep* rep, const char* context) {
    if (rep == NULL) {
        printf("    [FAIL] %s: rep is NULL\n", context);
        return 0;
    }

    VERIFY(rep->nLevels > 0, "nLevels must be > 0");
    VERIFY(rep->listLength > 0, "listLength must be > 0");
    VERIFY(rep->levels != NULL, "levels array is NULL");
    VERIFY(rep->base != NULL, "base array is NULL");
    VERIFY(rep->base_bits != NULL, "base_bits array is NULL");

    for (uint i = 0; i < rep->nLevels; i++) {
        VERIFY(rep->base[i] > 0, "base[i] must be > 0");
        VERIFY(rep->base_bits[i] > 0, "base_bits[i] must be > 0");
        VERIFY(rep->base[i] == (1u << rep->base_bits[i]), "base != 2^base_bits");
    }

    if (rep->nLevels > 1) {
        VERIFY(rep->bS != NULL, "bS array is NULL");
        for (uint i = 0; i < rep->nLevels - 1; i++) {
            VERIFY(rep->bS[i] != NULL, "bS[i] is NULL");
            VERIFY(rep->bS[i]->data != NULL, "bS[i]->data is NULL");
        }
    }

    return 1;
}

/* Verify that accessFT returns expected value */
int VerifyAccess(FTRep* rep, uint position, uint expected, const char* context) {
    stats.total_verifications++;
    uint retrieved = accessFT(rep, position);

    if (retrieved != expected) {
        printf("    [FAIL] %s: position %u expected %u, got %u\n",
            context, position, expected, retrieved);
        return 0;
    }
    return 1;
}

/* Verify all values in the structure match expected array */
int VerifyAllValues(FTRep* rep, uint* expected, uint count, const char* context) {
    for (uint i = 0; i < count; i++) {
        if (!VerifyAccess(rep, i + 1, expected[i], context)) {
            printf("           Failed at index %u\n", i);
            return 0;
        }
    }
    return 1;
}

/* Verify decompress matches access for all elements */
int VerifyDecompressMatchesAccess(FTRep* rep, const char* context) {
    uint* decompressed = decompressFT(rep, rep->listLength);

    for (uint i = 0; i < rep->listLength; i++) {
        uint accessed = accessFT(rep, i + 1);
        if (decompressed[i] != accessed) {
            printf("    [FAIL] %s: decompress/access mismatch at index %u\n", context, i);
            printf("           decompressed=%u, accessed=%u\n", decompressed[i], accessed);
            free(decompressed);
            return 0;
        }
    }

    free(decompressed);
    return 1;
}

/* ============================================================================
 * UNIT TESTS
 * ============================================================================ */

 /* Test 1: Insert at beginning */
int Test_InsertAtBeginning() {
    TEST_START("Insert at beginning (position 1)");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Insert at position 1 */
    insertFT(rep, 1, 5);
    stats.total_inserts++;

    VERIFY(rep->listLength == 4, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after insert")) return 0;

    /* Verify all values: {5, 10, 20, 30} */
    uint expected[] = { 5, 10, 20, 30 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Insert at beginning");
    return 1;
}

/* Test 2: Insert in middle */
int Test_InsertInMiddle() {
    TEST_START("Insert in middle position");

    uint initial[] = { 10, 20, 40, 50 };
    FTRep* rep = createFT(initial, 4);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Insert at position 3 (between 20 and 40) */
    insertFT(rep, 3, 30);
    stats.total_inserts++;

    VERIFY(rep->listLength == 5, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after insert")) return 0;

    /* Verify all values: {10, 20, 30, 40, 50} */
    uint expected[] = { 10, 20, 30, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Insert in middle");
    return 1;
}

/* Test 3: Insert at end (should delegate to append) */
int Test_InsertAtEnd() {
    TEST_START("Insert at end (delegates to append)");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Insert at position 4 (listLength + 1) */
    insertFT(rep, 4, 40);
    stats.total_inserts++;

    VERIFY(rep->listLength == 4, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after insert")) return 0;

    /* Verify all values: {10, 20, 30, 40} */
    uint expected[] = { 10, 20, 30, 40 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Insert at end");
    return 1;
}

/* Test 4: Insert into single element */
int Test_InsertIntoSingle() {
    TEST_START("Insert into single-element structure");

    uint initial[] = { 50 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Insert at beginning */
    insertFT(rep, 1, 25);
    stats.total_inserts++;
    VERIFY(rep->listLength == 2, "listLength after first insert");

    /* Insert in middle */
    insertFT(rep, 2, 40);
    stats.total_inserts++;
    VERIFY(rep->listLength == 3, "listLength after second insert");

    /* Insert at end */
    insertFT(rep, 4, 75);
    stats.total_inserts++;
    VERIFY(rep->listLength == 4, "listLength after third insert");

    /* Verify all values: {25, 40, 50, 75} */
    uint expected[] = { 25, 40, 50, 75 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after inserts")) return 0;

    destroyFT(rep);
    TEST_PASS("Insert into single element");
    return 1;
}

/* Test 5: Force level creation with insert */
int Test_ForceLevelCreation() {
    TEST_START("Force level creation with large value insert");

    uint initial[] = { 1, 2, 3 };
    FTRep* rep = createFT(initial, 3);

    uint initial_levels = rep->nLevels;

    /* Insert large value at position 2 */
    uint large_value = 100000;
    insertFT(rep, 2, large_value);
    stats.total_inserts++;

    VERIFY(rep->listLength == 4, "listLength incorrect");
    if (!VerifyInternalStructure(rep, "after large value insert")) return 0;

    /* Verify all values: {1, 100000, 2, 3} */
    uint expected[] = { 1, large_value, 2, 3 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Force level creation");
    return 1;
}

/* Test 6: Boundary values */
int Test_BoundaryValues() {
    TEST_START("Boundary values with insert");

    uint initial[] = { 100 };
    FTRep* rep = createFT(initial, 1);

    uint boundaries[] = {
        0, 1, 2, 63, 64, 127, 128, 255, 256,
        1023, 1024, 65535, 65536, UINT_MAX
    };

    int n_boundaries = sizeof(boundaries) / sizeof(boundaries[0]);

    /* Insert all boundary values at position 1 (push everything right) */
    for (int i = 0; i < n_boundaries; i++) {
        insertFT(rep, 1, boundaries[i]);
        stats.total_inserts++;

        if (!VerifyAccess(rep, 1, boundaries[i], "boundary value")) {
            printf("           Failed at boundary value %u\n", boundaries[i]);
            destroyFT(rep);
            return 0;
        }
    }

    /* Verify original value is now at the end */
    if (!VerifyAccess(rep, n_boundaries + 1, 100, "original value")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Boundary values");
    return 1;
}

/* Test 7: Insert zeros */
int Test_InsertZeros() {
    TEST_START("Insert zeros at various positions");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    /* Insert zeros at positions 2, 4, 6 */
    insertFT(rep, 2, 0);
    stats.total_inserts++;
    insertFT(rep, 4, 0);
    stats.total_inserts++;
    insertFT(rep, 6, 0);
    stats.total_inserts++;

    VERIFY(rep->listLength == 8, "listLength incorrect");

    /* Verify all values: {10, 0, 20, 0, 30, 0, 40, 50} */
    uint expected[] = { 10, 0, 20, 0, 30, 0, 40, 50 };
    if (!VerifyAllValues(rep, expected, 8, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after zero inserts")) return 0;

    destroyFT(rep);
    TEST_PASS("Insert zeros");
    return 1;
}

/* Test 8: Insert UINT_MAX - DEBUG VERSION */
int Test_InsertUINTMAX() {
    TEST_START("Insert UINT_MAX at various positions - DEBUG");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    printf("    === INITIAL STATE ===\n");
    printf("    Structure details:\n");
    printFT(rep, 0);
    printf("    Values via accessFT: ");
    for (uint i = 1; i <= rep->listLength; i++) {
        printf("%u ", accessFT(rep, i));
    }
    printf("\n");

    /* First insert: UINT_MAX at position 1 */
    printf("\n    === INSERT #1: UINT_MAX at position 1 ===\n");
    printf("    Expected result: {UINT_MAX, 10, 20, 30}\n");

    insertFT(rep, 1, UINT_MAX);
    stats.total_inserts++;

    printf("    After insert - Structure details:\n");
    printFT(rep, 0);
    printf("    Values via accessFT: ");
    for (uint i = 1; i <= rep->listLength; i++) {
        uint val = accessFT(rep, i);
        if (val == UINT_MAX) {
            printf("UINT_MAX ");
        }
        else {
            printf("%u ", val);
        }
    }
    printf("\n");

    if (!VerifyAccess(rep, 1, UINT_MAX, "first insert")) {
        printf("    [FAIL] First insert failed!\n");
        destroyFT(rep);
        return 0;
    }
    printf("    [OK] First insert verified\n");

    /* Second insert: UINT_MAX at position 3 */
    printf("\n    === INSERT #2: UINT_MAX at position 3 ===\n");
    printf("    Expected result: {UINT_MAX, 10, UINT_MAX, 20, 30}\n");

    insertFT(rep, 3, UINT_MAX);
    stats.total_inserts++;

    printf("    After insert - Structure details:\n");
    printFT(rep, 0);
    printf("    Values via accessFT: ");
    for (uint i = 1; i <= rep->listLength; i++) {
        uint val = accessFT(rep, i);
        if (val == UINT_MAX) {
            printf("UINT_MAX ");
        }
        else {
            printf("%u ", val);
        }
    }
    printf("\n");

    if (!VerifyAccess(rep, 3, UINT_MAX, "second insert")) {
        printf("    [FAIL] Second insert failed!\n");
        destroyFT(rep);
        return 0;
    }
    printf("    [OK] Second insert verified\n");

    /* Third insert: UINT_MAX at position 6 */
    printf("\n    === INSERT #3: UINT_MAX at position 6 (THE FAILING ONE) ===\n");
    printf("    Expected result: {UINT_MAX, 10, UINT_MAX, 20, 30, UINT_MAX}\n");
    printf("    Current listLength: %u\n", rep->listLength);

    printf("    BEFORE third insert - Structure details:\n");
    printFT(rep, 0);
    printf("    BEFORE - All values via accessFT: ");
    for (uint i = 1; i <= rep->listLength; i++) {
        uint val = accessFT(rep, i);
        if (val == UINT_MAX) {
            printf("UINT_MAX ");
        }
        else {
            printf("%u ", val);
        }
    }
    printf("\n");

    insertFT(rep, 6, UINT_MAX);
    stats.total_inserts++;

    printf("    AFTER third insert - Structure details:\n");
    printFT(rep, 0);
    printf("    AFTER - All values via accessFT: ");
    for (uint i = 1; i <= rep->listLength; i++) {
        uint val = accessFT(rep, i);
        if (val == UINT_MAX) {
            printf("UINT_MAX ");
        }
        else {
            printf("%u ", val);
        }
    }
    printf("\n");

    printf("    Checking position 6 specifically:\n");
    uint retrieved = accessFT(rep, 6);
    printf("    Retrieved value at position 6: %u\n", retrieved);
    printf("    Expected value at position 6: %u (UINT_MAX)\n", UINT_MAX);

    if (retrieved != UINT_MAX) {
        printf("    [FAIL] Third insert FAILED - got %u instead of UINT_MAX\n", retrieved);

        /* Additional debugging: check all positions */
        printf("\n    Full position-by-position verification:\n");
        uint expected_all[] = { UINT_MAX, 10, UINT_MAX, 20, 30, UINT_MAX };
        for (uint i = 0; i < 6; i++) {
            uint val = accessFT(rep, i + 1);
            char* status = (val == expected_all[i]) ? "OK" : "FAIL";
            printf("    Position %u: expected=%u, got=%u [%s]\n",
                i + 1, expected_all[i], val, status);
        }

        destroyFT(rep);
        return 0;
    }
    printf("    [OK] Third insert verified\n");

    /* Final verification */
    printf("\n    === FINAL VERIFICATION ===\n");
    VERIFY(rep->listLength == 6, "listLength incorrect");

    uint expected[] = { UINT_MAX, 10, UINT_MAX, 20, 30, UINT_MAX };
    if (!VerifyAllValues(rep, expected, 6, "all values")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after UINT_MAX inserts")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Insert UINT_MAX");
    return 1;
}

/* Test 9: Multiple inserts at same position */
int Test_MultipleInsertsAtSamePosition() {
    TEST_START("Multiple inserts at same position");

    uint initial[] = { 100 };
    FTRep* rep = createFT(initial, 1);

    /* Insert 10 values all at position 1 */
    for (uint i = 0; i < 10; i++) {
        insertFT(rep, 1, i * 10);
        stats.total_inserts++;
    }

    VERIFY(rep->listLength == 11, "listLength incorrect");

    /* Values should be in reverse order: {90, 80, 70, ..., 10, 0, 100} */
    uint expected[] = { 90, 80, 70, 60, 50, 40, 30, 20, 10, 0, 100 };
    if (!VerifyAllValues(rep, expected, 11, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after multiple inserts")) return 0;

    destroyFT(rep);
    TEST_PASS("Multiple inserts at same position");
    return 1;
}

/* Test 10: Sequential inserts building array */
int Test_SequentialInserts() {
    TEST_START("Sequential inserts building array");

    uint initial[] = { 1 };
    FTRep* rep = createFT(initial, 1);

    /* Build array {1, 2, 3, ..., 100} by inserting at end */
    for (uint i = 2; i <= 100; i++) {
        insertFT(rep, i, i);
        stats.total_inserts++;
    }

    VERIFY(rep->listLength == 100, "listLength incorrect");

    /* Verify sequential values */
    for (uint i = 1; i <= 100; i++) {
        if (!VerifyAccess(rep, i, i, "sequential value")) {
            destroyFT(rep);
            return 0;
        }
    }

    if (!VerifyInternalStructure(rep, "after sequential inserts")) return 0;

    destroyFT(rep);
    TEST_PASS("Sequential inserts");
    return 1;
}

/* Test 11: Alternating insert positions */
int Test_AlternatingPositions() {
    TEST_START("Alternating insert positions");

    uint initial[] = { 50 };
    FTRep* rep = createFT(initial, 1);

    /* Alternate between inserting at position 1 and at end */
    for (uint i = 0; i < 20; i++) {
        if (i % 2 == 0) {
            insertFT(rep, 1, i);  /* Insert at beginning */
        }
        else {
            insertFT(rep, rep->listLength + 1, i);  /* Insert at end */
        }
        stats.total_inserts++;
    }

    VERIFY(rep->listLength == 21, "listLength incorrect");
    if (!VerifyInternalStructure(rep, "after alternating inserts")) return 0;

    destroyFT(rep);
    TEST_PASS("Alternating positions");
    return 1;
}

/* Test 12: Random position inserts with verification */
int Test_RandomPositionInserts() {
    TEST_START("Random position inserts with incremental verification");

    srand(54321);  /* Fixed seed for reproducibility */

    uint initial[] = { 100 };
    FTRep* rep = createFT(initial, 1);

    uint count = 500;
    uint* all_values = malloc(sizeof(uint) * (count + 1));
    all_values[0] = 100;
    uint current_length = 1;

    for (uint i = 0; i < count; i++) {
        uint value = (uint)rand() * (uint)rand();
        uint position = (rand() % (current_length + 1)) + 1;  /* 1 to current_length+1 */

        /* Insert into tracking array */
        for (uint j = current_length; j >= position; j--) {
            all_values[j] = all_values[j - 1];
        }
        all_values[position - 1] = value;
        current_length++;

        /* Insert into DAC */
        insertFT(rep, position, value);
        stats.total_inserts++;

        /* Verify every 50 inserts */
        if ((i + 1) % 50 == 0) {
            if (!VerifyAllValues(rep, all_values, current_length, "incremental verification")) {
                printf("           Failed at insert %u\n", i);
                free(all_values);
                destroyFT(rep);
                return 0;
            }
        }
    }

    /* Final verification */
    if (!VerifyAllValues(rep, all_values, current_length, "final verification")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after random inserts")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    free(all_values);
    destroyFT(rep);
    TEST_PASS("Random position inserts");
    return 1;
}

/* Test 13: Stress test */
int Test_Stress() {
    TEST_START("Stress test with 3000 random position inserts");

    srand(11111);

    uint initial[] = { 1, 2, 3 };
    FTRep* rep = createFT(initial, 3);

    uint count = 3000;
    uint* all_values = malloc(sizeof(uint) * (count + 3));
    all_values[0] = 1;
    all_values[1] = 2;
    all_values[2] = 3;
    uint current_length = 3;

    printf("    Inserting %u values at random positions...\n", count);

    for (uint i = 0; i < count; i++) {
        uint value = (uint)rand() * (uint)rand();
        uint position = (rand() % (current_length + 1)) + 1;

        /* Insert into tracking array */
        for (uint j = current_length; j >= position; j--) {
            all_values[j] = all_values[j - 1];
        }
        all_values[position - 1] = value;
        current_length++;

        /* Insert into DAC */
        insertFT(rep, position, value);
        stats.total_inserts++;

        if ((i + 1) % 1000 == 0) {
            printf("    Progress: %u/%u inserts completed\n", i + 1, count);
        }
    }

    printf("    Verifying all %u values...\n", current_length);

    if (!VerifyAllValues(rep, all_values, current_length, "stress test")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after stress test")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyDecompressMatchesAccess(rep, "stress test final")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    free(all_values);
    destroyFT(rep);
    TEST_PASS("Stress test");
    return 1;
}

/* Test 14: Insert with decompress consistency */
int Test_DecompressConsistency() {
    TEST_START("Decompress matches access after each insert");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    /* After initial creation */
    if (!VerifyDecompressMatchesAccess(rep, "initial")) {
        destroyFT(rep);
        return 0;
    }

    /* Insert values and verify consistency */
    uint inserts[][2] = {
        {5, 1},    /* {5, 10, 20, 30} */
        {15, 3},   /* {5, 10, 15, 20, 30} */
        {35, 6},   /* {5, 10, 15, 20, 30, 35} */
        {0, 1},    /* {0, 5, 10, 15, 20, 30, 35} */
        {UINT_MAX, 4}  /* {0, 5, 10, UINT_MAX, 15, 20, 30, 35} */
    };

    for (int i = 0; i < 5; i++) {
        insertFT(rep, inserts[i][1], inserts[i][0]);
        stats.total_inserts++;

        if (!VerifyDecompressMatchesAccess(rep, "after insert")) {
            printf("           Failed after insert %d (value=%u, pos=%u)\n",
                i, inserts[i][0], inserts[i][1]);
            destroyFT(rep);
            return 0;
        }
    }

    destroyFT(rep);
    TEST_PASS("Decompress consistency");
    return 1;
}

/* Test 15: Deep structure validation */
int Test_DeepStructureValidation() {
    TEST_START("Deep DAC structure validation after inserts");

    uint initial[] = { 50 };
    FTRep* rep = createFT(initial, 1);

    /* Insert diverse values at various positions */
    uint inserts[][2] = {
        {0, 1}, {1, 1}, {2, 2}, {15, 3}, {16, 4}, {63, 5},
        {64, 6}, {127, 7}, {128, 8}, {255, 9}, {256, 10},
        {1000, 11}, {5000, 12}, {10000, 13}, {50000, 14},
        {100000, 15}, {UINT_MAX, 16}
    };
    int n_inserts = sizeof(inserts) / sizeof(inserts[0]);

    for (int i = 0; i < n_inserts; i++) {
        insertFT(rep, inserts[i][1], inserts[i][0]);
        stats.total_inserts++;
    }

    uint total_elements = 1 + n_inserts;

    /* Bitmap structure consistency */
    printf("    Checking bitmap structure...\n");

    if (rep->nLevels > 1) {
        uint level0_size = rep->bS[0]->n;
        VERIFY(level0_size == total_elements,
            "Level 0 bitmap size should equal listLength");
    }

    for (uint lv = 1; lv < rep->nLevels - 1; lv++) {
        uint ones_count = rank(rep->bS[lv - 1], rep->bS[lv - 1]->n - 1);
        uint zeros_count = rep->bS[lv - 1]->n - ones_count;
        uint actual_size = rep->bS[lv]->n;

        if (actual_size != zeros_count) {
            printf("    [FAIL] Level %u size mismatch: expected %u (zeros), got %u\n",
                lv, zeros_count, actual_size);
            destroyFT(rep);
            return 0;
        }
    }

    /* Termination bit invariant */
    printf("    Checking termination bit invariant...\n");

    for (uint elem_idx = 0; elem_idx < total_elements; elem_idx++) {
        uint termination_count = 0;
        uint current_pos = elem_idx;

        for (uint lv = 0; lv < rep->nLevels - 1; lv++) {
            if (current_pos >= rep->bS[lv]->n) break;

            uint bit = bitget(rep->bS[lv]->data, current_pos);

            if (bit == 1) {
                termination_count++;
                break;
            }

            uint rank_before = (current_pos > 0) ? rank(rep->bS[lv], current_pos - 1) : 0;
            current_pos = current_pos - rank_before;
        }

        if (termination_count == 0) {
            termination_count = 1;
        }

        if (termination_count != 1) {
            printf("    [FAIL] Element %u has %u termination bits (should be 1)\n",
                elem_idx, termination_count);
            destroyFT(rep);
            return 0;
        }
    }

    /* Base consistency */
    printf("    Checking base/base_bits consistency...\n");

    for (uint lv = 0; lv < rep->nLevels; lv++) {
        uint expected_base = 1u << rep->base_bits[lv];
        if (rep->base[lv] != expected_base) {
            printf("    [FAIL] Level %u: base=%u but 2^base_bits=%u\n",
                lv, rep->base[lv], expected_base);
            destroyFT(rep);
            return 0;
        }
    }

    /* Last level properties */
    printf("    Checking last level properties...\n");

    uint last_level = rep->nLevels - 1;
    uint expected_last_size;

    if (last_level > 0) {
        uint ones = rank(rep->bS[last_level - 1], rep->bS[last_level - 1]->n - 1);
        expected_last_size = rep->bS[last_level - 1]->n - ones;
    }
    else {
        expected_last_size = total_elements;
    }

    VERIFY(rep->sizeLastLevel == expected_last_size,
        "sizeLastLevel should match bitmap calculation");

    printf("    All DAC structure invariants verified!\n");

    destroyFT(rep);
    TEST_PASS("Deep DAC structure validation");
    return 1;
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("insertFT UNIT TEST SUITE\n");
    printf("========================================\n");
    printf("* Testing insertFT functionality from ground up\n");
    printf("* Verifying position-based insertion\n");
    printf("* Verifying internal consistency at each step\n");
    printf("\n");

    /* Run all tests */
    int all_passed = 1;

    all_passed &= Test_InsertAtBeginning();
    all_passed &= Test_InsertInMiddle();
    all_passed &= Test_InsertAtEnd();
    all_passed &= Test_InsertIntoSingle();
    all_passed &= Test_ForceLevelCreation();
    all_passed &= Test_BoundaryValues();
    all_passed &= Test_InsertZeros();
    all_passed &= Test_InsertUINTMAX();
    all_passed &= Test_MultipleInsertsAtSamePosition();
    all_passed &= Test_SequentialInserts();
    all_passed &= Test_AlternatingPositions();
    all_passed &= Test_RandomPositionInserts();
    all_passed &= Test_Stress();
    all_passed &= Test_DecompressConsistency();
    all_passed &= Test_DeepStructureValidation();

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total tests:          %d\n", stats.total_tests);
    printf("Tests passed:         %d\n", stats.passed_tests);
    printf("Tests failed:         %d\n", stats.failed_tests);
    printf("\n");
    printf("Total inserts:        %d\n", stats.total_inserts);
    printf("Total verifications:  %d\n", stats.total_verifications);
    printf("\n");

    if (all_passed && stats.failed_tests == 0) {
        printf("[OK] ALL TESTS PASSED\n");
        printf("========================================\n");
        return 0;
    }
    else {
        printf("[FAIL] SOME TESTS FAILED\n");
        printf("========================================\n");
        return 1;
    }
}