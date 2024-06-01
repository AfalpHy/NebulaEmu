#pragma once

#include <cstdint>

namespace NebulaEmu {

enum NameTableMirroring { Horizontal, Vertical, SingleScreen, FourScreen };

class Mapper {
public:
    static Mapper* createMapper(unsigned num);

    virtual uint8_t readPRG(uint16_t addr) = 0;
    virtual uint8_t readCHR(uint16_t addr) = 0;

    virtual void wirtePRG(uint16_t addr, uint8_t data) = 0;
    virtual void wirteCHR(uint16_t addr, uint8_t data) = 0;

    uint8_t* getSRAMPtr(uint16_t addr);
    uint8_t readSRAM(uint16_t addr);
    void writeSRAM(uint16_t addr, uint8_t data);

    NameTableMirroring getNameTableMirroing();
};

class MapperNROM : public Mapper {
public:
    uint8_t readPRG(uint16_t addr);
    uint8_t readCHR(uint16_t addr);

    void wirtePRG(uint16_t addr, uint8_t data);
    void wirteCHR(uint16_t addr, uint8_t data);
};

}  // namespace NebulaEmu