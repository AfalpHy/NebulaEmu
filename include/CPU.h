#pragma once

#include <cstdint>

namespace NebulaEmu {

enum InterruptType {
    NMI_I,
    IRQ_I,
    BRK_I  // triggered by software
};

class CPU {
public:
    void reset();

    void step();

private:
    uint8_t readByte(uint16_t addr);

    uint16_t readWord(uint16_t addr);

    void write(uint16_t addr, uint8_t data);

    void pushStack(uint8_t data);

    uint8_t popStack();

    void setZN(uint8_t result);

    void addSkipCyclesIfPageCrossed(uint16_t cur, uint16_t next);

    void executeInterrupt(InterruptType type);

    bool executeImplied(uint8_t opcode);
    bool executeBranch(uint8_t opcode);
    bool executeCommon(uint8_t opcode);

    uint16_t _PC;
    uint8_t _SP;
    uint8_t _A;
    uint8_t _X;
    uint8_t _Y;
    union {
        struct {
            uint8_t C : 1;  // carry
            uint8_t Z : 1;  // zero
            uint8_t I : 1;  // interrupt disable
            uint8_t D : 1;  // decimal mode
            uint8_t B : 1;  // break comand
            uint8_t U : 1;  // unused always pushed as 1
            uint8_t V : 1;  // overflow
            uint8_t N : 1;  // negative
        } bits;
        uint8_t value;
    } _P;

    uint8_t _RAM[0x800];

    bool _NMI_pin;
    bool _IRQ_pin;

    uint64_t _cycles = 0;
    uint64_t _skipCycles = 0;
};

}  // namespace NebulaEmu