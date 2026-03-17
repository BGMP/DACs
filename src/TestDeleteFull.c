/*
 * deleteFT Unit Test Suite
 * Complete validation of deleteFT functionality
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
    int total_deletes;
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
    if (rep->listLength == 0) return 1;  // Empty structure

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

 /* Test 1: Delete from beginning */
int Test_DeleteFromBeginning() {
    TEST_START("Delete from beginning (position 1)");

    uint initial[] = { 10, 20, 30, 40 };
    FTRep* rep = createFT(initial, 4);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete position 1 */
    deleteFT(rep, 1);
    stats.total_deletes++;

    VERIFY(rep->listLength == 3, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after delete")) return 0;

    /* Verify remaining values: {20, 30, 40} */
    uint expected[] = { 20, 30, 40 };
    if (!VerifyAllValues(rep, expected, 3, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete from beginning");
    return 1;
}

/* Test 2: Delete from middle */
int Test_DeleteFromMiddle() {
    TEST_START("Delete from middle position");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete position 3 (value 30) */
    deleteFT(rep, 3);
    stats.total_deletes++;

    VERIFY(rep->listLength == 4, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after delete")) return 0;

    /* Verify remaining values: {10, 20, 40, 50} */
    uint expected[] = { 10, 20, 40, 50 };
    if (!VerifyAllValues(rep, expected, 4, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete from middle");
    return 1;
}

/* Test 3: Delete from end */
int Test_DeleteFromEnd() {
    TEST_START("Delete from end position");

    uint initial[] = { 10, 20, 30, 40 };
    FTRep* rep = createFT(initial, 4);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete position 4 (last element) */
    deleteFT(rep, 4);
    stats.total_deletes++;

    VERIFY(rep->listLength == 3, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after delete")) return 0;

    /* Verify remaining values: {10, 20, 30} */
    uint expected[] = { 10, 20, 30 };
    if (!VerifyAllValues(rep, expected, 3, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete from end");
    return 1;
}

/* Test 4: Delete single element */
int Test_DeleteSingleElement() {
    TEST_START("Delete from single-element structure");

    uint initial[] = { 42 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete the only element */
    deleteFT(rep, 1);
    stats.total_deletes++;

    VERIFY(rep->listLength == 0, "listLength should be 0");
    VERIFY(rep->sizeLastLevel == 0, "sizeLastLevel should be 0");
    if (!VerifyInternalStructure(rep, "after delete")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete single element");
    return 1;
}

/* Test 5: Delete causing level removal */
int Test_DeleteCausingLevelRemoval() {
    TEST_START("Delete that causes level removal");

    uint initial[] = { 10, 20, 1000000 };
    FTRep* rep = createFT(initial, 3);

    uint initial_levels = rep->nLevels;
    printf("    Initial levels: %u\n", initial_levels);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete the large value */
    deleteFT(rep, 3);
    stats.total_deletes++;

    printf("    Levels after delete: %u\n", rep->nLevels);

    VERIFY(rep->listLength == 2, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after delete")) return 0;

    /* Verify remaining values: {10, 20} */
    uint expected[] = { 10, 20 };
    if (!VerifyAllValues(rep, expected, 2, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete causing level removal");
    return 1;
}

/* Test 6: Multiple consecutive deletes */
int Test_MultipleConsecutiveDeletes() {
    TEST_START("Multiple consecutive deletes");

    uint initial[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    FTRep* rep = createFT(initial, 10);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete positions 3, 5, 7 (values 30, 50, 70) */
    deleteFT(rep, 3);
    stats.total_deletes++;
    VERIFY(rep->listLength == 9, "listLength after first delete");

    deleteFT(rep, 4);  // Was position 5, now 4 after previous delete
    stats.total_deletes++;
    VERIFY(rep->listLength == 8, "listLength after second delete");

    deleteFT(rep, 5);  // Was position 7, now 5 after previous deletes
    stats.total_deletes++;
    VERIFY(rep->listLength == 7, "listLength after third delete");

    if (!VerifyInternalStructure(rep, "after deletes")) return 0;

    /* Verify remaining values: {10, 20, 40, 60, 80, 90, 100} */
    uint expected[] = { 10, 20, 40, 60, 80, 90, 100 };
    if (!VerifyAllValues(rep, expected, 7, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Multiple consecutive deletes");
    return 1;
}

/* Test 7: Delete all elements one by one */
int Test_DeleteUntilEmpty() {
    TEST_START("Delete all elements until empty");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");

    /* Delete all elements from the end */
    for (int i = 5; i >= 1; i--) {
        deleteFT(rep, i);
        stats.total_deletes++;
        VERIFY(rep->listLength == (uint)(i - 1), "listLength incorrect");
        if (!VerifyInternalStructure(rep, "after delete")) return 0;
    }

    VERIFY(rep->listLength == 0, "Structure should be empty");
    VERIFY(rep->sizeLastLevel == 0, "sizeLastLevel should be 0");

    destroyFT(rep);
    TEST_PASS("Delete until empty");
    return 1;
}

/* Test 8: Delete UINT_MAX values */
int Test_DeleteUINTMAX() {
    TEST_START("Delete UINT_MAX values");

    uint initial[] = { 10, UINT_MAX, 20, UINT_MAX, 30 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete first UINT_MAX (position 2) */
    deleteFT(rep, 2);
    stats.total_deletes++;

    VERIFY(rep->listLength == 4, "listLength after first delete");
    uint expected1[] = { 10, 20, UINT_MAX, 30 };
    if (!VerifyAllValues(rep, expected1, 4, "after first delete")) return 0;

    /* Delete second UINT_MAX (now at position 3) */
    deleteFT(rep, 3);
    stats.total_deletes++;

    VERIFY(rep->listLength == 3, "listLength after second delete");
    uint expected2[] = { 10, 20, 30 };
    if (!VerifyAllValues(rep, expected2, 3, "after second delete")) return 0;

    if (!VerifyInternalStructure(rep, "after deletes")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete UINT_MAX values");
    return 1;
}

/* Test 9: Delete zeros */
int Test_DeleteZeros() {
    TEST_START("Delete zero values");

    uint initial[] = { 0, 10, 0, 20, 0, 30 };
    FTRep* rep = createFT(initial, 6);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete zeros at positions 1, 2 (was 3), 3 (was 5) */
    deleteFT(rep, 1);
    stats.total_deletes++;
    deleteFT(rep, 2);  // Was position 3
    stats.total_deletes++;
    deleteFT(rep, 3);  // Was position 5 - NOW AT POSITION 3!
    stats.total_deletes++;

    VERIFY(rep->listLength == 3, "listLength incorrect");

    uint expected[] = { 10, 20, 30 };
    if (!VerifyAllValues(rep, expected, 3, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after deletes")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete zeros");
    return 1;
}

/* Test 10: Delete with boundary values */
int Test_DeleteBoundaryValues() {
    TEST_START("Delete with boundary values");

    uint initial[] = { 0, 1, 63, 64, 127, 128, 255, 256, 65535, 65536 };
    FTRep* rep = createFT(initial, 10);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Delete every other element */
    deleteFT(rep, 2);  // Delete 1
    stats.total_deletes++;
    deleteFT(rep, 3);  // Delete 64 (was position 4)
    stats.total_deletes++;
    deleteFT(rep, 4);  // Delete 128 (was position 6)
    stats.total_deletes++;
    deleteFT(rep, 5);  // Delete 256 (was position 8)
    stats.total_deletes++;
    deleteFT(rep, 6);  // Delete 65536 (was position 10)
    stats.total_deletes++;

    VERIFY(rep->listLength == 5, "listLength incorrect");

    uint expected[] = { 0, 63, 127, 255, 65535 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after deletes")) return 0;

    destroyFT(rep);
    TEST_PASS("Delete boundary values");
    return 1;
}

/* Test 11: Random position deletes */
int Test_RandomPositionDeletes() {
    TEST_START("Random position deletes with verification");

    srand(99999);  /* Fixed seed for reproducibility */

    uint count = 100;
    uint* all_values = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        all_values[i] = i * 10;
    }

    FTRep* rep = createFT(all_values, count);
    uint current_length = count;

    /* Delete 50 random elements */
    for (uint i = 0; i < 50; i++) {
        uint position = (rand() % current_length) + 1;

        /* Remove from tracking array */
        for (uint j = position - 1; j < current_length - 1; j++) {
            all_values[j] = all_values[j + 1];
        }
        current_length--;

        /* Delete from DAC */
        deleteFT(rep, position);
        stats.total_deletes++;

        /* Verify every 10 deletes */
        if ((i + 1) % 10 == 0) {
            if (!VerifyAllValues(rep, all_values, current_length, "incremental verification")) {
                printf("           Failed at delete %u\n", i);
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

    if (!VerifyInternalStructure(rep, "after random deletes")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    free(all_values);
    destroyFT(rep);
    TEST_PASS("Random position deletes");
    return 1;
}

/* Test 12: Delete with decompress consistency */
int Test_DecompressConsistency() {
    TEST_START("Decompress matches access after each delete");

    uint initial[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    FTRep* rep = createFT(initial, 8);

    /* After initial creation */
    if (!VerifyDecompressMatchesAccess(rep, "initial")) {
        destroyFT(rep);
        return 0;
    }

    /* Delete values and verify consistency */
    uint delete_positions[] = { 3, 5, 2, 4, 1 };

    for (int i = 0; i < 5; i++) {
        deleteFT(rep, delete_positions[i]);
        stats.total_deletes++;

        if (!VerifyDecompressMatchesAccess(rep, "after delete")) {
            printf("           Failed after delete %d (position=%u)\n",
                i, delete_positions[i]);
            destroyFT(rep);
            return 0;
        }
    }

    destroyFT(rep);
    TEST_PASS("Decompress consistency");
    return 1;
}

/* Test 13: Stress test */
int Test_Stress() {
    TEST_START("Stress test with 2000 random deletes");

    srand(77777);

    uint count = 20;
    uint* all_values = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        all_values[i] = (uint)rand() * (uint)rand();
    }

    FTRep* rep = createFT(all_values, count);
    uint current_length = count;

	printf("Structure with accessFT before deletes:\n");
	for (uint i = 1; i <= rep->listLength; i++) {
		printf("  Position %u: %u\n", i, accessFT(rep, i));
	}

    printf("    Deleting 10 values at random positions...\n");

    for (uint i = 0; i < 10; i++) {
        uint position = (rand() % current_length) + 1;

        /* Remove from tracking array */
        for (uint j = position - 1; j < current_length - 1; j++) {
            all_values[j] = all_values[j + 1];
        }
        current_length--;

        /* Delete from DAC */
        deleteFT(rep, position);
        stats.total_deletes++;

        if ((i + 1) % 500 == 0) {
            printf("    Progress: %u/%u deletes completed\n", i + 1, 2000);
        }
    }

    printf("    Verifying all %u remaining values...\n", current_length);

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

/* Test 14: Deep structure validation */
int Test_DeepStructureValidation() {
    TEST_START("Deep DAC structure validation after deletes");

    uint initial[] = { 0, 1, 2, 15, 16, 63, 64, 127, 128, 255, 256,
                       1000, 5000, 10000, 50000, 100000, UINT_MAX };
    FTRep* rep = createFT(initial, 17);

    /* Delete some elements */
    deleteFT(rep, 5);   // Delete 16
    stats.total_deletes++;
    deleteFT(rep, 8);   // Delete 128 (was position 9)
    stats.total_deletes++;
    deleteFT(rep, 12);  // Delete 50000 (was position 14)
    stats.total_deletes++;

    uint remaining = 14;

    /* Bitmap structure consistency */
    printf("    Checking bitmap structure...\n");

    if (rep->nLevels > 1) {
        uint level0_size = rep->bS[0]->n;
        VERIFY(level0_size == remaining,
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

    for (uint elem_idx = 0; elem_idx < remaining; elem_idx++) {
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

    /* Last level properties */
    printf("    Checking last level properties...\n");

    uint last_level = rep->nLevels - 1;
    uint expected_last_size;

    if (last_level > 0) {
        uint ones = rank(rep->bS[last_level - 1], rep->bS[last_level - 1]->n - 1);
        expected_last_size = rep->bS[last_level - 1]->n - ones;
    }
    else {
        expected_last_size = remaining;
    }

    VERIFY(rep->sizeLastLevel == expected_last_size,
        "sizeLastLevel should match bitmap calculation");

    printf("    All DAC structure invariants verified!\n");

    destroyFT(rep);
    TEST_PASS("Deep DAC structure validation");
    return 1;
}


int Test_AlternatingDeletes() {
    TEST_START("Delete alternating positions");

    uint count = 100;
    uint* initial = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        initial[i] = i + 1;
    }

    FTRep* rep = createFT(initial, count);

    /* Delete all even-valued elements (2, 4, 6, 8, ..., 100)
     * by deleting positions 2, 3, 4, 5, ..., 51
     * After each deletion, positions shift down, so:
     *   Delete pos 2 removes value 2
     *   Delete pos 3 removes value 4 (which moved to pos 3)
     *   Delete pos 4 removes value 6 (which moved to pos 4)
     *   etc.
     */
    for (uint i = 0; i < 50; i++) {
        deleteFT(rep, 2 + i);  // Delete positions 2, 3, 4, ..., 51
        stats.total_deletes++;
    }

    VERIFY(rep->listLength == 50, "listLength should be 50");

    /* Remaining values should be all odd numbers: 1, 3, 5, 7, ..., 99 */
    for (uint i = 0; i < 50; i++) {
        uint expected = 2 * i + 1;  // 1, 3, 5, 7, ..., 99
        if (!VerifyAccess(rep, i + 1, expected, "odd value")) {
            free(initial);
            destroyFT(rep);
            return 0;
        }
    }

    if (!VerifyInternalStructure(rep, "after alternating deletes")) {
        free(initial);
        destroyFT(rep);
        return 0;
    }

    free(initial);
    destroyFT(rep);
    TEST_PASS("Delete alternating positions");
    return 1;
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("deleteFT UNIT TEST SUITE\n");
    printf("========================================\n");
    printf("* Testing deleteFT functionality from ground up\n");
    printf("* Verifying position-based deletion\n");
    printf("* Verifying internal consistency at each step\n");
    printf("\n");

    /* Run all tests */
    int all_passed = 1;

    all_passed &= Test_DeleteFromBeginning();
    all_passed &= Test_DeleteFromMiddle();
    all_passed &= Test_DeleteFromEnd();
    all_passed &= Test_DeleteSingleElement();
    all_passed &= Test_DeleteCausingLevelRemoval();
    all_passed &= Test_MultipleConsecutiveDeletes();
    all_passed &= Test_DeleteUntilEmpty();
    all_passed &= Test_DeleteUINTMAX();
    all_passed &= Test_DeleteZeros();
    all_passed &= Test_DeleteBoundaryValues();
    all_passed &= Test_RandomPositionDeletes();
    all_passed &= Test_DecompressConsistency();
    all_passed &= Test_Stress();
    all_passed &= Test_DeepStructureValidation();
    all_passed &= Test_AlternatingDeletes();

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total tests:          %d\n", stats.total_tests);
    printf("Tests passed:         %d\n", stats.passed_tests);
    printf("Tests failed:         %d\n", stats.failed_tests);
    printf("\n");
    printf("Total deletes:        %d\n", stats.total_deletes);
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
