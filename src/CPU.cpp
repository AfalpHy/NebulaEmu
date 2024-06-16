#include "CPU.h"

#include <iostream>

#include "APU.h"
#include "Cartridge.h"
#include "Controller.h"
#include "Mapper.h"
#include "PPU.h"
namespace NebulaEmu {

extern Cartridge* cartridge;
extern APU* apu;
extern PPU* ppu;
extern Controller* controller;

// clang-format off
static const uint32_t operationCycles[0x100] = {
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
    _P.value = 0X24;
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
        // interrupt spend 7 cycles(include this cycle)
        _skipCycles += 6;
        return;
    } else if (_IRQ_pin) {
        _IRQ_pin = false;
        executeInterrupt(IRQ_I);
        // interrupt spend 7 cycles(include this cycle)
        _skipCycles += 6;
        return;
    }

    uint8_t opcode = readByte(_PC++);
    uint32_t cycleLength = operationCycles[opcode];

    if (cycleLength && (executeImplied(opcode) || executeBranch(opcode) || executeCommon(opcode))) {
        _skipCycles += cycleLength - 1;
    } else {
        std::cerr << "unkown instruction" << std::endl;
        exit(1);
    }
}

uint8_t* CPU::getPagePtr(uint16_t addr) {
    addr <<= 8;
    if (addr < 0x2000) {
        return &_RAM[addr & 0x7ff];
    } else if (addr < 0x4020) {
        std::cerr << "DMA request should not reach here" << std::endl;
        exit(1);
    } else if (addr < 0x6000) {
        std::cerr << "unsupported addr" << std::endl;
        exit(2);
    } else if (addr < 0x8000) {
        return cartridge->getMapper()->getSRAMPtr(addr);
    } else {
        std::cerr << "DMA request should not reach here" << std::endl;
        exit(1);
    }
}

uint8_t CPU::readByte(uint16_t addr) {
    if (addr < 0x2000) {
        return _RAM[addr & 0x7ff];
    } else if (addr < 0x4000) {
        addr &= 0x2007;
        switch (addr) {
            case 0x2002:
                return ppu->readPPUSTATUS();
            case 0x2004:
                return ppu->readOAMDATA();
            case 0x2007:
                return ppu->readPPUDATA();
            default:
                std::cerr << "read from unmapped addr" << std::endl;
                exit(1);
        }
    } else if (addr < 0x4020) {
        if (addr == 0x4016) {
            return controller->readJoyStick1Data();
        } else if (addr == 0x4017) {
            return controller->readJoyStick2Data();
        } else if (addr == 0x4015) {
            return apu->readStatus();
        } else {
            std::cerr << "read from unmapped addr" << std::endl;
            exit(1);
        }
    } else if (addr < 0x6000) {
        std::cerr << "read unsupported addr" << std::endl;
        exit(2);
    } else if (addr < 0x8000) {
        return cartridge->getMapper()->readSRAM(addr);
    } else {
        return cartridge->getMapper()->readPRG(addr);
    }
    return 0;
}

uint16_t CPU::readWord(uint16_t addr) { return (readByte(addr + 1) << 8) | readByte(addr); }

