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

void CMC(system_state *state) {
    state->cc.cy = !state->cc.cy;
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

void MOV(system_state *state, uint8_t reg1, uint8_t reg2) {
    unsigned char byte;
    if (reg2 == M) {
        uint16_t address = get_m_address(state, H, L);
        byte = state->memory[address];
    } else {
        byte = state->regs[reg2];
    }

    if (reg1 == M) {
        uint16_t address = get_m_address(state, H, L);
        state->memory[address] = byte;
    } else {
        state->regs[reg1] = byte;
    }
}

void STAX(system_state *state, uint8_t reg) {
    uint16_t address = get_m_address(state, reg, reg + 1);
    state->memory[address] = state->regs[A];
}

void LDAX(system_state *state, uint8_t reg) {
    uint16_t address = get_m_address(state, reg, reg + 1);
    state->regs[A] = state->memory[address];
}

// -- Register or memory to accumulator instructions --

void ADD(system_state *state, uint8_t reg) {
    uint8_t add1 = state->regs[A];
    uint8_t add2;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        add2 = state->memory[address];
    } else {
        add2 = state->regs[reg];
    }

    state->cc.ac = (((add1 & 0x0f) + (add2 & 0x0f)) & 0xf0) != 0;

    uint16_t res = add1 + add2;

    state->cc.cy = (res & 0x0f00) != 0;

    state->regs[A] = res & 0xff;

    set_zsp(state, state->regs[A]);
}

void ADC(system_state *state, uint8_t reg) {
    uint8_t add1 = state->regs[A];
    uint8_t add2;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        add2 = state->memory[address];
    } else {
        add2 = state->regs[reg];
    }

    state->cc.ac = (((add1 & 0x0f) + (add2 & 0x0f) + state->cc.cy) & 0xf0) != 0;

    uint16_t res = add1 + add2 + state->cc.cy;

    state->cc.cy = (res & 0x0f00) != 0;

    state->regs[A] = res & 0xff;

    set_zsp(state, state->regs[A]);
}

void SUB(system_state *state, uint8_t reg) {
    uint8_t sub1 = state->regs[A];
    uint8_t sub2;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        sub2 = ~state->memory[address];
    } else {
        sub2 = ~state->regs[reg];
    }

    state->cc.ac = (((sub1 & 0x0f) + (sub2 & 0x0f) + 1) & 0xf0) != 0;

    uint16_t res = sub1 + sub2 + 1;

    state->cc.cy = (res & 0x0f00) == 0;

    state->regs[A] = res & 0xff;

    set_zsp(state, state->regs[A]);
}

void SBB(system_state *state, uint8_t reg) {
    uint8_t sub1 = state->regs[A];
    uint8_t sub2;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        sub2 = ~(state->memory[address] + 1);
    } else {
        sub2 = ~(state->regs[reg] + 1);
    }

    state->cc.ac = (((sub1 & 0x0f) + (sub2 & 0x0f) + 1) & 0xf0) != 0;

    uint16_t res = sub1 + sub2 + 1;

    state->cc.cy = (res & 0x0f00) == 0;

    state->regs[A] = res & 0xff;

    set_zsp(state, state->regs[A]);
}

void ANA(system_state *state, uint8_t reg) {
    uint8_t and;
    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        and = state->memory[address];
    } else {
        and = state->regs[reg];
    }

    state->regs[A] = state->regs[A] & and;

    state->cc.cy = 0;
    set_zsp(state, state->regs[A]);
}

void XRA(system_state *state, uint8_t reg) {
    uint8_t xor;
    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        xor = state->memory[address];
    } else {
        xor = state->regs[reg];
    }

    state->regs[A] = state->regs[A] ^ xor;

    state->cc.cy = 0;
    set_zsp(state, state->regs[A]);
}

void ORA(system_state *state, uint8_t reg) {
    uint8_t or;
    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        or = state->memory[address];
    } else {
        or = state->regs[reg];
    }

    state->regs[A] = state->regs[A] | or;

    state->cc.cy = 0;
    set_zsp(state, state->regs[A]);
}

