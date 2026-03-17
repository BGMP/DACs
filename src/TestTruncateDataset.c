#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    FILE* f_in = fopen(argv[1], "rb");
    if (!f_in) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    // Get file size in bytes
    fseek(f_in, 0, SEEK_END);
    unsigned int size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    unsigned int max_size_mb = 200;
    unsigned int max_size = max_size_mb * 1024 * 1024;

    unsigned int bytes_to_copy = size;
    if (size > max_size) {
        printf("File size %u bytes exceeds %uMB limit. Truncating to %uMB.\n",
            size, max_size_mb, max_size_mb);
        bytes_to_copy = max_size;
    }
    else {
        printf("File size %u bytes is within %uMB limit. Copying entire file.\n",
            size, max_size_mb);
    }

    FILE* f_out = fopen(argv[2], "wb");
    if (!f_out) {
        fprintf(stderr, "Error: Cannot create output file %s\n", argv[2]);
        fclose(f_in);
        return 1;
    }

    // Copy data in chunks
    unsigned int chunk_size = 1024 * 1024; // 1MB chunks
    unsigned char* buffer = (unsigned char*)malloc(chunk_size);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(f_in);
        fclose(f_out);
        return 1;
    }

    unsigned int bytes_copied = 0;
    while (bytes_copied < bytes_to_copy) {
        unsigned int to_read = (bytes_to_copy - bytes_copied < chunk_size) ?
            (bytes_to_copy - bytes_copied) : chunk_size;
        unsigned int bytes_read = fread(buffer, 1, to_read, f_in);
        if (bytes_read == 0) break;

        fwrite(buffer, 1, bytes_read, f_out);
        bytes_copied += bytes_read;
    }

    free(buffer);
    fclose(f_in);
    fclose(f_out);

    printf("Successfully copied %u bytes to %s\n", bytes_copied, argv[2]);

    return 0;
}