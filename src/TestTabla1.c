#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

// Structure to hold test results
typedef struct {
    char operation[20];
    int n_ops;
    double time_norebuild;
    double std_norebuild;
    uint size_norebuild;
    double time_rebuild;
    double std_rebuild;
    uint size_rebuild;
} TestResult;

// Function to calculate standard deviation
double calculateStdDev(double* times, int n, double mean) {
    double sum_sq_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = times[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / n);
}

// Function to get DDAC size by temporarily saving and checking file size
uint getDDACSizeInBytes(FTRep* rep, const char* temp_filename) {
    saveFT(rep, temp_filename);

    FILE* f = fopen(temp_filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open temporary file for size check\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    uint size = ftell(f);
    fclose(f);

    // Remove temporary file
    remove(temp_filename);

    return size;
}

// Function to read numbers from file
uint* readNumbers(const char* filename, int* count) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    int capacity = 1000;
    int num = 0;
    uint* numbers = (uint*)malloc(capacity * sizeof(uint));
    if (!numbers) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(f);
        return NULL;
    }

    uint value;
    while (fscanf(f, "%u", &value) == 1) {
        if (num >= capacity) {
            capacity *= 2;
            uint* temp = (uint*)realloc(numbers, capacity * sizeof(uint));
            if (!temp) {
                fprintf(stderr, "Error: Memory reallocation failed\n");
                free(numbers);
                fclose(f);
                return NULL;
            }
            numbers = temp;
        }
        numbers[num++] = value;
    }
    fclose(f);

    *count = num;
    return numbers;
}

// Function to copy FTRep structure by saving and reloading
FTRep* copyFTRep(FTRep* original, const char* temp_file) {
    saveFT(original, temp_file);
    FTRep* copy = loadFT(temp_file);
    remove(temp_file);
    return copy;
}

// Test appendFT operation
void testAppend(FTRep* base_rep, int n_ops, const char* numbers_file,
    double* time_norebuild, double* std_norebuild, uint* size_norebuild,
    double* time_rebuild, double* std_rebuild, uint* size_rebuild) {
    int count;
    uint* numbers = readNumbers(numbers_file, &count);
    if (!numbers || count != n_ops) {
        fprintf(stderr, "Error: Could not read %d numbers from %s\n", n_ops, numbers_file);
        *time_norebuild = *time_rebuild = 0;
        *std_norebuild = *std_rebuild = 0;
        *size_norebuild = *size_rebuild = 0;
        if (numbers) free(numbers);
        return;
    }

    struct timespec start, end;
    char temp_file[256];
    const int iterations = 3;
    double times_norebuild[iterations];
    double times_rebuild[iterations];

    // Run test iterations for mode 1: no rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_append_norebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep1 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            appendFT(rep1, numbers[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_norebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_append_norebuild_%d.ft", n_ops);
            *size_norebuild = getDDACSizeInBytes(rep1, temp_file);
        }

        destroyFT(rep1);
    }

    // Calculate mean and std dev
    double sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_norebuild[i];
    *time_norebuild = sum / iterations;
    *std_norebuild = calculateStdDev(times_norebuild, iterations, *time_norebuild);

    // Run test iterations for mode 2: rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_append_rebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep2 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            appendFT(rep2, numbers[i]);
        }
        rebuildFT(rep2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_rebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_append_rebuild_%d.ft", n_ops);
            *size_rebuild = getDDACSizeInBytes(rep2, temp_file);
        }

        destroyFT(rep2);
    }

    // Calculate mean and std dev
    sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_rebuild[i];
    *time_rebuild = sum / iterations;
    *std_rebuild = calculateStdDev(times_rebuild, iterations, *time_rebuild);

    free(numbers);
}

