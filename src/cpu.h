#include<stdint.h>

// -- Register names --

#define A 0 // accumulator
#define B 1
#define C 2
#define D 3
#define E 4
#define H 5
#define L 6
#define SP 7 // stack pointer
#define M 8 // memory reference
#define PSW 9

// -- System state --

typedef struct condition_codes {
    uint8_t z;
    uint8_t s;
    uint8_t p;
    uint8_t cy;
    uint8_t ac;
    //uint8_t pad;
} condition_codes;

typedef struct system_state {
    uint8_t regs[7]; // registers
    uint16_t sp; // stack pointer
    uint16_t pc; //program counter
    uint8_t *memory;
    condition_codes cc;
    uint8_t int_enable;
} system_state;

// -- Exported functions

int emulate_op(system_state *state);
