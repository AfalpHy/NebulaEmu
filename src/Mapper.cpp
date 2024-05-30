#include "Cartridge.h"

namespace NebulaEmu {

extern Cartridge* cartridge;

Mapper* Mapper::createMapper(unsigned num) {
    switch (num) {
        case 0:
            return new MapperNROM();
        default:
            return nullptr;
    }
}
uint8_t MapperNROM ::readPRGROM(uint16_t addr) { return cartridge->_PRG_ROM[addr - 0x8000]; }
}  // namespace NebulaEmu
