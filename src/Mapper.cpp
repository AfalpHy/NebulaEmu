#include <assert.h>

#include <iostream>

#include "Cartridge.h"
namespace NebulaEmu {

extern Cartridge* cartridge;

Mapper* Mapper::createMapper(uint32_t num) {
    switch (num) {
        case 0:
            return new MapperNROM();
        default:
            return nullptr;
    }
}

uint8_t MapperNROM::readPRG(uint16_t addr) {
    // CPU $8000-$BFFF: First 16 KB of ROM.
    // CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
    if (cartridge->_PRG_ROM.size() > 0x4000) {  // NROM-256
        return cartridge->_PRG_ROM[addr - 0x8000];
    } else {  // NROM-128
        return cartridge->_PRG_ROM[(addr - 0x8000) & 0x3fff];
    }
}

uint8_t MapperNROM::readCHR(uint16_t addr) { return cartridge->_CHR_ROM[addr]; }

void MapperNROM::wirtePRG(uint16_t addr, uint8_t data) {
    (void)data;
    std::cerr << "write only-read memory at " << addr << std::endl;
    exit(1);
}

void MapperNROM::wirteCHR(uint16_t addr, uint8_t data) {
    (void)data;
    std::cerr << "write only-read memory at " << addr << std::endl;
    exit(1);
}

uint8_t* Mapper::getSRAMPtr(uint16_t addr) {
    assert(cartridge->_battery_backed_RAM && "access not exist memory");
    return &cartridge->_battery_backed_RAM[addr - 0x6000];
}

uint8_t Mapper::readSRAM(uint16_t addr) {
    assert(cartridge->_battery_backed_RAM && "access not exist memory");
    return cartridge->_battery_backed_RAM[addr - 0x6000];
}

void Mapper::writeSRAM(uint16_t addr, uint8_t data) {
    assert(cartridge->_battery_backed_RAM && "access not exist memory");
    cartridge->_battery_backed_RAM[addr - 0x6000] = data;
}

NameTableMirroring Mapper::getNameTableMirroing() { return cartridge->_mirroring; }

}  // namespace NebulaEmu
