/*
 * appendFT Unit Test Suite
 * Complete validation of appendFT functionality
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
    int total_appends;
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

    /* Verify basic structure */
    VERIFY(rep->nLevels > 0, "nLevels must be > 0");
    VERIFY(rep->listLength > 0, "listLength must be > 0");
    VERIFY(rep->levels != NULL, "levels array is NULL");
    VERIFY(rep->base != NULL, "base array is NULL");
    VERIFY(rep->base_bits != NULL, "base_bits array is NULL");

    /* Verify level pointers consistency */
    for (uint i = 0; i < rep->nLevels; i++) {
        VERIFY(rep->base[i] > 0, "base[i] must be > 0");
        VERIFY(rep->base_bits[i] > 0, "base_bits[i] must be > 0");
        VERIFY(rep->base[i] == (1u << rep->base_bits[i]), "base != 2^base_bits");
    }

    /* Verify bitmap structures exist for all levels except last */
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

 /* Test 1: Append to minimal structure (2 elements) */
int Test_AppendToMinimal() {
    TEST_START("Append to minimal 2-element structure");

    uint initial[] = { 5, 10 };
    FTRep* rep = createFT(initial, 2);

    VERIFY(rep != NULL, "createFT failed");
    VERIFY(rep->listLength == 2, "initial listLength incorrect");

    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Append a third element */
    appendFT(rep, 20);
    stats.total_appends++;

    VERIFY(rep->listLength == 3, "listLength not updated");
    if (!VerifyInternalStructure(rep, "after first append")) return 0;
    if (!VerifyAccess(rep, 3, 20, "first append")) return 0;

    /* Verify all values */
    uint expected[] = { 5, 10, 20 };
    if (!VerifyAllValues(rep, expected, 3, "all values")) return 0;

    destroyFT(rep);
    TEST_PASS("Append to minimal structure");
    return 1;
}