void CPU::write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000) {
        _RAM[addr & 0x7ff] = data;
    } else if (addr < 0x4000) {
        addr &= 0x2007;
        switch (addr) {
            case 0x2000:
                ppu->writePPUCTRL(data);
                break;
            case 0x2001:
                ppu->writePPUCMASK(data);
                break;
            case 0x2003:
                ppu->writeOAMADDR(data);
                break;
            case 0x2004:
                ppu->writeOAMDATA(data);
                break;
            case 0x2005:
                ppu->writePPUSCROLL(data);
                break;
            case 0x2006:
                ppu->writePPUADDR(data);
                break;
            case 0x2007:
                ppu->writePPUDATA(data);
                break;
            default:
                std::cerr << "write to ummapped addr" << std::endl;
                exit(1);
        }
    } else if (addr < 0x4020) {
        switch (addr) {
            case 0x4000:
                apu->writePulseReg0(true, data);
                break;
            case 0x4001:
                apu->writePulseReg1(true, data);
                break;
            case 0x4002:
                apu->writePulseReg2(true, data);
                break;
            case 0x4003:
                apu->writePulseReg3(true, data);
                break;
            case 0x4004:
                apu->writePulseReg0(false, data);
                break;
            case 0x4005:
                apu->writePulseReg1(false, data);
                break;
            case 0x4006:
                apu->writePulseReg2(false, data);
                break;
            case 0x4007:
                apu->writePulseReg3(false, data);
                break;
            case 0x4008:
                apu->writeTriangleReg0(data);
                break;
            case 0x4009:
                // Unused
                break;
            case 0x400A:
                apu->writeTriangleReg2(data);
                break;
            case 0x400B:
                apu->writeTriangleReg3(data);
                break;
            case 0x400C:
                apu->writeNoiseReg0(data);
                break;
            case 0x400D:
                // Unused
                break;
            case 0x400E:
                apu->writeNoiseReg2(data);
                break;
            case 0x400F:
                apu->writeNoiseReg3(data);
                break;
            case 0x4010:
                apu->writeDMCReg0(data);
                break;
            case 0x4011:
                apu->writeDMCReg1(data);
                break;
            case 0x4012:
                apu->writeDMCReg2(data);
                break;
            case 0x4013:
                apu->writeDMCReg3(data);
                break;
            case 0x4014:
                _skipCycles += 513;
                _skipCycles += _cycles & 1;
                ppu->OAMDMA(getPagePtr(data));
                break;
            case 0x4015:
                apu->writeStatus(data);
                break;
            case 0x4016:
                controller->strobe(data);
                break;
            case 0x4017:
                apu->writeFrameCounter(data);
                break;
            default:
                std::cerr << "write to ummapped addr" << std::endl;
                exit(1);
        }
    } else if (addr < 0x6000) {
        std::cerr << "write to unsupported addr" << std::endl;
        exit(2);
    } else if (addr < 0x8000) {
        cartridge->getMapper()->writeSRAM(addr, data);
    } else {
        cartridge->getMapper()->wirtePRG(addr, data);
    }
}

void CPU::pushStack(uint8_t data) { _RAM[--_SP | 0x100] = data; }

uint8_t CPU::popStack() { return _RAM[_SP++ | 0x100]; }

void CPU::setZN(uint8_t result) {
    _P.bits.Z = !result;
    _P.bits.N = result >> 7;
}

