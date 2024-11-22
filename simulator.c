// MIPS (CPU) Simulator
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define MEMORY_SIZE 1024
#define REG_ra 31

uint32_t memory[MEMORY_SIZE]; // main memory (RAM) - 1024 rows of words (32-bit instructions)
uint32_t registers[32];
uint32_t pc = 0;
uint32_t hi;
uint32_t lo;

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

// !maybe change switch statements into lookup table for modularity and efficiency...
// WAIT WHAT?! Switch statements... O(1)?! Compiler Optimization?!!! GCC?? TF?!

// TODO: account for special registers like $zero, $at, etc...
void r_type_execute(uint32_t instruction, uint32_t func) {
    uint32_t rs = (instruction >> 21) & 0b11111;
    uint32_t rt = (instruction >> 16) & 0b11111;
    uint32_t rd = (instruction >> 11) & 0b11111;
    uint32_t shamt = (instruction >> 6) & 0b11111;

    switch (func) {
        case 0b100000: // add
            registers[rd] = registers[rs] + registers[rt];
            // overflow check
            break;

        case 0b100001: // addu
            registers[rd] = registers[rs] + registers[rt];
            break;

        case 0b011010: // div
            if (registers[rt] != 0) {
                lo = registers[rs] / registers[rt];
                hi = registers[rs] % registers[rt];
            }
            break;

        case 0b011011: // divu
            if (registers[rt] != 0) {
                lo = (unsigned int)registers[rs] / (unsigned int)registers[rt];
                hi = (unsigned int)registers[rs] % (unsigned int)registers[rt];
            }
            break;

        case 0b011000: // mult (signed multiplication)
            int64_t result = (int64_t)registers[rs] * (int64_t)registers[rt];
            lo = (uint32_t)(result & 0xFFFFFFFF);  // Lower 32 bits
            hi = (uint32_t)((result >> 32) & 0xFFFFFFFF);  // Upper 32 bits
            break;

        case 0b011001: // multu (unsigned multiplication)
            uint64_t uresult = (uint64_t)(uint32_t)registers[rs] * (uint64_t)(uint32_t)registers[rt];
            lo = (uint32_t)(uresult & 0xFFFFFFFF);  // Lower 32 bits
            hi = (uint32_t)((uresult >> 32) & 0xFFFFFFFF);  // Upper 32 bits
            break;

        case 0b100010: // sub
            registers[rd] = registers[rs] - registers[rt];
            // overflow check
            break;

        case 0b100011: // subu
            registers[rd] = registers[rs] - registers[rt];
            break;

        case 0b100100: // and
            registers[rd] = registers[rs] & registers[rt];
            break;

        case 0b100101: // or
            registers[rd] = registers[rs] | registers[rt];
            break;

        case 0b100110: // xor
            registers[rd] = registers[rs] ^ registers[rt];
            break;

        case 0b100111: // nor
            registers[rd] = ~(registers[rs] | registers[rt]);
            break;

        case 0b000000: // sll
            registers[rd] = registers[rt] << shamt;
            break;

        case 0b000010: // srl
            registers[rd] = registers[rt] >> shamt;
            break;

        case 0b000011: // sra
            registers[rd] = (int)registers[rt] >> shamt;
            break;

        case 0b000100: // sllv (shift left logical variable)
            registers[rd] = registers[rt] << registers[rs];  // Shift left by the value in rs
            break;

        case 0b000110: // srlv (shift right logical variable)
            registers[rd] = registers[rt] >> registers[rs];  // Shift right by the value in rs
            break;

        case 0b000111: // srav (shift right arithmetic variable)
            registers[rd] = (int)registers[rt] >> registers[rs];  // Arithmetic shift right by the value in rs
            break;

        case 0b010010: // mflo (move from LO register)
            registers[rd] = lo;  // Move the value from the LO register to the destination register (rd)
            break;

        case 0b010001: // mfhi (move from HI register)
            registers[rd] = hi;  // Move the value from the HI register to the destination register (rd)
            break;
        
        case 0b001001: // jalr
            registers[REG_ra] = pc + 1;  // Save return address in $ra (register 31)
            pc = registers[rs];      // Jump to the address in register $rs
            break;
        
        case 0b001000: // jr
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
            registers[REG_ra] = pc + 1;  // Store the return address in $ra (address of next instruction)
            pc = (pc & 0xF0000000) | (address << 2);  // Set the program counter to the jump address
            break;
    }
}
void i_type_execute(uint32_t instruction, uint32_t opcode) {
    uint32_t rs = (instruction >> 21) & 0b11111;
    uint32_t rt = (instruction >> 16) & 0b11111;
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
    uint32_t opcode = (instruction >> 26) & 0b111111; // extract last 6/32 bits

    if (opcode == 0x00000000) {
        uint32_t func = instruction & 0b111111; // extract first 6 (im reading right to left)
        r_type_execute(instruction, func);
    }
    else if (opcode == 0x2 || opcode == 0x3) j_type_execute(instruction, opcode); // j and jal
    else i_type_instruction(instruction, opcode);
    // add one for system calls?
}
// let assembler translate psuedoinstructions like li and lw

int main() {
    while (pc < MEMORY_SIZE) {
        execute_instruction(memory[pc]);
        pc += 1;
    }
}