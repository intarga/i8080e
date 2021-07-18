#include<stdint.h>

#define CPUDIAG 0

#define DISASSEMBLE_IN_EMULATION 0
#define PRINT_STATE 0

// -- Register names --

enum Register {
    A, // accumulator
    B,
    C,
    D,
    E,
    H,
    L,
    SP, // stack pointer
    M, // memory reference
    PSW,
};

// -- System state --

typedef struct {
    uint8_t z;
    uint8_t s;
    uint8_t p;
    uint8_t cy;
    uint8_t ac;
    //uint8_t pad;
} Condition_codes;

typedef struct {
    uint8_t regs[7]; // registers
    uint16_t sp; // stack pointer
    uint16_t pc; //program counter
    uint8_t *memory;
    Condition_codes cc;
    uint8_t int_enable;
} Cpu_state;

// -- Exported functions

uint8_t read_memory(Cpu_state *state, uint16_t address);
void write_memory(Cpu_state *state, uint16_t address, uint8_t value);
int emulate_op(Cpu_state *state);
int interrupt(Cpu_state *state, uint16_t offset);
