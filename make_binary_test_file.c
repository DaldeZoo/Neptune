#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_binary_file(const char *filename, const char *binary_str) {
    // Length of the binary string should be a multiple of 8 (1 byte = 8 bits)
    size_t len = strlen(binary_str);
    if (len % 8 != 0) {
        printf("Error: Binary string length must be a multiple of 8.\n");
        return;
    }

    // Open the file for writing in binary mode
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file");
        return;
    }

    // Process the binary string and write each byte
    for (size_t i = 0; i < len; i += 8) {
        char byte = 0;
        // Convert 8 bits into a single byte
        for (size_t j = 0; j < 8; ++j) {
            byte |= (binary_str[i + j] - '0') << (7 - j);
        }

        // Write the byte to the file
        fwrite(&byte, sizeof(char), 1, file);
    }

    // Close the file
    fclose(file);
    printf("Binary data written to %s\n", filename);
}

int main() {
    const char *binary_str = "0010000000001000000000000000001000100001000001000000000000000011";
    const char *filename = "output.bin";

    write_binary_file(filename, binary_str);

    return 0;
}
