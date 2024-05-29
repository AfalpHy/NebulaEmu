#pragma once

#include "CPU.h"

namespace NebulaEmu {
class Emulator {
public:
  Emulator() {}

  void run();

private:
  CPU *_cpu = nullptr;
};

} // namespace NebulaEmu