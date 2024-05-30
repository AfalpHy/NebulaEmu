#pragma once

#include <cstdint>

namespace NebulaEmu {

enum MapperType { NROM };

class Mapper {
public:
    static Mapper* createMapper(unsigned num);

    virtual uint8_t readPRGROM(uint16_t addr) = 0;
};

class MapperNROM : public Mapper {
public:
    uint8_t readPRGROM(uint16_t addr);
};

}  // namespace NebulaEmu