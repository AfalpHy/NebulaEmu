#include <iostream>

#include "Emulator.h"

int main(int argc, char **argv) {
    NebulaEmu::Emulator emulator;
    emulator.run(argv[1]);

    return 0;
}