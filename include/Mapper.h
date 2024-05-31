#pragma once

#include <cstdint>

namespace NebulaEmu {

enum MapperType { NROM };

class Mapper {
public:
    static Mapper* createMapper(unsigned num);

    virtual uint8_t readPRGROM(uint16_t addr) = 0;
    virtual uint8_t readCHRROM(uint16_t addr) = 0;

    virtual void wirtePRGROM(uint16_t addr, uint8_t data) = 0;
    virtual void wirteCHRROM(uint16_t addr, uint8_t data) = 0;

    uint8_t readSRAM(uint16_t addr);
    void writeSRAM(uint16_t addr, uint8_t data);
};

class MapperNROM : public Mapper {
public:
    uint8_t readPRGROM(uint16_t addr);
    uint8_t readCHRROM(uint16_t addr);

    void wirtePRGROM(uint16_t addr, uint8_t data);
    void wirteCHRROM(uint16_t addr, uint8_t data);
};

}  // namespace NebulaEmu