// Test insertFT operation
void testInsert(FTRep* base_rep, int n_ops, const char* numbers_file, const char* indices_file,
    double* time_norebuild, double* std_norebuild, uint* size_norebuild,
    double* time_rebuild, double* std_rebuild, uint* size_rebuild) {
    int count_nums, count_idx;
    uint* numbers = readNumbers(numbers_file, &count_nums);
    uint* indices = readNumbers(indices_file, &count_idx);

    if (!numbers || !indices || count_nums != n_ops || count_idx != n_ops) {
        fprintf(stderr, "Error: Could not read %d numbers/indices from %s/%s\n",
            n_ops, numbers_file, indices_file);
        *time_norebuild = *time_rebuild = 0;
        *std_norebuild = *std_rebuild = 0;
        *size_norebuild = *size_rebuild = 0;
        if (numbers) free(numbers);
        if (indices) free(indices);
        return;
    }

    struct timespec start, end;
    char temp_file[256];
    const int iterations = 3;
    double times_norebuild[iterations];
    double times_rebuild[iterations];

    // Run test iterations for mode 1: no rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_insert_norebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep1 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            insertFT(rep1, indices[i], numbers[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_norebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_insert_norebuild_%d.ft", n_ops);
            *size_norebuild = getDDACSizeInBytes(rep1, temp_file);
        }

        destroyFT(rep1);
    }

    double sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_norebuild[i];
    *time_norebuild = sum / iterations;
    *std_norebuild = calculateStdDev(times_norebuild, iterations, *time_norebuild);

    // Run test iterations for mode 2: rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_insert_rebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep2 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            insertFT(rep2, indices[i], numbers[i]);
        }
        rebuildFT(rep2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_rebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_insert_rebuild_%d.ft", n_ops);
            *size_rebuild = getDDACSizeInBytes(rep2, temp_file);
        }

        destroyFT(rep2);
    }

    sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_rebuild[i];
    *time_rebuild = sum / iterations;
    *std_rebuild = calculateStdDev(times_rebuild, iterations, *time_rebuild);

    free(numbers);
    free(indices);
}

// Test replaceFT operation
void testReplace(FTRep* base_rep, int n_ops, const char* numbers_file, const char* indices_file,
    double* time_norebuild, double* std_norebuild, uint* size_norebuild,
    double* time_rebuild, double* std_rebuild, uint* size_rebuild) {
    int count_nums, count_idx;
    uint* numbers = readNumbers(numbers_file, &count_nums);
    uint* indices = readNumbers(indices_file, &count_idx);

    if (!numbers || !indices || count_nums != n_ops || count_idx != n_ops) {
        fprintf(stderr, "Error: Could not read %d numbers/indices from %s/%s\n",
            n_ops, numbers_file, indices_file);
        *time_norebuild = *time_rebuild = 0;
        *std_norebuild = *std_rebuild = 0;
        *size_norebuild = *size_rebuild = 0;
        if (numbers) free(numbers);
        if (indices) free(indices);
        return;
    }

    struct timespec start, end;
    char temp_file[256];
    const int iterations = 3;
    double times_norebuild[iterations];
    double times_rebuild[iterations];

    // Run test iterations for mode 1: no rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_replace_norebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep1 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            replaceFT(rep1, indices[i], numbers[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_norebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_replace_norebuild_%d.ft", n_ops);
            *size_norebuild = getDDACSizeInBytes(rep1, temp_file);
        }

        destroyFT(rep1);
    }

    double sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_norebuild[i];
    *time_norebuild = sum / iterations;
    *std_norebuild = calculateStdDev(times_norebuild, iterations, *time_norebuild);

    // Run test iterations for mode 2: rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_replace_rebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep2 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            replaceFT(rep2, indices[i], numbers[i]);
        }
        rebuildFT(rep2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_rebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_replace_rebuild_%d.ft", n_ops);
            *size_rebuild = getDDACSizeInBytes(rep2, temp_file);
        }

        destroyFT(rep2);
    }

    sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_rebuild[i];
    *time_rebuild = sum / iterations;
    *std_rebuild = calculateStdDev(times_rebuild, iterations, *time_rebuild);

    free(numbers);
    free(indices);
}

