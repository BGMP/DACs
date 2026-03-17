/*
 * rebuildFT Unit Test Suite
 * Complete validation of rebuildFT functionality
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
    int total_rebuilds;
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

/* Calculate total bits used by structure */
uint CalculateTotalBits(FTRep* rep) {
    uint total_bits = 0;

    // Count bits in all levels
    for (uint i = 0; i < rep->nLevels; i++) {
        if (i < rep->nLevels - 1) {
            total_bits += rep->bS[i]->n * rep->base_bits[i];  // Data bits
            total_bits += rep->bS[i]->n;                       // Bitmap bits
        }
        else {
            total_bits += rep->sizeLastLevel * rep->base_bits[i];  // Last level data
        }
    }

    return total_bits;
}

/* Print structure details */
void PrintStructureInfo(FTRep* rep, const char* label) {
    printf("    %s: nLevels=%u, listLength=%u, chunk_sizes=[",
        label, rep->nLevels, rep->listLength);
    for (uint i = 0; i < rep->nLevels; i++) {
        printf("%u", rep->base_bits[i]);
        if (i < rep->nLevels - 1) printf(",");
    }
    printf("], total_bits=%u\n", CalculateTotalBits(rep));
}

/* ============================================================================
 * UNIT TESTS
 * ============================================================================ */

 /* Test 1: Rebuild already optimal structure */
int Test_RebuildAlreadyOptimal() {
    TEST_START("Rebuild already optimal structure");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    if (!VerifyInternalStructure(rep, "initial structure")) return 0;

    PrintStructureInfo(rep, "Before rebuild");

    uint levels_before = rep->nLevels;
    uint* chunk_sizes_before = malloc(sizeof(uint) * levels_before);
    memcpy(chunk_sizes_before, rep->base_bits, sizeof(uint) * levels_before);

    // Rebuild should detect it's already optimal
    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Structure should be unchanged
    VERIFY(rep->nLevels == levels_before, "nLevels changed");
    for (uint i = 0; i < levels_before; i++) {
        if (rep->base_bits[i] != chunk_sizes_before[i]) {
            printf("    [FAIL] Chunk size changed at level %u\n", i);
            free(chunk_sizes_before);
            destroyFT(rep);
            return 0;
        }
    }

    // Data should still be correct
    if (!VerifyAllValues(rep, initial, 5, "after rebuild")) {
        free(chunk_sizes_before);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        free(chunk_sizes_before);
        destroyFT(rep);
        return 0;
    }

    free(chunk_sizes_before);
    destroyFT(rep);
    TEST_PASS("Rebuild already optimal");
    return 1;
}

