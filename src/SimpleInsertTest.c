#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "dacs.h"

// Helper function to validate bitmap structure
int validate_bitmap_structure(FTRep* rep, const char* context) {
    int errors = 0;

    printf("\n--- Validating Bitmap Structure (%s) ---\n", context);

    for (uint i = 0; i < rep->nLevels - 1; i++) {
        bitRankW32Int* bsi = rep->bS[i];

        // Count 1s and 0s in the bitmap
        uint ones = rank(bsi, bsi->n - 1);
        uint zeros = bsi->n - ones;

        printf("Level %u bitmap:\n", i);
        printf("  Total elements (bS[%u]->n): %u\n", i, bsi->n);
        printf("  Zeros (continue to next level): %u\n", zeros);
        printf("  Ones (terminate at this level): %u\n", ones);

        // Validate that ZEROS equals next level's size
        uint expected_next_size;
        if (i < rep->nLevels - 2) {
            expected_next_size = rep->bS[i + 1]->n;
        }
        else {
            expected_next_size = rep->sizeLastLevel;
        }

        printf("  Expected next level size: %u\n", expected_next_size);

        if (zeros != expected_next_size) {
            printf("  ERROR: Bitmap zeros (%u) != next level size (%u)\n", zeros, expected_next_size);
            errors++;
        }
        else {
            printf("  OK: Bitmap consistent\n");
        }
    }

    if (errors == 0) {
        printf("Bitmap structure validation: PASS\n");
    }
    else {
        printf("Bitmap structure validation: FAIL (%d errors)\n", errors);
    }

    return errors;
}

// Helper function to validate pointers
int validate_pointers(FTRep* rep, const char* context) {
    int errors = 0;

    printf("\n--- Validating Pointers (%s) ---\n", context);

    // Check base_bits array
    if (rep->base_bits[0] != 0) {
        printf("base_bits array:\n");
        for (uint i = 0; i < rep->nLevels; i++) {
            printf("  base_bits[%u] = %u\n", i, rep->base_bits[i]);
        }
    }

    // Check bitmap sizes
    printf("\nBitmap sizes:\n");
    for (uint i = 0; i < rep->nLevels - 1; i++) {
        printf("  bS[%u]->n = %u\n", i, rep->bS[i]->n);

        // Check that bitmap data exists and size makes sense
        if (rep->bS[i]->data == NULL) {
            printf("  ERROR: bS[%u]->data is NULL\n", i);
            errors++;
        }
    }

    printf("Last level size: %u\n", rep->sizeLastLevel);

    if (errors == 0) {
        printf("Pointer validation: PASS\n");
    }
    else {
        printf("Pointer validation: FAIL (%d errors)\n", errors);
    }

    return errors;
}

// Helper function to validate level sizes
int validate_level_sizes(FTRep* rep, const char* context) {
    int errors = 0;

    printf("\n--- Validating Level Sizes (%s) ---\n", context);

    printf("Total listLength: %u\n", rep->listLength);
    printf("Number of levels: %u\n", rep->nLevels);

    // First level should have listLength elements
    if (rep->nLevels > 1 && rep->bS[0]->n != rep->listLength) {
        printf("ERROR: First level size (%u) != listLength (%u)\n",
            rep->bS[0]->n, rep->listLength);
        errors++;
    }
    else if (rep->nLevels > 1) {
        printf("OK: First level size matches listLength\n");
    }

    // Check decreasing sizes through levels
    for (uint i = 0; i < rep->nLevels - 2; i++) {
        if (rep->bS[i + 1]->n > rep->bS[i]->n) {
            printf("ERROR: Level %u size (%u) > Level %u size (%u)\n",
                i + 1, rep->bS[i + 1]->n, i, rep->bS[i]->n);
            errors++;
        }
    }

    if (errors == 0) {
        printf("Level size validation: PASS\n");
    }
    else {
        printf("Level size validation: FAIL (%d errors)\n", errors);
    }

    return errors;
}

// Helper function to validate full structure
int validate_full_structure(FTRep* rep, const char* context) {
    printf("\n========================================\n");
    printf("FULL STRUCTURE VALIDATION: %s\n", context);
    printf("========================================\n");

    int total_errors = 0;

    total_errors += validate_bitmap_structure(rep, context);
    total_errors += validate_pointers(rep, context);
    total_errors += validate_level_sizes(rep, context);

    printf("\n--- Overall Structure Status ---\n");
    if (total_errors == 0) {
        printf("PASS: Structure is internally consistent\n");
    }
    else {
        printf("FAIL: Found %d structural errors\n", total_errors);
    }

    return total_errors;
}