void CMP(system_state *state, uint8_t reg) {
    uint8_t cmp1 = state->regs[A];
    uint8_t cmp2;

    if (reg == M) {
        uint16_t address = get_m_address(state, H, L);
        cmp2 = ~state->memory[address];
    } else {
        cmp2 = ~state->regs[reg];
    }

    // might be wrong... data book didn't specify the behaviour
    state->cc.ac = (((state->regs[A] & 0x0f) + (cmp2 & 0x0f) + 1) & 0xf0) != 0;

    uint16_t res = cmp1 + cmp2 + 1;

    state->cc.cy = (res & 0x0f00) == 0;

    set_zsp(state, res & 0xff);
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

void PUSH(system_state *state, uint8_t reg) {
    uint8_t byte1;
    uint8_t byte2;
    if (reg == PSW) {
        byte1 = state->regs[A];
        byte2 = (state->cc.s << 7)
            | (state->cc.z << 6)
            | (state->cc.ac << 4)
            | (state->cc.p << 2)
            | (1 << 1)
            | (state->cc.cy);
    } else {
        byte1 = state->regs[reg];
        byte2 = state->regs[reg + 1];
    }

    state->memory[state->sp - 1] = byte1;
    state->memory[state->sp - 2] = byte2;

    state->sp -= 2;
}

void POP(system_state *state, uint8_t reg) {
    uint8_t byte1 = state->memory[state->sp + 1];
    uint8_t byte2 = state->memory[state->sp];

    if (reg == PSW) {
        //bytes might be reversed? it's right according to the manual, but
        //seems reversed compared to PUSH???
        state->cc.s = byte1 >> 7;
        state->cc.z = (byte1 >> 6) & 1;
        state->cc.ac = (byte1 >> 4) & 1;
        state->cc.p = (byte1 >> 2) & 1;
        state->cc.cy = byte1 & 1;

        state->regs[A] = byte2;
    } else {
        state->regs[reg] = byte1;
        state->regs[reg + 1] = byte2;
    }

    state->sp += 2;
}

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

    state->pc += 2;
}

