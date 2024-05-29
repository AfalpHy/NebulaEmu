#pragma once
#include <string>
#include <vector>

namespace NebulaEmu {

typedef unsigned char Byte;

class Cartridge {
public:
    void load(std::string path);

    enum Mirroring { Horizontal, Vertical, SingleScreen, FourScreen };

private:
    Mirroring _mirroring;
    Byte* _battery_backed_RAM = nullptr;
    // Byte* _trainer = nullptr;
    Byte* _PRG_ROM;  // 16384 * x bytes
    Byte* _CHR_ROM;  // 8192 * x bytes
    unsigned _mapperNum;
};

}  // namespace NebulaEmu