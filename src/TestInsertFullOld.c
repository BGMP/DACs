#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <stdint.h>
#include "dacs.h"

typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int total_accesses;
    int failed_accesses;
    int integrity_checks;
    int integrity_failures;
} TestStats;

static TestStats stats = { 0 };

static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

/* ============================================================================
 * STRUCTURE INTEGRITY
 * ============================================================================
 */

static int VerifyStructureIntegrity(FTRep* rep, const char* ctx) {
    stats.integrity_checks++;
    int ok = 1;

    printf("  [Integrity: %s]\n", ctx);

    if (rep->nLevels == 0 || rep->nLevels > 64) {
        printf("    FAIL: nLevels=%u\n", rep->nLevels);
        ok = 0;
    }

    if (rep->listLength == 0 && rep->nLevels > 0) {
        printf("    FAIL: listLength=0 but nLevels=%u\n", rep->nLevels);
        ok = 0;
    }

    for (uint i = 0; i < rep->nLevels; i++) {
        if (!rep->base[i] || !rep->base_bits[i]) {
            printf("    FAIL: base/base_bits NULL at level %u\n", i);
            ok = 0;
        }
        if (i < rep->nLevels - 1 && rep->bS[i]->n == 0) {
            printf("    FAIL: bitmap[%u] empty\n", i);
            ok = 0;
        }
    }

    if (rep->sizeLastLevel > rep->listLength) {
        printf("    FAIL: sizeLastLevel=%u listLength=%u\n",
            rep->sizeLastLevel, rep->listLength);
        ok = 0;
    }

    if (!ok) stats.integrity_failures++;
    else printf("    PASS\n");

    return ok;
}

/* ============================================================================
 * SEQUENCE VERIFICATION
 * ============================================================================
 */

static int VerifySequence(
    FTRep* rep,
    const uint* expected,
    uint len,
    const char* name
) {
    int ok = 1;
    int shown = 0;

    for (uint i = 0; i < len; i++) {
        uint got = accessFT(rep, i + 1);
        stats.total_accesses++;

        if (got != expected[i]) {
            if (shown < 10) {
                printf("  FAIL [%s] pos=%u exp=%u got=%u\n",
                    name, i + 1, expected[i], got);
                shown++;
            }
            stats.failed_accesses++;
            ok = 0;
        }
    }

    return ok;
}

/* ============================================================================
 * REFERENCE ARRAY INSERT
 * ============================================================================
 */

static void ref_insert(uint** arr, uint* len, uint pos, uint val) {
    assert(pos >= 1 && pos <= *len + 1);

    *arr = realloc(*arr, sizeof(uint) * (*len + 1));
    for (uint i = *len; i >= pos; i--) {
        (*arr)[i] = (*arr)[i - 1];
        if (i == pos) break;
    }
    (*arr)[pos - 1] = val;
    (*len)++;
}

/* ============================================================================
 * TEST CASES
 * ============================================================================
 */

static void TestInsertBeginning(const char* file) {
    printf("\n=== Insert at Beginning ===\n");
    stats.total_tests++;

    FTRep* rep = loadFT(file);
    if (!rep) return;

    VerifyStructureIntegrity(rep, "before");

    uint len = rep->listLength;
    uint* ref = malloc(sizeof(uint) * len);
    for (uint i = 0; i < len; i++) ref[i] = accessFT(rep, i + 1);

    for (uint i = 0; i < 100; i++) {
        ref_insert(&ref, &len, 1, i * 10);
        insertFT(rep, 1, i * 10);
    }

    int ok = VerifySequence(rep, ref, len, "begin");
    ok &= VerifyStructureIntegrity(rep, "after");

    free(ref);
    destroyFT(rep);

    ok ? stats.passed_tests++ : stats.failed_tests++;
}

