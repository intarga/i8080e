#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
    //uint8_t int_enable;
} system_state;

// -- Helper functions --

uint8_t check_parity(uint8_t res, int bits) {
    int p = 0;
    for (int i = 0; i < bits; i++) {
        if ((res >>= 1) & 0x1)
            p++;
    }
    return ((p & 0x1) == 0);
}

void set_zsp(system_state *state, uint8_t res) {
    state->cc.z = (res == 0x00);
    state->cc.s = ((res & 0x80) == 0x80);
    state->cc.p = check_parity(res, 8);
}

uint16_t get_m_address(system_state *state, uint8_t reg1, uint8_t reg2) {
    return (state->regs[reg1] << 8) | state->regs[reg2];
}

uint16_t get_immediate_address(system_state *state) {
     return (state->memory[state->pc + 2] << 8) | state->memory[state->pc + 1];
}

// -- Carry bit instructions --

void STC(system_state *state) {
    state->cc.cy = 1;
}

// -- Single register instructions --

void INR(system_state *state, uint8_t reg) {
    uint8_t res;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        res = ++state->memory[address];
    } else {
        res = ++state->regs[reg];
    }

    set_zsp(state, res);
    state->cc.ac = ((res & 0x0f) == 0x00);
}

void DCR(system_state *state, uint8_t reg) {
    uint8_t res;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        res = --state->memory[address];
    } else {
        res = --state->regs[reg];
    }

    set_zsp(state, res);
    state->cc.ac = ((res & 0x0f) == 0x0f);
}

void CMA(system_state *state) {
    state->regs[A] = ~state->regs[A];
}

void DAA(system_state *state) {
    if (state->cc.ac || (state->regs[A] & 0x0f) > 9) {
        state->cc.ac = 1;
        state->regs[A] += 6;
    }

    if (state->cc.cy || (state->regs[A] >> 4) > 9) {
        state->cc.cy = 1;
        state->regs[A] = ((state->regs[A] & 0xf0) + (6 << 4)) + (state->regs[A] &0x0f);
    }
}

// -- Data transfer instructions --

void STAX(system_state *state, uint8_t reg) {
    uint16_t address = get_m_address(state, reg, reg + 1);
    state->memory[address] = state->regs[A];
}

void LDAX(system_state *state, uint8_t reg) {
    uint16_t address = get_m_address(state, reg, reg + 1);
    state->regs[A] = state->memory[address];
}

// -- Rotate accumulator instructions --

void RLC(system_state *state) {
    state->cc.cy = (state->regs[A] & 0x80) != 0;

    state->regs[A] <<= 1;
    if (state->cc.cy)
        state->regs[A]++;
}

void RRC(system_state *state) {
    state->cc.cy = (state->regs[A] & 0x01) != 0;

    state->regs[A] >>= 1;
    if (state->cc.cy)
        state->regs[A] += 0x80;
}

void RAL(system_state *state) {
    uint8_t oldcy = state->cc.cy;
    state->cc.cy = (state->regs[A] & 0x80) != 0;

    state->regs[A] <<= 1;
    state->regs[A] += oldcy;
}

void RAR(system_state *state) {
    uint8_t oldcy = state->cc.cy;
    state->cc.cy = (state->regs[A] & 0x01) != 0;

    state->regs[A] >>= 1;
    state->regs[A] += (oldcy * 0x80);
}

// -- Register pair instructions --

void DAD(system_state *state, uint8_t reg) {
    uint16_t add1 = (state->regs[reg] << 8) | state->regs[reg + 1];
    uint16_t add2 = (state->regs[H] << 8) | state->regs[L];

    uint32_t sum = add1 + add2;

    state->cc.cy = ((sum & 0x00010000) != 0);

    state->regs[reg] = ((sum >> 8) & 0x000000ff);
    state->regs[reg + 1] = (sum & 0x000000ff);
}

void INX(system_state *state, uint8_t reg) {
    if (reg == SP) {
        state->sp++;
    } else {
        if (state->regs[reg + 1]++ == 0xff)
            state->regs[reg]++;
    }
}

void DCX(system_state *state, uint8_t reg) {
    if (reg == SP) {
        state->sp--;
    } else {
        if (state->regs[reg + 1]-- == 0x00)
            state->regs[reg]--;
    }
}

// -- Immediate instructions --

void LXI(system_state *state, uint8_t reg) {
    unsigned char byte1 = state->memory[state->pc + 1];
    unsigned char byte2 = state->memory[state->pc + 2];

    if (reg == SP) {
        state->sp = (byte2 << 8) | byte1;
    } else {
        state->regs[reg] = byte2;
        state->regs[reg + 1] = byte1;
    }

    state->pc += 2;
}

void MVI(system_state *state, uint8_t reg) {
    unsigned char byte = state->memory[state->pc + 1];

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        state->memory[address] = byte;
    } else {
        state->regs[reg] = byte;
    }

    state->pc++;
}

