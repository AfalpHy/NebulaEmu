#include "Emulator.h"

using namespace std;

namespace NebulaEmu {

void Emulator::run(string path) { _cartridge.load(path); }
}  // namespace NebulaEmu