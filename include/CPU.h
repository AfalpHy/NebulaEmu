#pragma once

#include <cstdint>

namespace NebulaEmu {
class CPU {
public:
    void powerUp();

    uint8_t read(uint16_t addr);

    void write(uint16_t addr, uint8_t data);

private:
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
};

}  // namespace NebulaEmu