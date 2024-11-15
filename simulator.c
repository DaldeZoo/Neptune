// MIPS (CPU) Simulator
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define MEMORY_SIZE 1024

uint32_t memory[MEMORY_SIZE]; // main memory (RAM) - 1024 rows of words (32-bit instructions)
uint32_t registers[32];
uint32_t pc = 0;

char check_extension(const char* filename, const char* extension) {
    // returns 0 if extensions are the same, nonzero otherwise
    size_t filename_len = strlen(filename);
    size_t ext_len = strlen(extension);
    return strcmp(filename + (filename_len - ext_len), extension);
}
void load_instructions(const char* filename, const char* extension) {
    // loads a MIPS machine code (binary file) f onto memory
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        perror("Error in opening file");
        return;
    }
    if (check_extension(filename, extension)) {
        fprintf(stderr, "File %s does not have the correct extension %s\n", filename, extension);
        fclose(f);
        return;
    }

    size_t i = 0;
    while (i < MEMORY_SIZE && fread(&memory[i], sizeof(uint32_t), 1, f) == 1) {
        i++;
    }

    if (i == MEMORY_SIZE) {
        fprintf(stderr, "Warning: file exceeded memory size! Stopped after %d instructions\n");
    }

    fclose(f);
}

void r_type_execute() {

}

void j_type_execute() {
    
}

void i_type_execute() {
    
}

void d_instruction(uint32_t instruction) {
    // decodes and executes instruction
    uint32_t opcode = (instruction >> 26) & 0b111111; // extract last 6/32 bits

    if (opcode == 0x00000000) {
        uint32_t func = instruction & 0b111111; // extract first 6 (im reading right to left)
        r_type_execute(instruction, func);
    }
    else if (opcode == 0x2 || opcode == 0x3) j_type_execute(instruction, opcode); // j and jal
    else i_type_instruction(instruction, opcode);
    // add one for system calls?
}

int main() {
    while (pc < MEMORY_SIZE) {
        execute_instruction(memory[pc]);
    }
}