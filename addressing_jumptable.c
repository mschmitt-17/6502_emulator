#include "addressing_jumptable.h"
#include "6502.h"

static int implied_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_IMP) {
        return opcode;
    }
    return -1;
}

static int standard_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_IND_X ||
        addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_IMM ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_IND_Y ||
        addressing_mode == ADDR_MODE_ZPG_X ||
        addressing_mode == ADDR_MODE_ABS_Y ||
        addressing_mode == ADDR_MODE_ABS_X) {
        return opcode | (addressing_mode << 2);
    }
    return -1;
}

static int accum_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_ZPG_X ||
        addressing_mode == ADDR_MODE_ABS_X) {
        return opcode | (addressing_mode << 2);
    } else if (addressing_mode == ADDR_MODE_ACCUM_PARSE) {
        return opcode | (ADDR_MODE_ACCUM << 2);
    }
    return -1;
}

static int relative_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG) { // ZPG and relative addressing are syntactically the same
        return opcode;
    }
    return -1;
}

static int JSR_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ABS) {
        return OP_JSR;
    }
    return -1;
}

static int BIT_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS) {
        return OP_BIT | (addressing_mode << 2);
    }
    return -1;
}

static int JMP_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_IND) {
        return OP_JI;
    } else if (addressing_mode == ADDR_MODE_ABS) {
        return OP_JMP;
    }
    return -1;
}

static int STY_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_ZPG_X) {
        return opcode | (addressing_mode << 2);
    }
    return -1;
}

static int STA_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_IND_X ||
        addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_IND_Y ||
        addressing_mode == ADDR_MODE_ZPG_X ||
        addressing_mode == ADDR_MODE_ABS_Y ||
        addressing_mode == ADDR_MODE_ABS_X) {
        return opcode | (addressing_mode << 2);
    }
    return -1;
}

static int STX_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS) {
        return opcode | (addressing_mode << 2);
    } else if (addressing_mode == ADDR_MODE_ZPG_Y_PARSE) {
        return opcode | (ADDR_MODE_ZPG_Y << 2);
    }
    return -1;
}

static int LDY_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_ZPG_X) {
        return opcode | (addressing_mode << 2);
    } else if (addressing_mode == ADDR_MODE_IMM) {
        return OP_LDY;
    }
    return -1;
}

static int LDX_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS) {
        return opcode | (addressing_mode << 2);
    } else if (addressing_mode == ADDR_MODE_IMM) {
        return OP_LDX;
    } else if (addressing_mode == ADDR_MODE_ZPG_Y_PARSE) {
        return opcode | (ADDR_MODE_ZPG_Y << 2);
    }
    return -1;
}

static int CPX_CPY_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS) {
        return opcode | (addressing_mode << 2);
    } else if (addressing_mode == ADDR_MODE_IMM) {
        return opcode;
    }
    return -1;
}

static int DEC_INC_addressing(uint8_t opcode, uint8_t addressing_mode) {
    if (addressing_mode == ADDR_MODE_ZPG ||
        addressing_mode == ADDR_MODE_ABS ||
        addressing_mode == ADDR_MODE_ZPG_X ||
        addressing_mode == ADDR_MODE_ABS_X) {
        return opcode | (addressing_mode << 2);
    }
    return -1;
}

static int invalid_addressing(uint8_t opcode, uint8_t addressing_mode) {
    return -1;
}

int (* ptr_arr[0xFF])(uint8_t opcode, uint8_t addressing_mode) = {
    implied_addressing,     // OP_BRK
    standard_addressing,    // OP_ORA
    accum_addressing,       // OP_ASL
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_PHP
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BPL
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_CLC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    BIT_addressing,         // OP_BIT
    standard_addressing,    // OP_AND
    accum_addressing,       // OP_ROL
    invalid_addressing,     //
    JSR_addressing,         // OP_JSR_PARSE
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_PLP
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BMI
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_SEC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_RTI
    standard_addressing,    // OP_EOR
    accum_addressing,       // OP_LSR
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_PHA
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    JMP_addressing,         // OP_JMP
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BVC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_CLI
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_RTS
    standard_addressing,    // OP_ADC
    accum_addressing,       // OP_ROR
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_PLA
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BVS
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_SEI
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    STY_addressing,         // OP_STY
    STA_addressing,         // OP_STA
    STX_addressing,         // OP_STX
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_DEY
    invalid_addressing,     //
    implied_addressing,     // OP_TXA
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BCC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_TYA
    invalid_addressing,     //
    implied_addressing,     // OP_TXS
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    LDY_addressing,         // OP_LDY
    standard_addressing,    // OP_LDA
    LDX_addressing,         // OP_LDX
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_TAY
    invalid_addressing,     //
    implied_addressing,     // OP_TAX
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BCS
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_CLV
    invalid_addressing,     //
    implied_addressing,     // OP_TSX
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    CPX_CPY_addressing,     // OP_CPY
    standard_addressing,    // OP_CMP
    DEC_INC_addressing,     // OP_DEC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_INY
    invalid_addressing,     //
    implied_addressing,     // OP_DEX
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BNE
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_CLD
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    CPX_CPY_addressing,     // OP_CPX
    standard_addressing,    // OP_SBC
    DEC_INC_addressing,     // OP_INC
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_INX
    invalid_addressing,     //
    implied_addressing,     // OP_NOP
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    relative_addressing,    // OP_BEQ
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    implied_addressing,     // OP_SED
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing,     //
    invalid_addressing
};