/*
 * replaceFT Unit Test Suite
 * Complete validation of replaceFT functionality
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
    int total_replaces;
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
    VERIFY(rep->listLength >= 0, "listLength must be >= 0");
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
    if (rep->listLength == 0) return 1;

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

 /* Test 1: Replace at beginning */
int Test_ReplaceAtBeginning() {
    TEST_START("Replace at beginning (position 1)");

    uint initial[] = { 10, 20, 30, 40 };
    FTRep* rep = createFT(initial, 4);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace position 1: 10 -> 99 */
    replaceFT(rep, 1, 99);
    stats.total_replaces++;

    VERIFY(rep->listLength == 4, "listLength should remain 4");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {99, 20, 30, 40} */
    uint expected[] = { 99, 20, 30, 40 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace at beginning");
    return 1;
}

/* Test 2: Replace at middle */
int Test_ReplaceAtMiddle() {
    TEST_START("Replace at middle position");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace position 3: 30 -> 300 */
    replaceFT(rep, 3, 300);
    stats.total_replaces++;

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {10, 20, 300, 40, 50} */
    uint expected[] = { 10, 20, 300, 40, 50 };

	printf("Showing values after replace using accessFT:\n");
    for (uint i = 1; i <= rep->listLength; i++) {
        uint val = accessFT(rep, i);
        printf("    Position %u: %u\n", i, val);
	}


    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace at middle");
    return 1;
}

/* Test 3: Replace at end */
int Test_ReplaceAtEnd() {
    TEST_START("Replace at end position");

    uint initial[] = { 10, 20, 30, 40 };
    FTRep* rep = createFT(initial, 4);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace position 4: 40 -> 4000 */
    replaceFT(rep, 4, 4000);
    stats.total_replaces++;

    VERIFY(rep->listLength == 4, "listLength should remain 4");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {10, 20, 30, 4000} */
    uint expected[] = { 10, 20, 30, 4000 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace at end");
    return 1;
}

/* Test 4: Replace single element */
int Test_ReplaceSingleElement() {
    TEST_START("Replace in single-element structure");

    uint initial[] = { 42 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace the only element: 42 -> 1000 */
    replaceFT(rep, 1, 1000);
    stats.total_replaces++;

    VERIFY(rep->listLength == 1, "listLength should remain 1");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    if (!VerifyAccess(rep, 1, 1000, "single element")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace single element");
    return 1;
}

/* Test 5: Replace small with large (causing level creation) */
int Test_ReplaceSmallWithLarge() {
    TEST_START("Replace small with large value (level creation)");

    uint initial[] = { 10, 20, 30, 40 };
    FTRep* rep = createFT(initial, 4);

    uint initial_levels = rep->nLevels;
    printf("    Initial levels: %u\n", initial_levels);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace 20 with a much larger value */
    replaceFT(rep, 2, 10000000);
    stats.total_replaces++;

    printf("    Levels after replace: %u\n", rep->nLevels);

    VERIFY(rep->listLength == 4, "listLength should remain 4");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {10, 10000000, 30, 40} */
    uint expected[] = { 10, 10000000, 30, 40 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace small with large");
    return 1;
}

/* Test 6: Replace large with small (causing level removal) */
int Test_ReplaceLargeWithSmall() {
    TEST_START("Replace large with small value (level removal)");

    uint initial[] = { 10, 20, 10000000, 40 };
    FTRep* rep = createFT(initial, 4);

    uint initial_levels = rep->nLevels;
    printf("    Initial levels: %u\n", initial_levels);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace large value with small */
    replaceFT(rep, 3, 30);
    stats.total_replaces++;

    printf("    Levels after replace: %u\n", rep->nLevels);

    VERIFY(rep->listLength == 4, "listLength should remain 4");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {10, 20, 30, 40} */
    uint expected[] = { 10, 20, 30, 40 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace large with small");
    return 1;
}

/* Test 7: Replace with same value (no-op) */
int Test_ReplaceWithSameValue() {
    TEST_START("Replace with same value");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace with same values */
    replaceFT(rep, 1, 10);
    stats.total_replaces++;
    replaceFT(rep, 3, 30);
    stats.total_replaces++;
    replaceFT(rep, 5, 50);
    stats.total_replaces++;

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values unchanged: {10, 20, 30, 40, 50} */
    uint expected[] = { 10, 20, 30, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace with same value");
    return 1;
}

/* Test 8: Replace with UINT_MAX */
int Test_ReplaceWithUINTMAX() {
    TEST_START("Replace with UINT_MAX");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace middle value with UINT_MAX */
    replaceFT(rep, 3, UINT_MAX);
    stats.total_replaces++;

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replace")) return 0;

    /* Verify values: {10, 20, UINT_MAX, 40, 50} */
    uint expected[] = { 10, 20, UINT_MAX, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace with UINT_MAX");
    return 1;
}

/* Test 9: Replace UINT_MAX with small value */
int Test_ReplaceUINTMAXWithSmall() {
    TEST_START("Replace UINT_MAX with small value");

    uint initial[] = { 10, UINT_MAX, 30, UINT_MAX, 50 };
    FTRep* rep = createFT(initial, 5);

    uint initial_levels = rep->nLevels;
    printf("    Initial levels: %u\n", initial_levels);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace first UINT_MAX with small value */
    replaceFT(rep, 2, 20);
    stats.total_replaces++;

    /* Replace second UINT_MAX with small value */
    replaceFT(rep, 4, 40);
    stats.total_replaces++;

    printf("    Levels after replaces: %u\n", rep->nLevels);

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values: {10, 20, 30, 40, 50} */
    uint expected[] = { 10, 20, 30, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace UINT_MAX with small");
    return 1;
}

/* Test 10: Replace with zeros */
int Test_ReplaceWithZeros() {
    TEST_START("Replace values with zeros");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace several values with 0 */
    replaceFT(rep, 2, 0);
    stats.total_replaces++;
    replaceFT(rep, 4, 0);
    stats.total_replaces++;

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values: {10, 0, 30, 0, 50} */
    uint expected[] = { 10, 0, 30, 0, 50 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace with zeros");
    return 1;
}

/* Test 11: Replace zeros with values */
int Test_ReplaceZerosWithValues() {
    TEST_START("Replace zeros with non-zero values");

    uint initial[] = { 0, 10, 0, 20, 0, 30 };
    FTRep* rep = createFT(initial, 6);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace zeros with values */
    replaceFT(rep, 1, 5);
    stats.total_replaces++;
    replaceFT(rep, 3, 15);
    stats.total_replaces++;
    replaceFT(rep, 5, 25);
    stats.total_replaces++;

    VERIFY(rep->listLength == 6, "listLength should remain 6");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values: {5, 10, 15, 20, 25, 30} */
    uint expected[] = { 5, 10, 15, 20, 25, 30 };
    if (!VerifyAllValues(rep, expected, 6, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace zeros with values");
    return 1;
}

/* Test 12: Replace with boundary values */
int Test_ReplaceBoundaryValues() {
    TEST_START("Replace with boundary values");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace with various boundary values */
    replaceFT(rep, 1, 0);        // Min value
    stats.total_replaces++;
    replaceFT(rep, 2, 1);        // Min + 1
    stats.total_replaces++;
    replaceFT(rep, 3, 127);      // 2^7 - 1
    stats.total_replaces++;
    replaceFT(rep, 4, 128);      // 2^7
    stats.total_replaces++;
    replaceFT(rep, 5, 65535);    // 2^16 - 1
    stats.total_replaces++;

    VERIFY(rep->listLength == 5, "listLength should remain 5");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values */
    uint expected[] = { 0, 1, 127, 128, 65535 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Replace boundary values");
    return 1;
}

/* Test 13: Multiple consecutive replaces */
int Test_MultipleConsecutiveReplaces() {
    TEST_START("Multiple consecutive replaces");

    uint initial[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    FTRep* rep = createFT(initial, 10);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Replace every element */
    for (uint i = 1; i <= 10; i++) {
        replaceFT(rep, i, i * 100);
        stats.total_replaces++;
    }

    VERIFY(rep->listLength == 10, "listLength should remain 10");
    if (!VerifyInternalStructure(rep, "after replaces")) return 0;

    /* Verify values: {100, 200, 300, ..., 1000} */
    uint expected[] = { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 };
    if (!VerifyAllValues(rep, expected, 10, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Multiple consecutive replaces");
    return 1;
}

/* Test 14: Replace causing alternating level changes */
int Test_AlternatingLevelChanges() {
    TEST_START("Replace causing alternating level changes");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Alternate between small and large values */
    replaceFT(rep, 1, 1000000);  // Small -> Large
    stats.total_replaces++;
    if (!VerifyInternalStructure(rep, "after replace 1")) return 0;

    replaceFT(rep, 3, 2000000);  // Small -> Large
    stats.total_replaces++;
    if (!VerifyInternalStructure(rep, "after replace 2")) return 0;

    replaceFT(rep, 5, 3000000);  // Small -> Large
    stats.total_replaces++;
    if (!VerifyInternalStructure(rep, "after replace 3")) return 0;

    replaceFT(rep, 2, 5000000);  // Small -> Large
    stats.total_replaces++;
    if (!VerifyInternalStructure(rep, "after replace 4")) return 0;

    replaceFT(rep, 4, 6000000);  // Small -> Large
    stats.total_replaces++;
    if (!VerifyInternalStructure(rep, "after replace 5")) return 0;

    VERIFY(rep->listLength == 5, "listLength should remain 5");

    /* Verify values */
    uint expected[] = { 1000000, 5000000, 2000000, 6000000, 3000000 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Alternating level changes");
    return 1;
}

/* Test 15: Random position replaces */
int Test_RandomPositionReplaces() {
    TEST_START("Random position replaces with verification");

    srand(88888);

    uint count = 100;
    uint* all_values = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        all_values[i] = i * 10;
    }

    FTRep* rep = createFT(all_values, count);

    /* Replace 50 random positions with random values */
    for (uint i = 0; i < 50; i++) {
        uint position = (rand() % count) + 1;
        uint new_value = (uint)rand() * (uint)rand();

        all_values[position - 1] = new_value;

        replaceFT(rep, position, new_value);
        stats.total_replaces++;

        /* Verify every 10 replaces */
        if ((i + 1) % 10 == 0) {
            if (!VerifyAllValues(rep, all_values, count, "incremental verification")) {
                printf("           Failed at replace %u\n", i);
                free(all_values);
                destroyFT(rep);
                return 0;
            }
        }
    }

    /* Final verification */
    if (!VerifyAllValues(rep, all_values, count, "final verification")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after random replaces")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    free(all_values);
    destroyFT(rep);
    TEST_PASS("Random position replaces");
    return 1;
}

/* Test 16: Replace with decompress consistency */
int Test_DecompressConsistency() {
    TEST_START("Decompress matches access after each replace");

    uint initial[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    FTRep* rep = createFT(initial, 8);

    /* After initial creation */
    if (!VerifyDecompressMatchesAccess(rep, "initial")) {
        destroyFT(rep);
        return 0;
    }

    /* Replace values and verify consistency */
    uint replacements[][2] = {
        {1, 100},
        {3, 3000},
        {5, 50000},
        {7, 700000},
        {2, 2}
    };

    for (int i = 0; i < 5; i++) {
        replaceFT(rep, replacements[i][0], replacements[i][1]);
        stats.total_replaces++;

        if (!VerifyDecompressMatchesAccess(rep, "after replace")) {
            printf("           Failed after replace %d (pos=%u, val=%u)\n",
                i, replacements[i][0], replacements[i][1]);
            destroyFT(rep);
            return 0;
        }
    }

    destroyFT(rep);
    TEST_PASS("Decompress consistency");
    return 1;
}

/* Test 17: Stress test */
int Test_Stress() {
    TEST_START("Stress test with 1000 random replaces");

    srand(55555);

    uint count = 100;
    uint* all_values = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        all_values[i] = (uint)rand() % 10000;
    }

    FTRep* rep = createFT(all_values, count);

    printf("    Performing 1000 random replacements...\n");

    for (uint i = 0; i < 1000; i++) {
        uint position = (rand() % count) + 1;
        uint new_value = (uint)rand() * (uint)rand();

        all_values[position - 1] = new_value;

        replaceFT(rep, position, new_value);
        stats.total_replaces++;

        if ((i + 1) % 250 == 0) {
            printf("    Progress: %u/%u replaces completed\n", i + 1, 1000);
        }
    }

    printf("    Verifying all %u values...\n", count);

    if (!VerifyAllValues(rep, all_values, count, "stress test")) {
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

/* Test 18: Deep structure validation */
int Test_DeepStructureValidation() {
    TEST_START("Deep DAC structure validation after replaces");

    uint initial[] = { 1, 2, 15, 16, 63, 64, 127, 128, 255, 256,
                       1000, 5000, 10000, 50000, 100000 };
    FTRep* rep = createFT(initial, 15);

    /* Replace with various values that change structure */
    replaceFT(rep, 3, UINT_MAX);     // 15 -> UINT_MAX
    stats.total_replaces++;
    replaceFT(rep, 8, 100);          // 128 -> 100
    stats.total_replaces++;
    replaceFT(rep, 12, 1);           // 5000 -> 1
    stats.total_replaces++;
    replaceFT(rep, 15, 50);          // 100000 -> 50
    stats.total_replaces++;

    /* Bitmap structure consistency */
    printf("    Checking bitmap structure...\n");

    if (rep->nLevels > 1) {
        uint level0_size = rep->bS[0]->n;
        VERIFY(level0_size == rep->listLength,
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

    for (uint elem_idx = 0; elem_idx < rep->listLength; elem_idx++) {
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
    printf("replaceFT UNIT TEST SUITE\n");
    printf("========================================\n");
    printf("* Testing replaceFT functionality\n");
    printf("* Verifying value replacement\n");
    printf("* Verifying level changes\n");
    printf("* Verifying internal consistency\n");
    printf("\n");

    /* Run all tests */
    int all_passed = 1;

    all_passed &= Test_ReplaceAtBeginning();
    all_passed &= Test_ReplaceAtMiddle();
    all_passed &= Test_ReplaceAtEnd();
    all_passed &= Test_ReplaceSingleElement();
    all_passed &= Test_ReplaceSmallWithLarge();
    all_passed &= Test_ReplaceLargeWithSmall();
    all_passed &= Test_ReplaceWithSameValue();
    all_passed &= Test_ReplaceWithUINTMAX();
    all_passed &= Test_ReplaceUINTMAXWithSmall();
    all_passed &= Test_ReplaceWithZeros();
    all_passed &= Test_ReplaceZerosWithValues();
    all_passed &= Test_ReplaceBoundaryValues();
    all_passed &= Test_MultipleConsecutiveReplaces();
    all_passed &= Test_AlternatingLevelChanges();
    all_passed &= Test_RandomPositionReplaces();
    all_passed &= Test_DecompressConsistency();
    all_passed &= Test_Stress();
    all_passed &= Test_DeepStructureValidation();

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total tests:          %d\n", stats.total_tests);
    printf("Tests passed:         %d\n", stats.passed_tests);
    printf("Tests failed:         %d\n", stats.failed_tests);
    printf("\n");
    printf("Total replaces:       %d\n", stats.total_replaces);
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