/* Test 2: Append to single element (edge case) */
int Test_AppendToSingle() {
    TEST_START("Append to single-element structure");

    uint initial[] = { 42 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    /* Append multiple values */
    uint values[] = { 1, 2, 100, 255 };
    for (int i = 0; i < 4; i++) {
        appendFT(rep, values[i]);
        stats.total_appends++;
        VERIFY(rep->listLength == (uint)(2 + i), "listLength incorrect");
    }

    /* Verify all values */
    uint expected[] = { 42, 1, 2, 100, 255 };
    if (!VerifyAllValues(rep, expected, 5, "all values")) return 0;
    if (!VerifyInternalStructure(rep, "after appends")) return 0;

    destroyFT(rep);
    TEST_PASS("Append to single element");
    return 1;
}

/* Test 3: Force level creation - small to large values */
int Test_ForceLevelCreation() {
    TEST_START("Force level creation with progressively larger values");

    /* Start with very small values (likely 1 level) */
    uint initial[] = { 0, 1, 2 };
    FTRep* rep = createFT(initial, 3);

    uint initial_levels = rep->nLevels;

    /* Append value that should force more levels */
    uint large_value = 100000;
    appendFT(rep, large_value);
    stats.total_appends++;

    VERIFY(rep->listLength == 4, "listLength incorrect");

    if (!VerifyInternalStructure(rep, "after large value")) return 0;
    if (!VerifyAccess(rep, 4, large_value, "large value")) return 0;

    /* Verify all original values still correct */
    for (uint i = 0; i < 3; i++) {
        if (!VerifyAccess(rep, i + 1, initial[i], "original values")) return 0;
    }

    destroyFT(rep);
    TEST_PASS("Force level creation");
    return 1;
}

/* Test 4: Boundary value testing */
int Test_BoundaryValues() {
    TEST_START("Boundary values for typical base sizes");

    uint initial[] = { 1 };
    FTRep* rep = createFT(initial, 1);

    uint boundaries[] = {
        0, 1, 2, 3, 63, 64, 65, 127, 128, 129,
        255, 256, 257, 1023, 1024, 1025,
        65535, 65536, UINT_MAX
    };

    int n_boundaries = sizeof(boundaries) / sizeof(boundaries[0]);

    for (int i = 0; i < n_boundaries; i++) {
        uint old_nLevels = rep->nLevels;

        appendFT(rep, boundaries[i]);
        stats.total_appends++;

        if (!VerifyAccess(rep, i + 2, boundaries[i], "boundary value")) {
            printf("FAILED at boundary value %u (i=%d)\n", boundaries[i], i);
            destroyFT(rep);
            return 0;
        }
    }

    destroyFT(rep);

    TEST_PASS("Boundary values");
    return 1;
}
/* Test 5: Zeros and UINT_MAX extremes */
int Test_Extremes() {
    TEST_START("Extreme values: zeros and UINT_MAX");

    uint initial[] = { 100 };
    FTRep* rep = createFT(initial, 1);

    /* Append zeros */
    for (int i = 0; i < 10; i++) {
        appendFT(rep, 0);
        stats.total_appends++;
    }

    /* Append UINT_MAX values */
    for (int i = 0; i < 10; i++) {
        appendFT(rep, UINT_MAX);
        stats.total_appends++;
    }

    /* Verify zeros */
    for (int i = 0; i < 10; i++) {
        if (!VerifyAccess(rep, i + 2, 0, "zero values")) {
            destroyFT(rep);
            return 0;
        }
    }

    /* Verify UINT_MAX */
    for (int i = 0; i < 10; i++) {
        if (!VerifyAccess(rep, i + 12, UINT_MAX, "UINT_MAX values")) {
            destroyFT(rep);
            return 0;
        }
    }

    if (!VerifyInternalStructure(rep, "after extremes")) return 0;

    destroyFT(rep);
    TEST_PASS("Extreme values");
    return 1;
}

/* Test 6: Alternating small and large values */
int Test_AlternatingValues() {
    TEST_START("Alternating small and large values");

    uint initial[] = { 5 };
    FTRep* rep = createFT(initial, 1);

    uint count = 100;
    for (uint i = 0; i < count; i++) {
        uint value = (i % 2 == 0) ? i : (1000000 + i);
        appendFT(rep, value);
        stats.total_appends++;

        if (!VerifyAccess(rep, i + 2, value, "alternating value")) {
            destroyFT(rep);
            return 0;
        }
    }

    VERIFY(rep->listLength == count + 1, "listLength incorrect");
    if (!VerifyInternalStructure(rep, "after alternating")) return 0;

    destroyFT(rep);
    TEST_PASS("Alternating values");
    return 1;
}

/* Test 7: Sequential values (with debug output) */
int Test_Sequential() {
    TEST_START("Sequential values 1, 2, 3, ..., N");

    uint initial[] = { 1 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    uint count = 500;
    /* Append 2, 3, 4, ..., 501 */
    for (uint i = 2; i <= count + 1; i++) {
        appendFT(rep, i);
        stats.total_appends++;
    }

    /* Verify all values: {1, 2, 3, ..., 501} */
    for (uint i = 1; i <= count + 1; i++) {
        if (!VerifyAccess(rep, i, i, "sequential values")) {
            TEST_FAIL("Sequential values", "Value mismatch detected");
            destroyFT(rep);
            return 0;
        }
    }

    VERIFY(rep->listLength == count + 1, "listLength incorrect");
    if (!VerifyInternalStructure(rep, "after sequential")) {
        TEST_FAIL("Sequential values", "Final structure invalid");
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Sequential values");
    return 1;
}

/* Test 8: Powers of 2 */
int Test_PowersOfTwo() {
    TEST_START("Powers of 2");

    uint initial[] = { 1 };
    FTRep* rep = createFT(initial, 1);

    /* Append all powers of 2 from 2^0 to 2^31 */
    for (int i = 0; i < 32; i++) {
        uint value = 1u << i;
        appendFT(rep, value);
        stats.total_appends++;

        if (!VerifyAccess(rep, i + 2, value, "power of 2")) {
            printf("           Failed at 2^%d = %u\n", i, value);
            destroyFT(rep);
            return 0;
        }
    }

    if (!VerifyInternalStructure(rep, "after powers of 2")) return 0;

    destroyFT(rep);
    TEST_PASS("Powers of 2");
    return 1;
}

/* Test 9: Decompress-Access consistency */
int Test_DecompressConsistency() {
    TEST_START("Decompress matches access at every step");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    /* After initial creation */
    if (!VerifyDecompressMatchesAccess(rep, "initial")) {
        destroyFT(rep);
        return 0;
    }

    /* Append values and verify consistency after each */
    uint values[] = { 0, 1, 100, 1000, 10000, 100000, UINT_MAX };
    for (int i = 0; i < 7; i++) {
        appendFT(rep, values[i]);
        stats.total_appends++;

        if (!VerifyDecompressMatchesAccess(rep, "after append")) {
            printf("           Failed after appending %u\n", values[i]);
            destroyFT(rep);
            return 0;
        }
    }

    destroyFT(rep);
    TEST_PASS("Decompress-access consistency");
    return 1;
}

/* Test 10: Random values with full incremental verification */
int Test_RandomIncremental() {
    TEST_START("Random values with incremental verification");

    srand(12345);  /* Fixed seed for reproducibility */

    uint initial[] = { 42 };
    FTRep* rep = createFT(initial, 1);

    uint count = 1000;
    uint* all_values = malloc(sizeof(uint) * (count + 1));
    all_values[0] = 42;

    for (uint i = 0; i < count; i++) {
        uint value = (uint)rand() * (uint)rand();  /* Full range */
        all_values[i + 1] = value;

        appendFT(rep, value);
        stats.total_appends++;

        /* Verify THIS append immediately */
        if (!VerifyAccess(rep, i + 2, value, "immediate verify")) {
            printf("           Failed immediately after append %u\n", i);
            free(all_values);
            destroyFT(rep);
            return 0;
        }

        /* Every 100 appends, verify ALL values */
        if ((i + 1) % 100 == 0) {
            if (!VerifyAllValues(rep, all_values, i + 2, "full verification")) {
                printf("           Failed full verification at append %u\n", i);
                free(all_values);
                destroyFT(rep);
                return 0;
            }
        }
    }

    /* Final verification of all values */
    if (!VerifyAllValues(rep, all_values, count + 1, "final verification")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after random appends")) {
        free(all_values);
        destroyFT(rep);
        return 0;
    }

    free(all_values);
    destroyFT(rep);
    TEST_PASS("Random incremental");
    return 1;
}

/* Test 11: Stress test - large number of appends */
int Test_Stress() {
    TEST_START("Stress test with 5000 random appends");

    srand(99999);

    uint initial[] = { 1, 2, 3 };
    FTRep* rep = createFT(initial, 3);

    uint count = 5000;
    uint* all_values = malloc(sizeof(uint) * (count + 3));
    all_values[0] = 1;
    all_values[1] = 2;
    all_values[2] = 3;

    printf("    Appending %u values...\n", count);

    for (uint i = 0; i < count; i++) {
        uint value = (uint)rand() * (uint)rand();
        all_values[i + 3] = value;

        appendFT(rep, value);
        stats.total_appends++;

        if ((i + 1) % 1000 == 0) {
            printf("    Progress: %u/%u appends completed\n", i + 1, count);
        }
    }

    printf("    Verifying all %u values...\n", count + 3);

    if (!VerifyAllValues(rep, all_values, count + 3, "stress test")) {
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

/* Test 12: Repeated identical values */
int Test_RepeatedValues() {
    TEST_START("Repeated identical values");

    uint test_values[] = { 0, 1, 63, 64, 255, 256, 1000, 65536, UINT_MAX };
    int n_values = sizeof(test_values) / sizeof(test_values[0]);
    int repeats = 50;

    for (int v = 0; v < n_values; v++) {
        uint value = test_values[v];

        uint initial[] = { 100 };
        FTRep* rep = createFT(initial, 1);

        for (int i = 0; i < repeats; i++) {
            appendFT(rep, value);
            stats.total_appends++;
        }

        /* Verify all instances */
        for (int i = 0; i < repeats; i++) {
            if (!VerifyAccess(rep, i + 2, value, "repeated value")) {
                printf("           Failed at value=%u, instance=%d\n", value, i);
                destroyFT(rep);
                return 0;
            }
        }

        destroyFT(rep);
    }

    TEST_PASS("Repeated values");
    return 1;
}

/* Test 13: Append zeros to non-zero structure */
int Test_AppendZeros() {
    TEST_START("Append zeros to non-zero structure");

    /* Start with non-zero values */
    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    /* Append zeros */
    for (int i = 0; i < 20; i++) {
        appendFT(rep, 0);
        stats.total_appends++;
    }

    /* Verify initial values still correct */
    VERIFY(VerifyAccess(rep, 1, 10, "first value"), "first value check");
    VERIFY(VerifyAccess(rep, 2, 20, "second value"), "second value check");
    VERIFY(VerifyAccess(rep, 3, 30, "third value"), "third value check");

    /* Verify zeros */
    for (int i = 0; i < 20; i++) {
        if (!VerifyAccess(rep, i + 4, 0, "appended zeros")) {
            destroyFT(rep);
            return 0;
        }
    }

    VERIFY(rep->listLength == 23, "listLength incorrect");
    if (!VerifyInternalStructure(rep, "after appending zeros")) return 0;

    destroyFT(rep);
    TEST_PASS("Append zeros");
    return 1;
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("appendFT UNIT TEST SUITE\n");
    printf("========================================\n");
    printf("Testing appendFT functionality from ground up\n");
    printf("Building structures programmatically\n");
    printf("Verifying internal consistency at each step\n");
    printf("\n");

    /* Run all tests */
    int all_passed = 1;

    all_passed &= Test_AppendToMinimal();
    all_passed &= Test_AppendToSingle();
    all_passed &= Test_BoundaryValues();
    all_passed &= Test_ForceLevelCreation();
    all_passed &= Test_Extremes();
    all_passed &= Test_AlternatingValues();
    all_passed &= Test_Sequential();
    all_passed &= Test_PowersOfTwo();
    all_passed &= Test_DecompressConsistency();
    all_passed &= Test_RandomIncremental();
    all_passed &= Test_Stress();
    all_passed &= Test_RepeatedValues();
    all_passed &= Test_AppendZeros();
    

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total tests:          %d\n", stats.total_tests);
    printf("Tests passed:         %d\n", stats.passed_tests);
    printf("Tests failed:         %d\n", stats.failed_tests);
    printf("\n");
    printf("Total appends:        %d\n", stats.total_appends);
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