static void TestInsertEnd(const char* file) {
    printf("\n=== Insert at End ===\n");
    stats.total_tests++;

    FTRep* rep = loadFT(file);
    if (!rep) return;

    uint len = rep->listLength;
    uint* ref = malloc(sizeof(uint) * len);
    for (uint i = 0; i < len; i++) ref[i] = accessFT(rep, i + 1);

    for (uint i = 0; i < 100; i++) {
        ref_insert(&ref, &len, len + 1, 5000 + i);
        insertFT(rep, rep->listLength + 1, 5000 + i);
    }

    int ok = VerifySequence(rep, ref, len, "end");
    ok &= VerifyStructureIntegrity(rep, "after");

    free(ref);
    destroyFT(rep);

    ok ? stats.passed_tests++ : stats.failed_tests++;
}

/* ============================================================================
 * REALISTIC MIDDLE-SPAN INSERT TEST
 * ============================================================================
 */

static void TestInsertMiddleSpan(const char* file) {
    printf("\n=== Insert Every ~1000 Elements (Bounded) ===\n");
    stats.total_tests++;

    FTRep* rep = loadFT(file);
    if (!rep) return;

    VerifyStructureIntegrity(rep, "before");

    uint len = rep->listLength;
    uint* ref = malloc(sizeof(uint) * len);
    for (uint i = 0; i < len; i++) ref[i] = accessFT(rep, i + 1);

    const uint SPAN = 30000;
    const uint MAX_INSERTS = 30000;

    uint inserts = len / SPAN;
    if (inserts > MAX_INSERTS)
        inserts = MAX_INSERTS;

    printf("  Performing %u insertions...\n", inserts);

    double t0 = get_time();

    for (uint i = 0; i < inserts; i++) {
        uint pos = 1 + (uint)((uint64_t)i * rep->listLength / inserts);
        uint val = 1 + i;

        ref_insert(&ref, &len, pos, val);
        insertFT(rep, pos, val);

        if ((i + 1) % 5000 == 0) {
            printf("    %u/%u inserts (%.1f sec)\n",
                i + 1, inserts, get_time() - t0);
        }

		printf("\r  Inserted value=%u %u/%u\n", val, i + 1, inserts);
    }

    printf("  Insert phase: %.2f sec\n", get_time() - t0);

    int ok = VerifySequence(rep, ref, len, "middle_span");
    ok &= VerifyStructureIntegrity(rep, "after");

    free(ref);
    destroyFT(rep);

    ok ? stats.passed_tests++ : stats.failed_tests++;
}

static void TestInsertRandom(const char* file) {
    printf("\n=== Random Inserts ===\n");
    stats.total_tests++;

    FTRep* rep = loadFT(file);
    if (!rep) return;

    uint len = rep->listLength;
    uint* ref = malloc(sizeof(uint) * len);
    for (uint i = 0; i < len; i++) ref[i] = accessFT(rep, i + 1);

    srand(12345);

    for (uint i = 0; i < 500; i++) {
        uint pos = 1 + rand() % (len + 1);
        uint val = (uint)rand() * (uint)rand();

        ref_insert(&ref, &len, pos, val);
        insertFT(rep, pos, val);
    }

    int ok = VerifySequence(rep, ref, len, "random");
    ok &= VerifyStructureIntegrity(rep, "after");

    free(ref);
    destroyFT(rep);

    ok ? stats.passed_tests++ : stats.failed_tests++;
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ddac-file>\n", argv[0]);
        return 1;
    }

    double t0 = get_time();

    TestInsertBeginning(argv[1]);
    TestInsertEnd(argv[1]);
    TestInsertMiddleSpan(argv[1]);
    TestInsertRandom(argv[1]);

    double t1 = get_time();

    printf("\n========================================\n");
    printf("Tests run: %d\n", stats.total_tests);
    printf("Passed:    %d\n", stats.passed_tests);
    printf("Failed:    %d\n", stats.failed_tests);
    printf("Accesses:  %d\n", stats.total_accesses);
    printf("Failures:  %d\n", stats.failed_accesses);
    printf("Integrity: %d checks, %d fails\n",
        stats.integrity_checks, stats.integrity_failures);
    printf("Time:      %.3f sec\n", t1 - t0);
    printf("========================================\n");

    return stats.failed_tests ? 1 : 0;
}