// Test deleteFT operation
void testDelete(FTRep* base_rep, int n_ops, const char* indices_file,
    double* time_norebuild, double* std_norebuild, uint* size_norebuild,
    double* time_rebuild, double* std_rebuild, uint* size_rebuild) {
    int count_idx;
    uint* indices = readNumbers(indices_file, &count_idx);

    if (!indices || count_idx != n_ops) {
        fprintf(stderr, "Error: Could not read %d indices from %s\n", n_ops, indices_file);
        *time_norebuild = *time_rebuild = 0;
        *std_norebuild = *std_rebuild = 0;
        *size_norebuild = *size_rebuild = 0;
        if (indices) free(indices);
        return;
    }

    struct timespec start, end;
    char temp_file[256];
    const int iterations = 3;
    double times_norebuild[iterations];
    double times_rebuild[iterations];

    // Run test iterations for mode 1: no rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_delete_norebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep1 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            deleteFT(rep1, indices[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_norebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_delete_norebuild_%d.ft", n_ops);
            *size_norebuild = getDDACSizeInBytes(rep1, temp_file);
        }

        destroyFT(rep1);
    }

    double sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_norebuild[i];
    *time_norebuild = sum / iterations;
    *std_norebuild = calculateStdDev(times_norebuild, iterations, *time_norebuild);

    // Run test iterations for mode 2: rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_delete_rebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep2 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            deleteFT(rep2, indices[i]);
        }
        rebuildFT(rep2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_rebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_delete_rebuild_%d.ft", n_ops);
            *size_rebuild = getDDACSizeInBytes(rep2, temp_file);
        }

        destroyFT(rep2);
    }

    sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_rebuild[i];
    *time_rebuild = sum / iterations;
    *std_rebuild = calculateStdDev(times_rebuild, iterations, *time_rebuild);

    free(indices);
}