void CPU::addSkipCyclesIfPageCrossed(uint16_t cur, uint16_t next) {
    if ((cur & 0xFF00) != (next & 0xFF00)) {
        _skipCycles += 1;
    }
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

bool CPU::executeImplied(uint8_t opcode) {
    switch (opcode) {
        case BRK:
            executeInterrupt(BRK_I);
            break;
        case JMP:
            _PC = readWord(_PC);
            break;
        case JMPI: {
            uint16_t location = readWord(_PC);
            // An original 6502 has does not correctly fetch the target address if the indirect vector falls on a page
            // boundary (e.g. $xxFF where xx is any value from $00 to $FF). In this case fetches the LSB from $xxFF as
            // expected but takes the MSB from $xx00.
            _PC = readByte(location) | readByte((location & 0xff00) | ((location + 1) & 0xff)) << 8;
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

bool CPU::executeBranch(uint8_t opcode) {
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
        addSkipCyclesIfPageCrossed(_PC, _PC + offset);
        // uint16_t and int8_t will be promoted to int
        _PC = _PC + offset;
    } else {
        _PC++;
    }
    return true;
}

bool CPU::executeCommon(uint8_t opcode) {
    uint8_t addressMode = opcode & 0x1f;
    opcode = opcode & 0xe3;
    uint16_t location = 0;
    switch (addressMode) {
        // indexedIndirect
        case 0 << 0 | 1: {
            uint8_t zeroAddr = readByte(_PC++) + _X;
            location = readByte(zeroAddr) | readByte(zeroAddr + 1) << 8;
            break;
        }
        // immediate
        case 0 << 2 | 0:
        case 2 << 2 | 1:
        case 0 << 2 | 2:
            location = _PC++;
            break;
        // zero page
        case 1 << 2 | 0:
        case 1 << 2 | 1:
        case 1 << 2 | 2:
            location = readByte(_PC++);
            break;
        // absolute
        case 3 << 2 | 0:
        case 3 << 2 | 1:
        case 3 << 2 | 2:
            location = readWord(_PC);
            _PC += 2;
            break;
        // indirect indexed
        case 4 << 2 | 1: {
            uint8_t zeroAddr = readByte(_PC++);
            location = readByte(zeroAddr) | readByte(zeroAddr + 1) << 8;
            if (opcode != STA) {
                addSkipCyclesIfPageCrossed(location, location + _Y);
            }
            location += _Y;
            break;
        }
        // indexed zero page
        case 5 << 2 | 0:
        case 5 << 2 | 1:
        case 5 << 2 | 2:
            if (opcode == LDX || opcode == STX) {
                location = (readByte(_PC++) + _Y) & 0xff;
            } else {
                location = (readByte(_PC++) + _X) & 0xff;
            }
            break;
        // absolute Y
        case 6 << 2 | 1:
            location = readWord(_PC);
            _PC += 2;
            if (opcode != STA) {
                addSkipCyclesIfPageCrossed(location, location + _Y);
            }
            location += _Y;
            break;
        // absolute X
        case 7 << 2 | 0:
        case 7 << 2 | 1:
            location = readWord(_PC);
            _PC += 2;
            if (opcode != STA) {
                addSkipCyclesIfPageCrossed(location, location + _X);
            }
            location += _X;
            break;
        // absolute X/Y
        case 7 << 2 | 2: {
            location = readWord(_PC);
            _PC += 2;
            uint8_t index;
            if (opcode == LDX) {
                index = _Y;
            } else {
                index = _X;
            }
            addSkipCyclesIfPageCrossed(location, location + index);
            location += index;
            break;
        }
        default:
            break;
    }
    uint16_t operand = 0;
    switch (opcode) {
        case BIT:
            operand = readByte(location);
            _P.bits.Z = !(_A & operand);
            _P.bits.V = operand & 0x40;
            _P.bits.N = operand & 0x80;
            break;
        case STY:
            write(location, _Y);
            break;
        case LDY:
            _Y = readByte(location);
            setZN(_Y);
            break;
        case CPY:
            operand = _Y - readByte(location);
            _P.bits.C = !(operand & 0x100);
            setZN(operand);
            break;
        case CPX:
            operand = _X - readByte(location);
            _P.bits.C = !(operand & 0x100);
            setZN(operand);
            break;
        case ORA:
            _A |= readByte(location);
            setZN(_A);
            break;
        case AND:
            _A &= readByte(location);
            setZN(_A);
            break;
        case EOR:
            _A ^= readByte(location);
            setZN(_A);
            break;
        case ADC: {
            operand = readByte(location);
            uint16_t sum = _A + operand + _P.bits.C;
            _P.bits.C = sum & 0x100;
            _P.bits.V = (_A ^ sum) & (operand ^ sum) & 0x80;
            _A = sum;
            setZN(_A);
        } break;
        case STA:
            write(location, _A);
            break;
        case LDA:
            _A = readByte(location);
            setZN(_A);
            break;
        case CMP:
            operand = _A - readByte(location);
            _P.bits.C = !(operand & 0x100);
            setZN(operand);
            break;
        case SBC: {
            operand = readByte(location);
            uint16_t sum = _A - operand - !_P.bits.C;
            _P.bits.C = !(sum & 0x100);
            _P.bits.V = (_A ^ sum) & (~operand ^ sum) & 0x80;
            _A = sum;
            setZN(_A);
        } break;
        case ASL:
            if (addressMode == (2 << 2 | 2)) {
                _P.bits.C = _A & 0x80;
                _A = _A << 1;
                setZN(_A);
            } else {
                operand = readByte(location);
                _P.bits.C = operand & 0x80;
                operand = operand << 1;
                write(location, operand);
                setZN(operand);
            }
            break;
        case ROL: {
            auto tmp = _P.bits.C;
            if (addressMode == (2 << 2 | 2)) {
                _P.bits.C = _A & 0x80;
                _A = (_A << 1) | tmp;
                setZN(_A);
            } else {
                operand = readByte(location);
                _P.bits.C = operand & 0x80;
                operand = (operand << 1) | tmp;
                write(location, operand);
                setZN(operand);
            }
        } break;
        case LSR:
            if (addressMode == (2 << 2 | 2)) {
                _P.bits.C = _A & 1;
                _A = _A >> 1;
                setZN(_A);
            } else {
                operand = readByte(location);
                _P.bits.C = operand & 1;
                operand = operand >> 1;
                write(location, operand);
                setZN(operand);
            }
            break;
        case ROR: {
            auto tmp = _P.bits.C;
            if (addressMode == (2 << 2 | 2)) {
                _P.bits.C = _A & 1;
                _A = (_A >> 1) | (tmp << 7);
                setZN(_A);
            } else {
                operand = readByte(location);
                _P.bits.C = operand & 1;
                operand = (operand >> 1) | (tmp << 7);
                write(location, operand);
                setZN(operand);
            }
        } break;
        case STX:
            write(location, _X);
            break;
        case LDX:
            _X = readByte(location);
            setZN(_X);
            break;
        case DEC:
            operand = readByte(location) - 1;
            write(location, operand);
            setZN(operand);
            break;
        case INC:
            operand = readByte(location) + 1;
            write(location, operand);
            setZN(operand);
            break;
        default:
            return false;
    }
    return true;
}
}  // namespace NebulaEmu