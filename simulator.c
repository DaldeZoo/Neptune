// MIPS (CPU) Simulator
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define MEMORY_SIZE 1024
#define ZERO_REG 0
#define RA_REG 31

uint32_t memory[MEMORY_SIZE]; // main memory (RAM): MEMORY_SIZE rows of words (32-bit instructions)
// TODO: implement stack
uint32_t registers[32];
uint32_t pc = 0;
uint32_t hi;
uint32_t lo;

int g_lines_read = 0;

char check_extension(const char* filename, const char* extension) {
    // returns 0 if extensions are the same, nonzero otherwise
    size_t filename_len = strlen(filename);
    size_t ext_len = strlen(extension);
    return strcmp(filename + (filename_len - ext_len), extension);
}
char load_instructions(const char* filename, const char* extension) {
    // loads a MIPS machine code (binary file) f onto memory
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        perror("Error in opening file");
        return 1;
    }
    if (check_extension(filename, extension)) {
        fprintf(stderr, "File %s does not have the correct extension %s\n", filename, extension);
        fclose(f);
        return 1;
    }

    size_t i = 0;
    while (i < MEMORY_SIZE && (fread(&memory[i], sizeof(uint32_t), 1, f)) == 1) {
        g_lines_read++;
        i++;
    }

    if (i == MEMORY_SIZE) {
        fprintf(stderr, "Warning: file exceeded memory size! Stopped after %d instructions\n");
    }

    fclose(f);
    return 0;
}

// !maybe change switch statements into lookup table for modularity and efficiency...
// WAIT WHAT?! Switch statements... O(1)?! Compiler Optimization?!!! GCC??

// TODO: account for special registers like $zero, $at, etc...
void r_type_execute(uint32_t instruction, uint32_t func) {
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t rd = (instruction >> 11) & 0x1F;
    uint32_t shamt = (instruction >> 6) & 0x1F;

    int64_t result;
    uint64_t uresult;
    switch (func) {
        case 0x20: // add
            if (rd != ZERO_REG) registers[rd] = registers[rs] + registers[rt];
            // overflow check
            break;

        case 0x21: // addu
            if (rd != ZERO_REG) registers[rd] = registers[rs] + registers[rt];
            break;

        case 0x1A: // div
            if (registers[rt] != 0) {
                lo = registers[rs] / registers[rt];
                hi = registers[rs] % registers[rt];
            }
            break;

        case 0x1B: // divu
            if (registers[rt] != 0) {
                lo = (unsigned int)registers[rs] / (unsigned int)registers[rt];
                hi = (unsigned int)registers[rs] % (unsigned int)registers[rt];
            }
            break;

        case 0x18: // mult (signed multiplication)
            result = (int64_t)registers[rs] * (int64_t)registers[rt];
            lo = (uint32_t)(result & 0xFFFFFFFF);  // Lower 32 bits
            hi = (uint32_t)((result >> 32) & 0xFFFFFFFF);  // Upper 32 bits
            break;

        case 0x19: // multu (unsigned multiplication)
            uresult = (uint64_t)(uint32_t)registers[rs] * (uint64_t)(uint32_t)registers[rt];
            lo = (uint32_t)(uresult & 0xFFFFFFFF);  // Lower 32 bits
            hi = (uint32_t)((uresult >> 32) & 0xFFFFFFFF);  // Upper 32 bits
            break;

        case 0x22: // sub
            registers[rd] = registers[rs] - registers[rt];
            // overflow check
            break;

        case 0x23: // subu
            registers[rd] = registers[rs] - registers[rt];
            break;

        case 0x24: // and
            registers[rd] = registers[rs] & registers[rt];
            break;

        case 0x25: // or
            registers[rd] = registers[rs] | registers[rt];
            break;

        case 0x26: // xor
            registers[rd] = registers[rs] ^ registers[rt];
            break;

        case 0x27: // nor
            registers[rd] = ~(registers[rs] | registers[rt]);
            break;

        case 0x0: // sll
            registers[rd] = registers[rt] << shamt;
            break;

        case 0x2: // srl
            registers[rd] = registers[rt] >> shamt;
            break;

        case 0x3: // sra
            registers[rd] = (int)registers[rt] >> shamt;
            break;

        case 0x12: // mflo (move from LO register)
            registers[rd] = lo;  // Move the value from the LO register to the destination register (rd)
            break;

        case 0x10: // mfhi (move from HI register)
            registers[rd] = hi;  // Move the value from the HI register to the destination register (rd)
            break;
        
        case 0x09: // jalr
            registers[RA_REG] = pc + 1;  // Save return address in $ra (register 31)
            pc = registers[rs];      // Jump to the address in register $rs
            break;
        
        case 0x08: // jr
            pc = registers[rs];  // Jump to the address in register $rs
            break;
    }
}
void j_type_execute(uint32_t instruction, uint32_t opcode) {
    uint32_t address = instruction & 0x3FFFFFF;  // Extract the 26-bit address (last 26 bits of instruction)
    
    switch (opcode) {
        case 0x2: // j (Jump)
            // Set the new program counter by taking the upper 4 bits of the current PC and appending the 26-bit address
            pc = (pc & 0xF0000000) | (address << 2);
            break;

        case 0x3: // jal (Jump and Link)
            registers[RA_REG] = pc + 1;  // Store the return address in $ra (address of next instruction)
            pc = (pc & 0xF0000000) | (address << 2);  // Set the program counter to the jump address
            break;
    }
}
void i_type_execute(uint32_t instruction, uint32_t opcode) {
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t immediate = instruction & 0xFFFF;         // Extract immediate (16 bits)
    int32_t signed_immediate = (int16_t)immediate;    // Sign-extend the immediate for signed operations

    switch (opcode) {
        case 0x8: // addi (Add Immediate)
            registers[rt] = registers[rs] + signed_immediate;
            break;
        
        case 0x9: // addiu (Add Immediate Unsigned)
            registers[rt] = registers[rs] + signed_immediate;
            break;

        case 0xC: // andi (AND Immediate)
            registers[rt] = registers[rs] & immediate;
            break;

        case 0xD: // ori (OR Immediate)
            registers[rt] = registers[rs] | immediate;
            break;

        case 0xE: // xori (XOR Immediate)
            registers[rt] = registers[rs] ^ immediate;
            break;

        case 0x4: // beq (Branch if Equal)
            if (registers[rs] == registers[rt]) {
                pc += signed_immediate << 2;  // Branch offset is in words, not bytes
            }
            break;

        case 0x5: // bne (Branch if Not Equal)
            if (registers[rs] != registers[rt]) {
                pc += signed_immediate << 2;  // Branch offset is in words, not bytes
            }
            break;

        case 0x6: // blez (Branch if Less Than or Equal to Zero)
            if ((int32_t)registers[rs] <= 0) {
                pc += signed_immediate << 2;  // Branch offset is in words, not bytes
            }
            break;

        case 0x7: // bgtz (Branch if Greater Than Zero)
            if ((int32_t)registers[rs] > 0) {
                pc += signed_immediate << 2;  // Branch offset is in words, not bytes
            }
            break;

        case 0xA: // slti (Set on Less Than Immediate)
            registers[rt] = ((int32_t)registers[rs] < signed_immediate) ? 1 : 0;
            break;

        case 0xB: // sltiu (Set on Less Than Immediate Unsigned)
            registers[rt] = ((unsigned int)registers[rs] < (unsigned int)immediate) ? 1 : 0;
            break;

        case 0x20: // lb (Load Byte)
            registers[rt] = (int8_t)memory[registers[rs] + signed_immediate];
            break;

        case 0x24: // lw (Load Word)
            registers[rt] = memory[registers[rs] + signed_immediate];
            break;

        case 0x28: // sw (Store Word)
            memory[registers[rs] + signed_immediate] = registers[rt];
            break;

        case 0x29: // sh (Store Halfword)
            memory[registers[rs] + signed_immediate] = (uint16_t)registers[rt];
            break;

        case 0x2A: // sb (Store Byte)
            memory[registers[rs] + signed_immediate] = (uint8_t)registers[rt];
            break;

        case 0x18: // mflo (Move From LO)
            registers[rt] = lo;
            break;

        case 0x19: // mfhi (Move From HI)
            registers[rt] = hi;
            break;

        case 0x10: // beqz (Branch if Equal to Zero) - Example pseudo-instruction
            if (registers[rs] == 0) {
                pc += signed_immediate << 2;  // Branch offset is in words, not bytes
            }
            break;

        default:
            printf("Unknown I-type instruction with opcode: %x\n", opcode);
            break;
    }
}