/* Test 2: Rebuild after multiple appends */
int Test_RebuildAfterAppends() {
    TEST_START("Rebuild after multiple appends");

    uint initial[] = { 10, 20, 30 };
    FTRep* rep = createFT(initial, 3);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Initial");

    // Append values that might create suboptimal encoding
    appendFT(rep, 40);
    appendFT(rep, 50);
    appendFT(rep, 60);
    appendFT(rep, 1000);
    appendFT(rep, 2000);
    appendFT(rep, 3000);

    PrintStructureInfo(rep, "After appends");

    uint bits_before = CalculateTotalBits(rep);

    // Now rebuild to get optimal encoding
    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    uint bits_after = CalculateTotalBits(rep);

    printf("    Space: before=%u bits, after=%u bits, saved=%d bits\n",
        bits_before, bits_after, (int)bits_before - (int)bits_after);

    // Verify data integrity
    uint expected[] = { 10, 20, 30, 40, 50, 60, 1000, 2000, 3000 };
    if (!VerifyAllValues(rep, expected, 9, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild after appends");
    return 1;
}

/* Test 3: Rebuild after multiple replaces */
/* Test 3: Rebuild after multiple replaces */
int Test_RebuildAfterReplaces() {
    TEST_START("Rebuild after multiple replaces");

    uint initial[] = { 100000, 200000, 300000, 400000, 500000 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Initial (large values)");

    // Replace with much smaller values
    replaceFT(rep, 1, 10);
    replaceFT(rep, 2, 20);
    replaceFT(rep, 3, 30);
    replaceFT(rep, 4, 40);
    replaceFT(rep, 5, 50);

    PrintStructureInfo(rep, "After replaces (small values)");

    uint levels_before = rep->nLevels;
    uint bits_before = CalculateTotalBits(rep);
    uint chunk_size_before = rep->base_bits[0];  // First chunk size

    // Rebuild should create much more compact structure
    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    uint levels_after = rep->nLevels;
    uint bits_after = CalculateTotalBits(rep);
    uint chunk_size_after = rep->base_bits[0];

    printf("    Levels: before=%u, after=%u\n", levels_before, levels_after);
    printf("    Chunk size: before=%u, after=%u\n", chunk_size_before, chunk_size_after);
    printf("    Space saved: %u bits (%.1f%%)\n",
        bits_before - bits_after,
        100.0 * (bits_before - bits_after) / bits_before);

    // After replacing large values with small values, rebuild should reduce space
    VERIFY(bits_after < bits_before, "Should reduce space usage");
    VERIFY(chunk_size_after < chunk_size_before, "Should reduce chunk size");

    // Verify data integrity
    uint expected[] = { 10, 20, 30, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild after replaces");
    return 1;
}

/* Test 4: Rebuild after mixed operations */
int Test_RebuildAfterMixedOperations() {
    TEST_START("Rebuild after mixed operations");

    uint initial[] = { 100, 200, 300 };
    FTRep* rep = createFT(initial, 3);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Initial");

    // Mix of operations
    appendFT(rep, 400);
    insertFT(rep, 2, 150);
    replaceFT(rep, 4, 5000);
    appendFT(rep, 50);
    insertFT(rep, 1, 10);
    deleteFT(rep, 3);

    PrintStructureInfo(rep, "After mixed operations");

    uint bits_before = CalculateTotalBits(rep);

    // Get current values for verification
    uint* values_before = decompressFT(rep, rep->listLength);
    uint count = rep->listLength;

    // Rebuild
    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    uint bits_after = CalculateTotalBits(rep);

    printf("    Space: before=%u bits, after=%u bits\n", bits_before, bits_after);

    // Verify data integrity
    if (!VerifyAllValues(rep, values_before, count, "after rebuild")) {
        free(values_before);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        free(values_before);
        destroyFT(rep);
        return 0;
    }

    free(values_before);
    destroyFT(rep);
    TEST_PASS("Rebuild after mixed operations");
    return 1;
}

/* Test 5: Rebuild single element */
int Test_RebuildSingleElement() {
    TEST_START("Rebuild single element structure");

    uint initial[] = { 42 };
    FTRep* rep = createFT(initial, 1);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    VERIFY(rep->listLength == 1, "listLength should remain 1");
    if (!VerifyAccess(rep, 1, 42, "single element")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild single element");
    return 1;
}

/* Test 6: Rebuild with all same values */
int Test_RebuildAllSameValues() {
    TEST_START("Rebuild with all same values");

    uint initial[100];
    for (int i = 0; i < 100; i++) {
        initial[i] = 42;
    }

    FTRep* rep = createFT(initial, 100);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values
    if (!VerifyAllValues(rep, initial, 100, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild all same values");
    return 1;
}

/* Test 7: Rebuild with sequential values */
int Test_RebuildSequentialValues() {
    TEST_START("Rebuild with sequential values");

    uint initial[50];
    for (uint i = 0; i < 50; i++) {
        initial[i] = i * 100;
    }

    FTRep* rep = createFT(initial, 50);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values
    if (!VerifyAllValues(rep, initial, 50, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild sequential values");
    return 1;
}

/* Test 8: Rebuild with power-of-2 values */
int Test_RebuildPowerOf2Values() {
    TEST_START("Rebuild with power-of-2 values");

    uint initial[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
                       1024, 2048, 4096, 8192, 16384, 32768 };
    FTRep* rep = createFT(initial, 16);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values
    if (!VerifyAllValues(rep, initial, 16, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild power-of-2 values");
    return 1;
}

/* Test 9: Rebuild with boundary values */
int Test_RebuildBoundaryValues() {
    TEST_START("Rebuild with boundary values");

    uint initial[] = { 0, 1, 127, 128, 255, 256, 32767, 32768, 65535, 65536,
                       UINT_MAX - 1, UINT_MAX };
    FTRep* rep = createFT(initial, 12);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values
    if (!VerifyAllValues(rep, initial, 12, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild boundary values");
    return 1;
}

/* Test 10: Multiple consecutive rebuilds */
int Test_MultipleConsecutiveRebuilds() {
    TEST_START("Multiple consecutive rebuilds");

    uint initial[] = { 10, 20, 30, 40, 50 };
    FTRep* rep = createFT(initial, 5);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Initial");

    // First rebuild
    rebuildFT(rep);
    stats.total_rebuilds++;
    PrintStructureInfo(rep, "After rebuild 1");

    if (!VerifyAllValues(rep, initial, 5, "after rebuild 1")) {
        destroyFT(rep);
        return 0;
    }

    // Second rebuild (should be no-op)
    rebuildFT(rep);
    stats.total_rebuilds++;
    PrintStructureInfo(rep, "After rebuild 2");

    if (!VerifyAllValues(rep, initial, 5, "after rebuild 2")) {
        destroyFT(rep);
        return 0;
    }

    // Third rebuild (should be no-op)
    rebuildFT(rep);
    stats.total_rebuilds++;
    PrintStructureInfo(rep, "After rebuild 3");

    if (!VerifyAllValues(rep, initial, 5, "after rebuild 3")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after multiple rebuilds")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Multiple consecutive rebuilds");
    return 1;
}

/* Test 11: Rebuild after deletes */
int Test_RebuildAfterDeletes() {
    TEST_START("Rebuild after multiple deletes");

    uint initial[] = { 1000000, 10, 2000000, 20, 3000000, 30, 4000000, 40, 5000000, 50 };
    FTRep* rep = createFT(initial, 10);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Initial (mixed sizes)");

    // Delete all large values, leaving only small ones
    deleteFT(rep, 9);  // Remove 5000000
    deleteFT(rep, 7);  // Remove 4000000
    deleteFT(rep, 5);  // Remove 3000000
    deleteFT(rep, 3);  // Remove 2000000
    deleteFT(rep, 1);  // Remove 1000000

    PrintStructureInfo(rep, "After deletes (small values only)");

    uint levels_before = rep->nLevels;
    uint bits_before = CalculateTotalBits(rep);

    // Rebuild should create much more compact structure
    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    uint levels_after = rep->nLevels;
    uint bits_after = CalculateTotalBits(rep);

    printf("    Levels reduced: %u -> %u\n", levels_before, levels_after);
    printf("    Space saved: %u bits\n", bits_before - bits_after);

    // Verify remaining values
    uint expected[] = { 10, 20, 30, 40, 50 };
    if (!VerifyAllValues(rep, expected, 5, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild after deletes");
    return 1;
}

/* Test 12: Rebuild with zeros */
int Test_RebuildWithZeros() {
    TEST_START("Rebuild with zeros");

    uint initial[] = { 0, 0, 0, 100, 0, 200, 0, 0, 300, 0 };
    FTRep* rep = createFT(initial, 10);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values including zeros
    if (!VerifyAllValues(rep, initial, 10, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild with zeros");
    return 1;
}

/* Test 13: Rebuild random data */
int Test_RebuildRandomData() {
    TEST_START("Rebuild with random data");

    srand(12345);

    uint count = 100;
    uint* initial = malloc(sizeof(uint) * count);
    for (uint i = 0; i < count; i++) {
        initial[i] = (uint)rand() * (uint)rand();
    }

    FTRep* rep = createFT(initial, count);

    VERIFY(rep != NULL, "createFT failed");
    PrintStructureInfo(rep, "Before rebuild");

    rebuildFT(rep);
    stats.total_rebuilds++;

    PrintStructureInfo(rep, "After rebuild");

    // Verify all values
    if (!VerifyAllValues(rep, initial, count, "after rebuild")) {
        free(initial);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        free(initial);
        destroyFT(rep);
        return 0;
    }

    free(initial);
    destroyFT(rep);
    TEST_PASS("Rebuild random data");
    return 1;
}

/* Test 14: Rebuild consistency check */
int Test_RebuildConsistencyCheck() {
    TEST_START("Rebuild consistency with decompress");

    uint initial[] = { 100, 200, 1000, 5000, 10000, 50000, 100000 };
    FTRep* rep = createFT(initial, 7);

    VERIFY(rep != NULL, "createFT failed");

    // Check before rebuild
    if (!VerifyDecompressMatchesAccess(rep, "before rebuild")) {
        destroyFT(rep);
        return 0;
    }

    rebuildFT(rep);
    stats.total_rebuilds++;

    // Check after rebuild
    if (!VerifyDecompressMatchesAccess(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    if (!VerifyInternalStructure(rep, "after rebuild")) {
        destroyFT(rep);
        return 0;
    }

    destroyFT(rep);
    TEST_PASS("Rebuild consistency");
    return 1;
}

/* Test 15: Rebuild stress test */
int Test_RebuildStress() {
    TEST_START("Rebuild stress test with operations and rebuilds");

    srand(99999);

    uint count = 50;
    uint* values = malloc(sizeof(uint) * count * 2);  // Extra space for insertions
    for (uint i = 0; i < count; i++) {
        values[i] = (uint)rand() % 100000;
    }

    FTRep* rep = createFT(values, count);
    uint current_count = count;

    printf("    Performing random operations with periodic rebuilds...\n");

    for (uint cycle = 0; cycle < 5; cycle++) {
        printf("    Cycle %u: ", cycle + 1);

        // Random operations
        for (uint op = 0; op < 20; op++) {
            int op_type = rand() % 4;

            if (op_type == 0 && current_count < count * 2) {
                // Append
                uint new_val = (uint)rand() % 100000;
                appendFT(rep, new_val);
                values[current_count++] = new_val;
            }
            else if (op_type == 1 && current_count > 1) {
                // Delete
                uint pos = (rand() % current_count) + 1;
                deleteFT(rep, pos);
                memmove(&values[pos - 1], &values[pos],
                    (current_count - pos) * sizeof(uint));
                current_count--;
            }
            else if (op_type == 2 && current_count > 0) {
                // Replace
                uint pos = (rand() % current_count) + 1;
                uint new_val = (uint)rand() % 100000;
                replaceFT(rep, pos, new_val);
                values[pos - 1] = new_val;
            }
            else if (op_type == 3 && current_count > 0 && current_count < count * 2) {
                // Insert
                uint pos = (rand() % (current_count + 1)) + 1;
                uint new_val = (uint)rand() % 100000;
                insertFT(rep, pos, new_val);
                memmove(&values[pos], &values[pos - 1],
                    (current_count - pos + 1) * sizeof(uint));
                values[pos - 1] = new_val;
                current_count++;
            }
        }

        printf("%u elements, ", current_count);

        // Rebuild
        rebuildFT(rep);
        stats.total_rebuilds++;

        printf("rebuilt\n");

        // Verify
        if (!VerifyAllValues(rep, values, current_count, "stress cycle")) {
            printf("           Failed at cycle %u\n", cycle);
            free(values);
            destroyFT(rep);
            return 0;
        }
    }

    printf("    Final verification: %u elements\n", current_count);

    if (!VerifyInternalStructure(rep, "after stress test")) {
        free(values);
        destroyFT(rep);
        return 0;
    }

    if (!VerifyDecompressMatchesAccess(rep, "stress final")) {
        free(values);
        destroyFT(rep);
        return 0;
    }

    free(values);
    destroyFT(rep);
    TEST_PASS("Rebuild stress test");
    return 1;
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("rebuildFT UNIT TEST SUITE\n");
    printf("========================================\n");
    printf("* Testing rebuildFT functionality\n");
    printf("* Verifying data integrity\n");
    printf("* Verifying space optimization\n");
    printf("* Verifying internal consistency\n");
    printf("\n");

    /* Run all tests */
    int all_passed = 1;

    all_passed &= Test_RebuildAlreadyOptimal();
    all_passed &= Test_RebuildAfterAppends();
    all_passed &= Test_RebuildAfterReplaces();
    all_passed &= Test_RebuildAfterMixedOperations();
    all_passed &= Test_RebuildSingleElement();
    all_passed &= Test_RebuildAllSameValues();
    all_passed &= Test_RebuildSequentialValues();
    all_passed &= Test_RebuildPowerOf2Values();
    all_passed &= Test_RebuildBoundaryValues();
    all_passed &= Test_MultipleConsecutiveRebuilds();
    all_passed &= Test_RebuildAfterDeletes();
    all_passed &= Test_RebuildWithZeros();
    all_passed &= Test_RebuildRandomData();
    all_passed &= Test_RebuildConsistencyCheck();
    all_passed &= Test_RebuildStress();

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total tests:          %d\n", stats.total_tests);
    printf("Tests passed:         %d\n", stats.passed_tests);
    printf("Tests failed:         %d\n", stats.failed_tests);
    printf("\n");
    printf("Total rebuilds:       %d\n", stats.total_rebuilds);
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
