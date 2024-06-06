#pragma once

#include <string>
#include <vector>

#include "Mapper.h"

namespace NebulaEmu {

#define DeclareFriend(Mapper) friend class Mapper

class Cartridge {
public:
    void load(std::string path);

    Mapper* getMapper() { return _mapper; }

    DeclareFriend(Mapper);
    DeclareFriend(MapperNROM);

private:
    NameTableMirroring _mirroring;
    uint8_t* _battery_backed_RAM = nullptr;
    uint8_t* _trainer = nullptr;
    std::vector<uint8_t> _PRG_ROM;  // 16384 * x bytes
    std::vector<uint8_t> _CHR_ROM;  // 8192 * y bytes
    Mapper* _mapper = nullptr;
};

}  // namespace NebulaEmu