void LDA(system_state *state) {
    uint16_t address = get_immediate_address(state);
    state->regs[A] = state->memory[address];

    state->pc += 2;
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

// -- Return from subroutine instructions --

void RET(system_state *state) {
    state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
    state->sp += 2;
}

void RC(system_state *state) {
    if (state->cc.cy)
        RET(state);
}

void RNC(system_state *state) {
    if (!state->cc.cy)
        RET(state);
}

void RZ(system_state *state) {
    if (state->cc.z)
        RET(state);
}

void RNZ(system_state *state) {
    if (!state->cc.z)
        RET(state);
}

void RM(system_state *state) {
    if (state->cc.s)
        RET(state);
}

void RP(system_state *state) {
    if (!state->cc.s)
        RET(state);
}

void RPE(system_state *state) {
    if (state->cc.p)
        RET(state);
}

void RPO(system_state *state) {
    if (!state->cc.p)
        RET(state);
}

// -- HLT --

void HLT(system_state *state) {
    exit(0); //TODO implement interrupt response
}

// -- The emulation nation --

int emulate_op(system_state *state) {
    unsigned char op_code = state->memory[state->pc];

    switch (op_code) {
    case 0x00: break; // NOP
    case 0x01: LXI(state, B);       break;
    case 0x02: STAX(state, B);      break;
    case 0x03: INX(state, B);       break;
    case 0x04: INR(state, B);       break;
    case 0x05: DCR(state, B);       break;
    case 0x06: MVI(state, B);       break;
    case 0x07: RLC(state);          break;

    case 0x08: exit(1); // undocumented instruction!! break;
    case 0x09: DAD(state, B);       break;
    case 0x0a: LDAX(state, B);      break;
    case 0x0b: DCX(state, B);       break;
    case 0x0c: INR(state, C);       break;
    case 0x0d: DCR(state, C);       break;
    case 0x0e: MVI(state, C);       break;
    case 0x0f: RRC(state);          break;

    case 0x10: exit(1); // undocumented instruction!! break;
    case 0x11: LXI(state, D);       break;
    case 0x12: STAX(state, D);      break;
    case 0x13: INX(state, D);       break;
    case 0x14: INR(state, D);       break;
    case 0x15: DCR(state, D);       break;
    case 0x16: MVI(state, D);       break;
    case 0x17: RAL(state);          break;

    case 0x18: exit(1); // undocumented instruction!! break;
    case 0x19: DAD(state, D);       break;
    case 0x1a: LDAX(state, D);      break;
    case 0x1b: DCX(state, D);       break;
    case 0x1c: INR(state, E);       break;
    case 0x1d: DCR(state, E);       break;
    case 0x1e: MVI(state, E);       break;
    case 0x1f: RAR(state);          break;

    case 0x20: exit(1); // undocumented instruction!! break;
    case 0x21: LXI(state, H);       break;
    case 0x22: SHLD(state);         break;
    case 0x23: INX(state, H);       break;
    case 0x24: INR(state, H);       break;
    case 0x25: DCR(state, H);       break;
    case 0x26: MVI(state, H);       break;
    case 0x27: DAA(state);          break;

    case 0x28: exit(1); // undocumented instruction!! break;
    case 0x29: DAD(state, H);       break;
    case 0x2a: LHLD(state);         break;
    case 0x2b: DCX(state, H);       break;
    case 0x2c: INR(state, L);       break;
    case 0x2d: DCR(state, L);       break;
    case 0x2e: MVI(state, L);       break;
    case 0x2f: CMA(state);          break;

    case 0x30: exit(1); // undocumented instruction!! break;
    case 0x31: LXI(state, SP);      break;
    case 0x32: STA(state);          break;
    case 0x33: INX(state, SP);      break;
    case 0x34: INR(state, M);       break;
    case 0x35: DCR(state, M);       break;
    case 0x36: MVI(state, M);       break;
    case 0x37: STC(state);          break;

    case 0x38: exit(1); // undocumented instruction!! break;
    case 0x39: DAD(state, SP);      break;
    case 0x3a: LDA(state);          break;
    case 0x3b: DCX(state, SP);      break;
    case 0x3c: INR(state, A);       break;
    case 0x3d: DCR(state, A);       break;
    case 0x3e: MVI(state, A);       break;
    case 0x3f: CMC(state);          break;

    case 0x40: MOV(state, B, B);    break;
    case 0x41: MOV(state, B, C);    break;
    case 0x42: MOV(state, B, D);    break;
    case 0x43: MOV(state, B, E);    break;
    case 0x44: MOV(state, B, H);    break;
    case 0x45: MOV(state, B, L);    break;
    case 0x46: MOV(state, B, M);    break;
    case 0x47: MOV(state, B, A);    break;

    case 0x48: MOV(state, C, B);    break;
    case 0x49: MOV(state, C, C);    break;
    case 0x4a: MOV(state, C, D);    break;
    case 0x4b: MOV(state, C, E);    break;
    case 0x4c: MOV(state, C, H);    break;
    case 0x4d: MOV(state, C, L);    break;
    case 0x4e: MOV(state, C, M);    break;
    case 0x4f: MOV(state, C, A);    break;

    case 0x50: MOV(state, D, B);    break;
    case 0x51: MOV(state, D, C);    break;
    case 0x52: MOV(state, D, D);    break;
    case 0x53: MOV(state, D, E);    break;
    case 0x54: MOV(state, D, H);    break;
    case 0x55: MOV(state, D, L);    break;
    case 0x56: MOV(state, D, M);    break;
    case 0x57: MOV(state, D, A);    break;

    case 0x58: MOV(state, E, B);    break;
    case 0x59: MOV(state, E, C);    break;
    case 0x5a: MOV(state, E, D);    break;
    case 0x5b: MOV(state, E, E);    break;
    case 0x5c: MOV(state, E, H);    break;
    case 0x5d: MOV(state, E, L);    break;
    case 0x5e: MOV(state, E, M);    break;
    case 0x5f: MOV(state, E, A);    break;

    case 0x60: MOV(state, H, B);    break;
    case 0x61: MOV(state, H, C);    break;
    case 0x62: MOV(state, H, D);    break;
    case 0x63: MOV(state, H, E);    break;
    case 0x64: MOV(state, H, H);    break;
    case 0x65: MOV(state, H, L);    break;
    case 0x66: MOV(state, H, M);    break;
    case 0x67: MOV(state, H, A);    break;

    case 0x68: MOV(state, L, B);    break;
    case 0x69: MOV(state, L, C);    break;
    case 0x6a: MOV(state, L, D);    break;
    case 0x6b: MOV(state, L, E);    break;
    case 0x6c: MOV(state, L, H);    break;
    case 0x6d: MOV(state, L, L);    break;
    case 0x6e: MOV(state, L, M);    break;
    case 0x6f: MOV(state, L, A);    break;

    case 0x70: MOV(state, M, B);    break;
    case 0x71: MOV(state, M, C);    break;
    case 0x72: MOV(state, M, D);    break;
    case 0x73: MOV(state, M, E);    break;
    case 0x74: MOV(state, M, H);    break;
    case 0x75: MOV(state, M, L);    break;
    case 0x76: HLT(state);          break;
    case 0x77: MOV(state, M, A);    break;

    case 0x78: MOV(state, A, B);    break;
    case 0x79: MOV(state, A, C);    break;
    case 0x7a: MOV(state, A, D);    break;
    case 0x7b: MOV(state, A, E);    break;
    case 0x7c: MOV(state, A, H);    break;
    case 0x7d: MOV(state, A, L);    break;
    case 0x7e: MOV(state, A, M);    break;
    case 0x7f: MOV(state, A, A);    break;

    case 0x80: ADD(state, B);       break;
    case 0x81: ADD(state, C);       break;
    case 0x82: ADD(state, D);       break;
    case 0x83: ADD(state, E);       break;
    case 0x84: ADD(state, H);       break;
    case 0x85: ADD(state, L);       break;
    case 0x86: ADD(state, M);       break;
    case 0x87: ADD(state, A);       break;

    case 0x88: ADC(state, B);       break;
    case 0x89: ADC(state, C);       break;
    case 0x8a: ADC(state, D);       break;
    case 0x8b: ADC(state, E);       break;
    case 0x8c: ADC(state, H);       break;
    case 0x8d: ADC(state, L);       break;
    case 0x8e: ADC(state, M);       break;
    case 0x8f: ADC(state, A);       break;

    case 0x90: SUB(state, B);       break;
    case 0x91: SUB(state, C);       break;
    case 0x92: SUB(state, D);       break;
    case 0x93: SUB(state, E);       break;
    case 0x94: SUB(state, H);       break;
    case 0x95: SUB(state, L);       break;
    case 0x96: SUB(state, M);       break;
    case 0x97: SUB(state, A);       break;

    case 0x98: SBB(state, B);       break;
    case 0x99: SBB(state, C);       break;
    case 0x9a: SBB(state, D);       break;
    case 0x9b: SBB(state, E);       break;
    case 0x9c: SBB(state, H);       break;
    case 0x9d: SBB(state, L);       break;
    case 0x9e: SBB(state, M);       break;
    case 0x9f: SBB(state, A);       break;

    case 0xa0: ANA(state, B);       break;
    case 0xa1: ANA(state, C);       break;
    case 0xa2: ANA(state, D);       break;
    case 0xa3: ANA(state, E);       break;
    case 0xa4: ANA(state, H);       break;
    case 0xa5: ANA(state, L);       break;
    case 0xa6: ANA(state, M);       break;
    case 0xa7: ANA(state, A);       break;

    case 0xa8: XRA(state, B);       break;
    case 0xa9: XRA(state, C);       break;
    case 0xaa: XRA(state, D);       break;
    case 0xab: XRA(state, E);       break;
    case 0xac: XRA(state, H);       break;
    case 0xad: XRA(state, L);       break;
    case 0xae: XRA(state, M);       break;
    case 0xaf: XRA(state, A);       break;

    case 0xb0: ORA(state, B);       break;
    case 0xb1: ORA(state, C);       break;
    case 0xb2: ORA(state, D);       break;
    case 0xb3: ORA(state, E);       break;
    case 0xb4: ORA(state, H);       break;
    case 0xb5: ORA(state, L);       break;
    case 0xb6: ORA(state, M);       break;
    case 0xb7: ORA(state, A);       break;

    case 0xb8: CMP(state, B);       break;
    case 0xb9: CMP(state, C);       break;
    case 0xba: CMP(state, D);       break;
    case 0xbb: CMP(state, E);       break;
    case 0xbc: CMP(state, H);       break;
    case 0xbd: CMP(state, L);       break;
    case 0xbe: CMP(state, M);       break;
    case 0xbf: CMP(state, A);       break;

    case 0xc0: RNZ(state);          break;
    case 0xc1: POP(state, B);       break;
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
