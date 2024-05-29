#pragma once

#include "CPU.h"
#include "Cartridge.h"

namespace NebulaEmu {
class Emulator {
public:
    void run(std::string path);

private:
    Cartridge _cartridge;
    CPU _cpu;
};

}  // namespace NebulaEmu