#include "CPU.h"

#include "Cartridge.h"
#include "Mapper.h"
namespace NebulaEmu {

extern Cartridge* cartridge;

void CPU::powerUp() {
    _A = _X = _Y = 0;
    _SP = 0XFD;
    _P.value = 0X34;
    return;
}

uint8_t CPU::read(uint16_t addr) {
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
}  // namespace NebulaEmu