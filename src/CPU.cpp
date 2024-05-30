#include "CPU.h"

#include <iostream>

#include "Cartridge.h"
#include "Mapper.h"

namespace NebulaEmu {

extern Cartridge* cartridge;

// clang-format off
static const int operationCycles[0x100] = {
        7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
        6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
        6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
        6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
        0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
        2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
        2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
        2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
        2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
        2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};
// clang-format on

enum ImpliedInstruction {
    BRK = 0x00,
    JMP = 0x4C,
    JMPI = 0x6C,
    JSR = 0X20,
    RTI = 0x40,
    RTS = 0x60,

    PHP = 0x08,
    PLP = 0x28,
    PHA = 0x48,
    PLA = 0x68,
    DEY = 0x88,
    TAY = 0xA8,
    INY = 0xC8,
    INX = 0xE8,

    CLC = 0X18,
    SEC = 0x38,
    CLI = 0x58,
    SEI = 0x78,
    TYA = 0x98,
    CLV = 0xB8,
    CLD = 0xD8,
    SED = 0xF8,

    TXA = 0x8A,
    TXS = 0x9A,
    TAX = 0xAA,

    TSX = 0xBA,
    DEX = 0xCA,
    NOP = 0xEA
};

enum BranchInstruction {
    BPL = 0x10,
    BMI = 0x30,
    BVC = 0x50,
    BVS = 0x70,
    BCC = 0x90,
    BCS = 0xB0,
    BNE = 0xD0,
    BEQ = 0xF0
};

// opcode & 11100011b
enum CommonInstruction {
    BIT = 1 << 5,
    STY = 4 << 5,
    LDY = 5 << 5,
    CPY = 6 << 5,
    CPX = 7 << 5,

    ORA = 0 << 5 | 1,
    AND = 1 << 5 | 1,
    EOR = 2 << 5 | 1,
    ADC = 3 << 5 | 1,
    STA = 4 << 5 | 1,
    LDA = 5 << 5 | 1,
    CMP = 6 << 5 | 1,
    SBC = 7 << 5 | 1,

    ASL = 0 << 5 | 2,
    ROL = 1 << 5 | 2,
    LSR = 2 << 5 | 2,
    ROR = 3 << 5 | 2,
    STX = 4 << 5 | 2,
    LDX = 5 << 5 | 2,
    DEC = 6 << 5 | 2,
    INC = 7 << 5 | 2
};

void CPU::reset() {
    _A = _X = _Y = 0;
    _SP = 0XFD;
    _P.value = 0X34;
    _PC = readWord((0xFFFC));
    _IRQ_pin = _NMI_pin = false;
    return;
}

void CPU::step() {
    _cycles++;

    if (_skipCycles > 0) {
        _skipCycles--;
        return;
    }

    if (_NMI_pin) {
        _NMI_pin = _IRQ_pin = false;
        executeInterrupt(NMI_I);
        // interrupt spend 7 cycles
        _skipCycles += 6;
        return;
    } else if (_IRQ_pin) {
        _IRQ_pin = false;
        executeInterrupt(IRQ_I);
        // interrupt spend 7 cycles
        _skipCycles += 6;
        return;
    }

    uint8_t opcode = readByte(_PC++);
    int cycleLength = operationCycles[opcode];

    if (cycleLength && (executeImpliedInstruct(opcode) || executeBranchInstruct(opcode)) ||
        executeCommonInstruct(opcode)) {
    } else {
        std::cerr << "unkown instruct" << std::endl;
        exit(1);
    }

    _skipCycles += cycleLength - 1;
}

uint8_t CPU::readByte(uint16_t addr) {
    if (addr < 0x2000) {
        return _RAM[addr & 0x7ff];
    } else if (addr < 0x4000) {
        /// TODO:
    } else if (addr < 0x4020) {
        /// TODO:
    } else if (addr < 0x6000) {
        /// TODO:
    } else if (addr < 0x8000) {
        /// TODO:
    } else {
        return cartridge->getMapper()->readPRGROM(addr);
    }
    return 0;
}

uint16_t CPU::readWord(uint16_t addr) { return (readByte(addr + 1) << 8) | readByte(addr); }

void CPU::write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000) {
        _RAM[addr & 0x7ff] = data;
    } else if (addr < 0x4000) {
        /// TODO:
    } else if (addr < 0x4020) {
        /// TODO:
    } else if (addr < 0x6000) {
        /// TODO:
    } else if (addr < 0x8000) {
        /// TODO:
    } else {
        /// TODO:
    }
}

void CPU::pushStack(uint8_t data) { _RAM[--_SP | 0x100] = data; }

uint8_t CPU::popStack() { return _RAM[_SP++ | 0x100]; }

void CPU::setZN(uint8_t result) {
    _P.bits.Z = !result;
    _P.bits.N = result >> 7;
}