// Test pop() operation - delete from the end
void testPop(FTRep* base_rep, int n_ops,
    double* time_norebuild, double* std_norebuild, uint* size_norebuild,
    double* time_rebuild, double* std_rebuild, uint* size_rebuild) {
    struct timespec start, end;
    char temp_file[256];
    const int iterations = 3;
    double times_norebuild[iterations];
    double times_rebuild[iterations];

    // Run test iterations for mode 1: no rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_pop_norebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep1 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            deleteFT(rep1, rep1->listLength);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_norebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_pop_norebuild_%d.ft", n_ops);
            *size_norebuild = getDDACSizeInBytes(rep1, temp_file);
        }

        destroyFT(rep1);
    }

    double sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_norebuild[i];
    *time_norebuild = sum / iterations;
    *std_norebuild = calculateStdDev(times_norebuild, iterations, *time_norebuild);

    // Run test iterations for mode 2: rebuild
    for (int iter = 0; iter < iterations; iter++) {
        sprintf(temp_file, "temp_copy_pop_rebuild_%d_%d.ft", n_ops, iter);
        FTRep* rep2 = copyFTRep(base_rep, temp_file);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < n_ops; i++) {
            deleteFT(rep2, rep2->listLength);
        }
        rebuildFT(rep2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        times_rebuild[iter] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        if (iter == 0) {
            sprintf(temp_file, "temp_size_pop_rebuild_%d.ft", n_ops);
            *size_rebuild = getDDACSizeInBytes(rep2, temp_file);
        }

        destroyFT(rep2);
    }

    sum = 0.0;
    for (int i = 0; i < iterations; i++) sum += times_rebuild[i];
    *time_rebuild = sum / iterations;
    *std_rebuild = calculateStdDev(times_rebuild, iterations, *time_rebuild);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <base_ddac_file> <output_csv_file>\n", argv[0]);
        fprintf(stderr, "\nExpected files in current directory:\n");
        fprintf(stderr, "  Elements: 1.txt, 10.txt, 100.txt, 1000.txt\n");
        fprintf(stderr, "  Indices: 1.indices.txt, 10.indices.txt, 100.indices.txt, 1000.indices.txt\n");
        fprintf(stderr, "\nNote: pop() operation doesn't require any input files\n");
        return 1;
    }

    char* base_file = argv[1];
    char* output_csv = argv[2];

    printf("Loading base DDAC structure from: %s\n", base_file);
    FTRep* base_rep = loadFT(base_file);
    if (!base_rep) {
        fprintf(stderr, "Error: Could not load base DDAC file\n");
        return 1;
    }
    printf("Base structure loaded successfully (size: %u elements)\n", base_rep->listLength);
    printf("Each test will run 3 iterations and report average times with standard deviation\n\n");

    // Array to store all results
    TestResult results[20];  // 5 operations × 4 N values
    int result_idx = 0;

    int n_values[] = { 1, 10, 100, 1000 };
    const char* operations[] = { "appendFT", "insertFT", "replaceFT", "deleteFT", "pop()" };

    // Test each operation
    for (int op = 0; op < 5; op++) {
        printf("Testing %s...\n", operations[op]);

        for (int n_idx = 0; n_idx < 4; n_idx++) {
            int n = n_values[n_idx];
            printf("  N = %d operations...\n", n);

            char numbers_file[50];
            char indices_file[50];
            sprintf(numbers_file, "%d.txt", n);
            sprintf(indices_file, "%d.indices.txt", n);

            TestResult* res = &results[result_idx++];
            strcpy(res->operation, operations[op]);
            res->n_ops = n;

            if (op == 0) {  // appendFT
                testAppend(base_rep, n, numbers_file,
                    &res->time_norebuild, &res->std_norebuild, &res->size_norebuild,
                    &res->time_rebuild, &res->std_rebuild, &res->size_rebuild);
            }
            else if (op == 1) {  // insertFT
                testInsert(base_rep, n, numbers_file, indices_file,
                    &res->time_norebuild, &res->std_norebuild, &res->size_norebuild,
                    &res->time_rebuild, &res->std_rebuild, &res->size_rebuild);
            }
            else if (op == 2) {  // replaceFT
                testReplace(base_rep, n, numbers_file, indices_file,
                    &res->time_norebuild, &res->std_norebuild, &res->size_norebuild,
                    &res->time_rebuild, &res->std_rebuild, &res->size_rebuild);
            }
            else if (op == 3) {  // deleteFT
                testDelete(base_rep, n, indices_file,
                    &res->time_norebuild, &res->std_norebuild, &res->size_norebuild,
                    &res->time_rebuild, &res->std_rebuild, &res->size_rebuild);
            }
            else {  // pop()
                testPop(base_rep, n,
                    &res->time_norebuild, &res->std_norebuild, &res->size_norebuild,
                    &res->time_rebuild, &res->std_rebuild, &res->size_rebuild);
            }

            printf("    No rebuild: avg time=%.6f s (+/-%.9f), size=%u bytes\n",
                res->time_norebuild, res->std_norebuild, res->size_norebuild);
            printf("    Rebuild:    avg time=%.6f s (+/-%.9f), size=%u bytes\n",
                res->time_rebuild, res->std_rebuild, res->size_rebuild);
        }
        printf("\n");
    }

    // Write results to CSV
    printf("Writing results to: %s\n", output_csv);
    FILE* csv = fopen(output_csv, "w");
    if (!csv) {
        fprintf(stderr, "Error: Cannot create output CSV file\n");
        destroyFT(base_rep);
        return 1;
    }

    // Write header
    fprintf(csv, "Operacion,Estructura,N° Operaciones,Tiempo Ejecucion (s),Desv. Est.,Tamaño (bytes),Tiempo Ejecucion (s),Desv. Est.,Tamaño (bytes)\n");
    fprintf(csv, ",,,DDAC (NO rebuild),,,DDAC (rebuild),,\n");

    // Write data for each operation
    for (int op = 0; op < 5; op++) {
        // Write operation name row (with empty column C)
        fprintf(csv, "%s,SPSI,,,,,,,\n", operations[op]);

        // Write data rows (with empty columns A and B)
        for (int n_idx = 0; n_idx < 4; n_idx++) {
            TestResult* res = &results[op * 4 + n_idx];

            fprintf(csv, ",,%d,%.9f,%.9f,%u,%.9f,%.9f,%u\n",
                res->n_ops,
                res->time_norebuild, res->std_norebuild, res->size_norebuild,
                res->time_rebuild, res->std_rebuild, res->size_rebuild);
        }
    }

    fclose(csv);
    destroyFT(base_rep);

    printf("\nAll tests completed successfully!\n");
    printf("Results saved to: %s\n", output_csv);

    return 0;
}