// Helper to test decompression consistency
int validate_decompression(FTRep* rep, const char* context) {
    printf("\n--- Testing Full Decompression (%s) ---\n", context);

    printf("Attempting to decompress all %u elements...\n", rep->listLength);

    uint* decompressed = decompressFT(rep, rep->listLength);
    if (decompressed == NULL) {
        printf("ERROR: Decompression failed (returned NULL)\n");
        return 1;
    }

    printf("Decompression successful, verifying against accessFT...\n");

    int errors = 0;
    int sample_size = rep->listLength < 1000 ? rep->listLength : 1000;

    // Sample positions throughout the array
    for (int i = 0; i < sample_size; i++) {
        // Sample evenly distributed positions
        uint pos = (rep->listLength * i) / sample_size;
        if (pos == 0) pos = 1;  // 1-based indexing

        uint access_val = accessFT(rep, pos);
        uint decomp_val = decompressed[pos - 1];  // decompressed is 0-indexed

        if (access_val != decomp_val) {
            if (errors < 10) {  // Print first 10 errors
                printf("ERROR at position %u: accessFT=%u, decompressed=%u\n",
                    pos, access_val, decomp_val);
            }
            errors++;
        }
    }

    free(decompressed);

    if (errors == 0) {
        printf("Decompression validation: PASS (sampled %d positions)\n", sample_size);
    }
    else {
        printf("Decompression validation: FAIL (%d errors in %d samples)\n",
            errors, sample_size);
    }

    return errors;
}