void execute_instruction(uint32_t instruction) {
    // decodes and executes instruction
    uint32_t opcode = (instruction >> 26) & 0x3F; // extract last 6/32 bits

    if (opcode == 0x0) {
        uint32_t func = instruction & 0x3F; // extract first 6 bits
        r_type_execute(instruction, func);
    }
    else if (opcode == 0x2 || opcode == 0x3) j_type_execute(instruction, opcode); // j and jal
    else i_type_execute(instruction, opcode);
    // add one for system calls?
}
// let assembler translate psuedoinstructions like li and lw

void print_current_register_values() {
    for (int i=0; i<32; i++) {
        printf("Register %d has value %d\n", i, registers[i]);
    }
}

void print_binary(uint32_t num) {
    printf("0b");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);  // Shift and mask each bit
    }
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];
    printf("%s\n", filename);
    char* dot = strrchr(filename, '.');
    char extension[10];
    if (dot != NULL && dot != filename) {
        strncpy(extension, dot + 1, sizeof(extension) - 1);
        extension[sizeof(extension) - 1] = '\0';
        printf("Extension: %s\n", extension);
    } else { printf("No extension found.\n"); return 1; }

    load_instructions(argv[1], extension);
    // printf("In test.txt\n");
    // printf("        001000 00000 01000 0000000000000010 // addi $0, $8, 2\n"
    //        "        001000 01000 00100 0000000000000011 // addi $8, $4, 3\n");
    
    printf("\n\n\n");

    for (int i=0;i<32;i++) registers[i] = 0;
    printf("lineas read %d\n", g_lines_read);
    while (pc < g_lines_read) {
        printf("Memory Instruction: "); print_binary(memory[pc]);
        execute_instruction(memory[pc]);
        printf("After executing line %d:\n", pc);
        print_current_register_values();
        pc++;
    }
    return 0;
}

// hmmm...