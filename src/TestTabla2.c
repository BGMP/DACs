#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "constants.h"
#include "dacs.h"

// Function to calculate standard deviation
double calculateStdDev(double* times, int n, double mean) {
    double sum_sq_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = times[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / (n - 1));  // Sample standard deviation
}

// Function to shuffle array using Fisher-Yates algorithm
void shuffle(uint* array, uint n) {
    if (n > 1) {
        for (uint i = n - 1; i > 0; i--) {
            uint j = rand() % (i + 1);
            uint temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }
}

// Function to get DDAC size by temporarily saving and checking file size
uint getDDACSizeInBytes(FTRep* rep) {
    char temp_file[] = "temp_ddac_size_check.ft";
    saveFT(rep, temp_file);

    FILE* f = fopen(temp_file, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open temporary file for size check\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    uint size = ftell(f);
    fclose(f);

    // Remove temporary file
    remove(temp_file);

    return size;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <1|2> <output_csv>\n", argv[0]);
        fprintf(stderr, "  Mode 1: RAW\n");
        fprintf(stderr, "  Mode 2: LCP\n");
        fprintf(stderr, "  output_csv: Path to output CSV file\n");
        return 1;
    }

    char* input_file = argv[1];
    int mode = atoi(argv[2]);
    char* output_csv = argv[3];

    if (mode != 1 && mode != 2) {
        fprintf(stderr, "Error: Mode must be 1 (RAW) or 2 (LCP)\n");
        return 1;
    }

    FILE* f;
    uint size;
    uint* list;
    uint i;
    double time_create, time_access, time_decompress;
    double std_access;
    clock_t begin, end;

    // Load data according to mode
    if (mode == 1) {
        // RAW mode - from ValidateCreateRAW.c
        f = fopen(input_file, "rb");
        if (!f) {
            fprintf(stderr, "Error: Cannot open file %s\n", input_file);
            return 1;
        }

        // Get file size in bytes
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);

        uint max_size_mb = 200;
        uint max_size = max_size_mb * 1024 * 1024;
        if (size > max_size) {
            printf("File size exceeds %uMB limit. Capping to %uMB.\n", max_size_mb, max_size_mb);
            size = max_size;
        }

        list = (uint*)malloc(sizeof(uint) * size);
        if (!list) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(f);
            return 1;
        }

        byte b;
        i = 0;
        // Read byte by byte (like DYNAMIC does)
        while (i < size && fread(&b, sizeof(byte), 1, f) == 1) {
            list[i] = (uint)b;  // Convert each byte to uint (0-255)
            i++;
        }
        fclose(f);
    }
    else {
        // LCP mode - from ValidateCreateLCP.c
        f = fopen(input_file, "rb");
        if (!f) {
            fprintf(stderr, "Error: Cannot open file %s\n", input_file);
            return 1;
        }

        fseek(f, 0, SEEK_END);
        size = ftell(f) / 4;
        fseek(f, 0, SEEK_SET);

        list = (uint*)malloc(sizeof(uint) * size);
        if (!list) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(f);
            return 1;
        }

        i = 0;
        while (feof(f) == 0 && i < size) {
            fread(&list[i], sizeof(uint), 1, f);
            i++;
        }
        fclose(f);
    }

    // Calculate original size (uncompressed)
    uint original_size_bytes = (mode == 1) ? size : size * sizeof(uint);
    double original_size_mb = original_size_bytes / (1024.0 * 1024.0);

    // Measure createFT time
    begin = clock();
    FTRep* listRep = createFT(list, size);
    end = clock();
    time_create = (double)(end - begin) / CLOCKS_PER_SEC;

    // Get DDAC size (compressed)
    uint ddac_size = getDDACSizeInBytes(listRep);
    double ddac_size_mb = ddac_size / (1024.0 * 1024.0);

    // Create array of random positions (all positions in random order)
    uint* random_positions = (uint*)malloc(sizeof(uint) * listRep->listLength);
    if (!random_positions) {
        fprintf(stderr, "Error: Memory allocation failed for random positions\n");
        free(list);
        return 1;
    }

    // Initialize positions array with sequential values
    for (i = 0; i < listRep->listLength; i++) {
        random_positions[i] = i + 1;  // Positions are 1-indexed
    }

    // Shuffle to create random order
    srand(12345);  // Fixed seed for reproducibility
    shuffle(random_positions, listRep->listLength);

    // Measure accessFT time with multiple iterations for standard deviation
    const int access_iterations = 5;
    double access_times[access_iterations];

    for (int iter = 0; iter < access_iterations; iter++) {
        begin = clock();
        for (i = 0; i < listRep->listLength; i++) {
            accessFT(listRep, random_positions[i]);
        }
        end = clock();
        access_times[iter] = (double)(end - begin) / CLOCKS_PER_SEC;
    }

    // Calculate mean access time
    double sum = 0.0;
    for (int iter = 0; iter < access_iterations; iter++) {
        sum += access_times[iter];
    }
    time_access = sum / access_iterations;

    // Calculate standard deviation
    std_access = calculateStdDev(access_times, access_iterations, time_access);

    // Calculate average access time per operation in microseconds
    double avg_access_time_us = (time_access * 1000000.0) / listRep->listLength;
    double std_access_time_us = (std_access * 1000000.0) / listRep->listLength;

    // Measure decompressFT time
    begin = clock();
    decompressFT(listRep, listRep->listLength);
    end = clock();
    time_decompress = (double)(end - begin) / CLOCKS_PER_SEC;

    // Output results to console
    printf("Nombre Archivo: %s\n", input_file);
    printf("Peso DDAC (bytes): %u\n", ddac_size);
    printf("Tiempo accessFT (microsegundos/acceso): %.9f (+/-%.9f)\n", avg_access_time_us, std_access_time_us);
    printf("Tiempo createFT (segundos): %.9f\n", time_create);
    printf("Tiempo decompressFT (segundos): %.9f\n", time_decompress);

    // Write results to CSV
    FILE* csv = fopen(output_csv, "w");
    if (!csv) {
        fprintf(stderr, "Error: Cannot create output CSV file %s\n", output_csv);
        free(list);
        free(random_positions);
        return 1;
    }

    // Write CSV header - ALL ASCII, no unicode
    fprintf(csv, "Tamano original (bytes),Tamano original (MB),Tamano compactado (bytes),Tamano compactado (MB),accessFT Promedio (us),accessFT Desviacion Estandar (us),createFT (s),decompressFT (s)\n");

    // Write data row - with COMMAS
    fprintf(csv, "%u,%.9f,%u,%.9f,%.9f,%.9f,%.9f,%.9f\n",
        original_size_bytes, original_size_mb,
        ddac_size, ddac_size_mb,
        avg_access_time_us, std_access_time_us,
        time_create, time_decompress);

    fclose(csv);
    printf("\nResults written to: %s\n", output_csv);

    // Cleanup
    free(list);
    free(random_positions);

    return 0;
}