// -- Direct addressing instructions --

void STA(system_state *state) {
    uint16_t address = get_immediate_address(state);
    state->memory[address] = state->regs[A];
}

void LDA(system_state *state) {
    uint16_t address = get_immediate_address(state);
    state->regs[A] = state->memory[address];
}

void SHLD(system_state *state) {
    uint16_t address = get_immediate_address(state);
    state->memory[address] = state->regs[L];
    state->memory[address + 1] = state->regs[H];

    state->pc += 2;
}

void LHLD(system_state *state) {
    uint16_t address = get_immediate_address(state);
    state->regs[L] = state->memory[address];
    state->regs[H] = state->memory[address + 1];

    state->pc += 2;
}

// -- The emulation nation --

int emulate_op(system_state *state) {
    unsigned char op_code = state->memory[state->pc];

    switch (op_code) {
    case 0x00: break; // NOP
    case 0x01: LXI(state, B);   break;
    case 0x02: STAX(state, B);  break;
    case 0x03: INX(state, B);   break;
    case 0x04: INR(state, B);   break;
    case 0x05: DCR(state, B);   break;
    case 0x06: MVI(state, B);   break;
    case 0x07: RLC(state);      break;

    case 0x08: exit(1); // undocumented instruction!! break;
    case 0x09: DAD(state, B);   break;
    case 0x0a: LDAX(state, B);  break;
    case 0x0b: DCX(state, B);   break;
    case 0x0c: INR(state, C);   break;
    case 0x0d: DCR(state, C);   break;
    case 0x0e: MVI(state, C);   break;
    case 0x0f: RRC(state);      break;

    case 0x10: exit(1); // undocumented instruction!! break;
    case 0x11: LXI(state, D);   break;
    case 0x12: STAX(state, D);  break;
    case 0x13: INX(state, D);   break;
    case 0x14: INR(state, D);   break;
    case 0x15: DCR(state, D);   break;
    case 0x16: MVI(state, D);   break;
    case 0x17: RAL(state);      break;

    case 0x18: exit(1); // undocumented instruction!! break;
    case 0x19: DAD(state, D);   break;
    case 0x1a: LDAX(state, D);  break;
    case 0x1b: DCX(state, D);   break;
    case 0x1c: INR(state, E);   break;
    case 0x1d: DCR(state, E);   break;
    case 0x1e: MVI(state, E);   break;
    case 0x1f: RAR(state);      break;

    case 0x20: exit(1); // undocumented instruction!! break;
    case 0x21: LXI(state, H);   break;
    case 0x22: SHLD(state);     break;
    case 0x23: INX(state, H);   break;
    case 0x24: INR(state, H);   break;
    case 0x25: DCR(state, H);   break;
    case 0x26: MVI(state, H);   break;
    case 0x27: DAA(state);      break;

    case 0x28: exit(1); // undocumented instruction!! break;
    case 0x29: DAD(state, H);   break;
    case 0x2a: LHLD(state);     break;
    case 0x2b: DCX(state, H);   break;
    case 0x2c: INR(state, L);   break;
    case 0x2d: DCR(state, L);   break;
    case 0x2e: MVI(state, L);   break;
    case 0x2f: CMA(state);      break;

    case 0x30: exit(1); // undocumented instruction!! break;
    case 0x31: LXI(state, SP);  break;
    case 0x32: STA(state);      break;
    case 0x33: INX(state, SP);  break;
    case 0x34: INR(state, M);   break;
    case 0x35: DCR(state, M);   break;
    case 0x36: MVI(state, M);   break;
    case 0x37: STC(state);      break;

    case 0x38: exit(1); // undocumented instruction!! break;
    case 0x39: DAD(state, SP);  break;
    case 0x3a: LDA(state);      break;
    case 0x3b: DCX(state, SP);  break;
    case 0x3c: INR(state, A);   break;
    case 0x3d: DCR(state, A);   break;
    case 0x3e: MVI(state, A);   break;
    case 0x3f: break;

    case 0x40: break;
    case 0x41: break;
    case 0x42: break;
    case 0x43: break;
    case 0x44: break;
    case 0x45: break;
    case 0x46: break;
    case 0x47: break;

    case 0x48: break;
    case 0x49: break;
    case 0x4a: break;
    case 0x4b: break;
    case 0x4c: break;
    case 0x4d: break;
    case 0x4e: break;
    case 0x4f: break;

    case 0x50: break;
    case 0x51: break;
    case 0x52: break;
    case 0x53: break;
    case 0x54: break;
    case 0x55: break;
    case 0x56: break;
    case 0x57: break;

    case 0x58: break;
    case 0x59: break;
    case 0x5a: break;
    case 0x5b: break;
    case 0x5c: break;
    case 0x5d: break;
    case 0x5e: break;
    case 0x5f: break;

    case 0x60: break;
    case 0x61: break;
    case 0x62: break;
    case 0x63: break;
    case 0x64: break;
    case 0x65: break;
    case 0x66: break;
    case 0x67: break;

    case 0x68: break;
    case 0x69: break;
    case 0x6a: break;
    case 0x6b: break;
    case 0x6c: break;
    case 0x6d: break;
    case 0x6e: break;
    case 0x6f: break;

    case 0x70: break;
    case 0x71: break;
    case 0x72: break;
    case 0x73: break;
    case 0x74: break;
    case 0x75: break;
    case 0x76: break;
    case 0x77: break;

    case 0x78: break;
    case 0x79: break;
    case 0x7a: break;
    case 0x7b: break;
    case 0x7c: break;
    case 0x7d: break;
    case 0x7e: break;
    case 0x7f: break;

    case 0x80: break;
    case 0x81: break;
    case 0x82: break;
    case 0x83: break;
    case 0x84: break;
    case 0x85: break;
    case 0x86: break;
    case 0x87: break;

    case 0x88: break;
    case 0x89: break;
    case 0x8a: break;
    case 0x8b: break;
    case 0x8c: break;
    case 0x8d: break;
    case 0x8e: break;
    case 0x8f: break;

    case 0x90: break;
    case 0x91: break;
    case 0x92: break;
    case 0x93: break;
    case 0x94: break;
    case 0x95: break;
    case 0x96: break;
    case 0x97: break;

    case 0x98: break;
    case 0x99: break;
    case 0x9a: break;
    case 0x9b: break;
    case 0x9c: break;
    case 0x9d: break;
    case 0x9e: break;
    case 0x9f: break;

    case 0xa0: break;
    case 0xa1: break;
    case 0xa2: break;
    case 0xa3: break;
    case 0xa4: break;
    case 0xa5: break;
    case 0xa6: break;
    case 0xa7: break;

    case 0xa8: break;
    case 0xa9: break;
    case 0xaa: break;
    case 0xab: break;
    case 0xac: break;
    case 0xad: break;
    case 0xae: break;
    case 0xaf: break;

    case 0xb0: break;
    case 0xb1: break;
    case 0xb2: break;
    case 0xb3: break;
    case 0xb4: break;
    case 0xb5: break;
    case 0xb6: break;
    case 0xb7: break;

    case 0xb8: break;
    case 0xb9: break;
    case 0xba: break;
    case 0xbb: break;
    case 0xbc: break;
    case 0xbd: break;
    case 0xbe: break;
    case 0xbf: break;

    case 0xc0: break;
    case 0xc1: break;
    case 0xc2: break;
    case 0xc3: break;
    case 0xc4: break;
    case 0xc5: break;
    case 0xc6: break;
    case 0xc7: break;

    case 0xc8: break;
    case 0xc9: break;
    case 0xca: break;
    case 0xcb: break;
    case 0xcc: break;
    case 0xcd: break;
    case 0xce: break;
    case 0xcf: break;

    case 0xd0: break;
    case 0xd1: break;
    case 0xd2: break;
    case 0xd3: break;
    case 0xd4: break;
    case 0xd5: break;
    case 0xd6: break;
    case 0xd7: break;

    case 0xd8: break;
    case 0xd9: break;
    case 0xda: break;
    case 0xdb: break;
    case 0xdc: break;
    case 0xdd: break;
    case 0xde: break;
    case 0xdf: break;

    case 0xe0: break;
    case 0xe1: break;
    case 0xe2: break;
    case 0xe3: break;
    case 0xe4: break;
    case 0xe5: break;
    case 0xe6: break;
    case 0xe7: break;

    case 0xe8: break;
    case 0xe9: break;
    case 0xea: break;
    case 0xeb: break;
    case 0xec: break;
    case 0xed: break;
    case 0xee: break;
    case 0xef: break;

    case 0xf0: break;
    case 0xf1: break;
    case 0xf2: break;
    case 0xf3: break;
    case 0xf4: break;
    case 0xf5: break;
    case 0xf6: break;
    case 0xf7: break;

    case 0xf8: break;
    case 0xf9: break;
    case 0xfa: break;
    case 0xfb: break;
    case 0xfc: break;
    case 0xfd: break;
    case 0xfe: break;
    case 0xff: break;
    }

    state->pc++;

    return 0;
}

unsigned char *initalise_memory(char *rom_filename) {
    FILE *f = fopen(rom_filename, "rb");
    if (!f) {
        printf("Could not open %s\n", rom_filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *memory = malloc(sizeof(unsigned char) * 16000);

    fread(memory, 1, fsize, f);
    fclose(f);

    return memory;
}

int main() {
    system_state state;
    state.memory = initalise_memory("rom/invaders");

    bool done = false;
    while (!done)
        done = emulate_op(&state);

    return 0;
}
