#include "Cartridge.h"

#include <cstring>
#include <fstream>
#include <iostream>

using namespace std;

namespace NebulaEmu {

void Cartridge::load(string path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file \"" << path << "\"" << endl;
        exit(1);
    }

    uint8_t header[16];
    file.read((char*)header, 16);

    if (!(header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == 0x1A)) {
        cerr << "unkown format" << endl;
        exit(1);
    }

    if ((header[7] & 0x0C) == 0x08) {
        // NES 2.0
        cerr << "NES 2.0 format unsupported" << endl;
        exit(2);
    }

    _mirroring = (NameTableMirroring)(header[6] & 0x01);

    if (header[6] & 0x02) {
        _battery_backed_RAM = (uint8_t*)malloc(0X2000);
        memset(_battery_backed_RAM, 0, 0x2000);
    }

    if (header[6] & 0x04) {
        // _trainer = (uint8_t*)malloc(512);
        // file.read((char*)_trainer, 512);
        cerr << "Trainer unsupported" << endl;
        exit(1);
    }

    if (header[6] & 0x08) {
        // poorly supporting
        _mirroring = FourScreen;
    }

    _mapper = Mapper::createMapper((header[7] & 0xF0) | ((header[6] & 0xF0) >> 4));

    unsigned _PRG_ROM_size = header[4] * 0x4000;
    _PRG_ROM.reserve(_PRG_ROM_size);
    file.read((char*)&_PRG_ROM[0], _PRG_ROM_size);

    unsigned _CHR_ROM_size = header[5] * 0x2000;
    _CHR_ROM.reserve(_CHR_ROM_size);
    file.read((char*)&_CHR_ROM[0], _CHR_ROM_size);
}

}  // namespace NebulaEmu