void print_separator(const char* title) {
    printf("\n");
    printf("========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ddacwtb_file>\n", argv[0]);
        return 1;
    }

    printf("Loading FT from: %s\n", argv[1]);
    FTRep* rep = loadFT(argv[1]);
    printf("Loaded: %u elements, %u levels\n", rep->listLength, rep->nLevels);

    int total_errors = 0;

    // ========================================
    // TEST 1: Validate initial structure
    // ========================================
    print_separator("TEST 1: Initial Structure Validation");

    total_errors += validate_full_structure(rep, "Initial state");
    total_errors += validate_decompression(rep, "Initial state");

    if (total_errors > 0) {
        printf("\nERROR: Initial structure is corrupted! Cannot proceed.\n");
        destroyFT(rep);
        return 1;
    }

    // ========================================
    // TEST 2: Single insertion at beginning
    // ========================================
    print_separator("TEST 2: Single Insertion at Beginning");

    uint original_first = accessFT(rep, 1);
    printf("Inserting 99999 at position 1...\n");
    printf("Original first element: %u\n", original_first);

    insertFT(rep, 1, 99999);

    printf("\nVerifying insertion:\n");
    uint new_first = accessFT(rep, 1);
    uint shifted_first = accessFT(rep, 2);

    printf("  Position 1: %u (expected 99999) %s\n",
        new_first, new_first == 99999 ? "OK" : "FAIL");
    printf("  Position 2: %u (expected %u) %s\n",
        shifted_first, original_first, shifted_first == original_first ? "OK" : "FAIL");

    if (new_first != 99999 || shifted_first != original_first) {
        total_errors++;
    }

    total_errors += validate_full_structure(rep, "After insertion at position 1");
    total_errors += validate_decompression(rep, "After insertion at position 1");

    // ========================================
    // TEST 3: Multiple sequential insertions
    // ========================================
    print_separator("TEST 3: Multiple Sequential Insertions");

    destroyFT(rep);
    rep = loadFT(argv[1]);

    printf("Performing 10 sequential insertions at positions 100-109...\n");

    for (uint i = 0; i < 10; i++) {
        uint pos = 100 + i;
        uint val = 11111 + i * 1111;
        insertFT(rep, pos, val);
        printf("  Inserted %u at position %u\n", val, pos);

        // Validate after each insertion
        if (i % 3 == 0) {  // Validate every 3rd insertion
            int errors = validate_full_structure(rep, "During sequential insertions");
            if (errors > 0) {
                printf("ERROR: Structure corrupted after insertion #%u\n", i + 1);
                total_errors += errors;
                break;
            }
        }
    }

    printf("\nVerifying all 10 insertions...\n");
    int verify_errors = 0;
    for (uint i = 0; i < 10; i++) {
        uint pos = 100 + i;
        uint expected = 11111 + i * 1111;
        uint actual = accessFT(rep, pos);

        printf("  Position %u: expected=%u, got=%u %s\n",
            pos, expected, actual, actual == expected ? "OK" : "FAIL");

        if (actual != expected) {
            verify_errors++;
        }
    }

    if (verify_errors > 0) {
        total_errors += verify_errors;
    }

    total_errors += validate_full_structure(rep, "After 10 sequential insertions");
    total_errors += validate_decompression(rep, "After 10 sequential insertions");

    // ========================================
    // TEST 4: Scattered insertions
    // ========================================
    print_separator("TEST 4: Scattered Insertions");

    destroyFT(rep);
    rep = loadFT(argv[1]);

    typedef struct {
        uint pos;
        uint val;
    } InsertOp;

    InsertOp scattered[] = {
        {50, 10001},
        {500, 10002},
        {5000, 10003},
        {50000, 10004},
        {100000, 10005}
    };

    printf("Performing 5 scattered insertions...\n");
    for (uint i = 0; i < 5; i++) {
        if (scattered[i].pos <= rep->listLength + 1) {
            printf("  Inserting %u at position %u (listLength: %u)\n",
                scattered[i].val, scattered[i].pos, rep->listLength);

            insertFT(rep, scattered[i].pos, scattered[i].val);

            // Validate structure after each scattered insertion
            int errors = validate_full_structure(rep, "After scattered insertion");
            if (errors > 0) {
                printf("ERROR: Structure corrupted after inserting at position %u\n",
                    scattered[i].pos);
                total_errors += errors;
                break;
            }
        }
    }

    printf("\nVerifying scattered insertions...\n");
    verify_errors = 0;
    for (uint i = 0; i < 5; i++) {
        if (scattered[i].pos <= rep->listLength) {
            uint actual = accessFT(rep, scattered[i].pos);
            printf("  Position %u: expected=%u, got=%u %s\n",
                scattered[i].pos, scattered[i].val, actual,
                actual == scattered[i].val ? "OK" : "FAIL");

            if (actual != scattered[i].val) {
                verify_errors++;
            }
        }
    }

    if (verify_errors > 0) {
        total_errors += verify_errors;
    }

    total_errors += validate_decompression(rep, "After scattered insertions");

    // ========================================
    // TEST 5: Large batch of insertions with detailed tracking
    // ========================================
    print_separator("TEST 5: Large Batch (100 insertions) with Debug Tracking");

    destroyFT(rep);
    rep = loadFT(argv[1]);

    printf("Performing 100 insertions at positions 1000, 1011, 1022, ...\n");
    printf("Detailed tracking will start at insertion #76\n\n");

    uint batch_size = 100;
    uint* inserted_values = malloc(sizeof(uint) * batch_size);

    for (uint i = 0; i < batch_size; i++) {
        uint pos = 1000 + i * 11;
        uint val = 50000 + i * 100;
        inserted_values[i] = val;

        insertFT(rep, pos, val);

        // Detailed tracking starting from insertion 76
        if (i >= 75) {
            // Check bitmap consistency for the problematic levels
            uint level3_zeros = 0;
            uint level3_size = 0;
            uint level4_size = 0;

            if (rep->nLevels >= 5) {
                level3_size = rep->bS[3]->n;
                uint level3_ones = rank(rep->bS[3], rep->bS[3]->n - 1);
                level3_zeros = rep->bS[3]->n - level3_ones;
            }

            if (rep->nLevels >= 6) {
                level4_size = rep->bS[4]->n;
            }
            else if (rep->nLevels >= 5) {
                level4_size = rep->sizeLastLevel;
            }

            printf("Insert #%u (pos=%u, val=%u):\n", i + 1, pos, val);
            printf("  nLevels=%u\n", rep->nLevels);
            printf("  Level3: size=%u, zeros=%u\n", level3_size, level3_zeros);
            printf("  Level4: size=%u\n", level4_size);
            printf("  Difference: %d\n", (int)level3_zeros - (int)level4_size);

            if (level3_zeros != level4_size) {
                printf("  *** CORRUPTION DETECTED ***\n\n");

                // Show full structure state
                printf("Full structure state at corruption:\n");
                for (uint j = 0; j < rep->nLevels; j++) {
                    if (j < rep->nLevels - 1) {
                        uint ones = rank(rep->bS[j], rep->bS[j]->n - 1);
                        uint zeros = rep->bS[j]->n - ones;
                        printf("  Level %u: bS[%u]->n=%u, zeros=%u, ones=%u\n",
                            j, j, rep->bS[j]->n, zeros, ones);
                    }
                    else {
                        printf("  Level %u: sizeLastLevel=%u\n", j, rep->sizeLastLevel);
                    }
                }

                break;
            }
            printf("\n");
        }
        else if (i % 20 == 0) {
            printf("  Progress: %u/%u insertions\n", i, batch_size);
        }
    }

    printf("\nVerifying sample of batch insertions...\n");
    verify_errors = 0;
    for (uint i = 0; i < batch_size; i += 10) {  // Sample every 10th
        uint pos = 1000 + i * 11;
        uint expected = 50000 + i * 100;
        uint actual = accessFT(rep, pos);

        if (actual != expected) {
            printf("  Position %u: expected=%u, got=%u FAIL\n", pos, expected, actual);
            verify_errors++;
        }
    }

    if (verify_errors == 0) {
        printf("  All sampled insertions verified OK\n");
    }
    else {
        printf("  Found %d errors in sampled insertions\n", verify_errors);
        total_errors += verify_errors;
    }

    free(inserted_values);

    total_errors += validate_full_structure(rep, "After 100 insertions");
    total_errors += validate_decompression(rep, "After 100 insertions");

    // ========================================
    // FINAL SUMMARY
    // ========================================
    print_separator("FINAL SUMMARY");

    if (total_errors == 0) {
        printf("\nOVERALL: ALL TESTS PASSED\n");
        printf("insertFT maintains structural integrity:\n");
        printf("  - Bitmaps remain consistent\n");
        printf("  - Pointers remain valid\n");
        printf("  - Level sizes remain correct\n");
        printf("  - Decompression works correctly\n");
        printf("  - All inserted values retrievable\n");
    }
    else {
        printf("\nOVERALL: TESTS FAILED\n");
        printf("Total errors found: %d\n", total_errors);
        printf("insertFT has structural integrity issues\n");
    }

    destroyFT(rep);
    return total_errors > 0 ? 1 : 0;
}