void CPU::executeInterrupt(InterruptType type) {
    if (_P.bits.I && type == IRQ_I) {
        return;
    }
    if (type == BRK_I) {
        _P.bits.B = true;
        _PC++;
    }

    pushStack(_PC >> 8);
    pushStack(_PC & 0xFF);
    pushStack(_P.value);
    _P.bits.I = true;
    if (type == NMI_I) {
        _PC = readWord(0xFFFA);
    } else {
        _PC = readWord(0xFFFE);
    }
}

bool CPU::executeImpliedInstruct(uint8_t opcode) {
    switch (opcode) {
        case BRK:
            executeInterrupt(BRK_I);
            break;
        case JMP:
            _PC = readWord(_PC);
            break;
        case JMPI: {
            uint16_t location = readWord(_PC);
            // bug in 6502, it's strange
            _PC = readByte(location) | readByte((location & 0xff00) | (location + 1) & 0xff) << 8;
            break;
        }
        case JSR:
            pushStack((_PC + 1) >> 8);
            pushStack(_PC + 1);
            _PC = readWord(_PC);
            break;
        case RTI:
            _P.value = popStack();
            _PC = popStack();
            _PC |= popStack() << 8;
            break;
        case RTS:
            _PC = popStack();
            _PC |= popStack() << 8;
            _PC += 1;
            break;
        case PHP:
            pushStack(_P.value);
            break;
        case PLP:
            _P.value = popStack();
            break;
        case PHA:
            pushStack(_A);
            break;
        case PLA:
            _A = popStack();
            setZN(_A);
            break;
        case DEY:
            _Y -= 1;
            setZN(_Y);
            break;
        case TAY:
            _Y = _A;
            setZN(_Y);
            break;
        case INY:
            _Y += 1;
            setZN(_Y);
            break;
        case INX:
            _X += 1;
            setZN(_X);
            break;
        case CLC:
            _P.bits.C = 0;
            break;
        case SEC:
            _P.bits.C = 1;
            break;
        case CLI:
            _P.bits.I = 0;
            break;
        case SEI:
            _P.bits.I = 1;
            break;
        case TYA:
            _A = _Y;
            setZN(_A);
            break;
        case CLV:
            _P.bits.V = 0;
            break;
        case CLD:
            _P.bits.D = 0;
            break;
        case SED:
            _P.bits.D = 1;
            break;
        case TXA:
            _A = _X;
            setZN(_A);
            break;
        case TXS:
            _SP = _X;
            break;
        case TAX:
            _X = _A;
            setZN(_X);
            break;
        case TSX:
            _X = _SP;
            setZN(_X);
            break;
        case DEX:
            _X -= 1;
            setZN(_X);
            break;
        case NOP:
            break;
        default:
            return false;
    }
    return true;
}

bool CPU::executeBranchInstruct(uint8_t opcode) {
    bool br;
    switch (opcode) {
        case BPL:
            br = !_P.bits.N;
            break;
        case BMI:
            br = _P.bits.N;
            break;
        case BVC:
            br = !_P.bits.V;
            break;
        case BVS:
            br = _P.bits.V;
            break;
        case BCC:
            br = !_P.bits.C;
            break;
        case BCS:
            br = _P.bits.C;
            break;
        case BNE:
            br = !_P.bits.Z;
            break;
        case BEQ:
            br = _P.bits.Z;
            break;
        default:
            return false;
    }
    if (br) {
        int8_t offset = readByte(_PC++);
        _skipCycles += 1;
        if ((_PC & 0xFF00) != ((_PC + offset) & 0xFF)) {
            _skipCycles += 1;
        }
        // uint16_t and int8_t will be promoted to int
        _PC = _PC + offset;
    } else {
        _PC++;
    }
    return true;
}

bool CPU::executeCommonInstruct(uint8_t opcode) {
    uint8_t addressMode = opcode & 0x1f;
    opcode = opcode & 0x63;
    uint16_t location = 0;
    switch (addressMode) {
        // indexedIndirectX
        case 0 << 0 | 1: {
            uint8_t zeroAddr = readByte(_PC) + _X;
            location = readByte(zeroAddr) | readByte(zeroAddr + 1) << 8;
            break;
        }
        // zero page
        case 1 << 2 | 0:
        case 1 << 2 | 1:
        case 1 << 2 | 2:
            location = readByte(_PC++);
            break;
        // immediate
        case 0 << 2 | 0:
        case 2 << 2 | 1:
        case 0 << 2 | 2:
            location = _PC++;
            break;
        // absolute
        case 3 << 2 | 0:
        case 3 << 2 | 1:
        case 3 << 2 | 2:
            location = readWord(_PC);
            _PC += 2;
        // indexed
        case 5 << 2 | 0:
        case 5 << 2 | 1:
        case 5 << 2 | 2:
            if (opcode == LDX || opcode == STX) {
                location = (readByte(_PC++) + _Y) & 0xff;
            } else {
                location = (readByte(_PC++) + _X) & 0xff;
            }
            break;
        default:
            break;
    }
    return false;
}
}  // namespace NebulaEmu