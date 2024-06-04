#include "PPU.h"

#include <algorithm>
#include <iostream>

#include "CPU.h"
#include "Cartridge.h"
namespace NebulaEmu {

extern Cartridge* cartridge;
extern CPU* cpu;

extern uint32_t* pixels;

const unsigned systemPalette[] = {
    0x666666ff, 0x002a88ff, 0x1412a7ff, 0x3b00a4ff, 0x5c007eff, 0x6e0040ff, 0x6c0600ff, 0x561d00ff,
    0x333500ff, 0x0b4800ff, 0x005200ff, 0x004f08ff, 0x00404dff, 0x000000ff, 0x000000ff, 0x000000ff,
    0xadadadff, 0x155fd9ff, 0x4240ffff, 0x7527feff, 0xa01accff, 0xb71e7bff, 0xb53120ff, 0x994e00ff,
    0x6b6d00ff, 0x388700ff, 0x0c9300ff, 0x008f32ff, 0x007c8dff, 0x000000ff, 0x000000ff, 0x000000ff,
    0xfffeffff, 0x64b0ffff, 0x9290ffff, 0xc676ffff, 0xf36affff, 0xfe6eccff, 0xfe8170ff, 0xea9e22ff,
    0xbcbe00ff, 0x88d800ff, 0x5ce430ff, 0x45e082ff, 0x48cddeff, 0x4f4f4fff, 0x000000ff, 0x000000ff,
    0xfffeffff, 0xc0dfffff, 0xd3d2ffff, 0xe8c8ffff, 0xfbc2ffff, 0xfec4eaff, 0xfeccc5ff, 0xf7d8a5ff,
    0xe4e594ff, 0xcfef96ff, 0xbdf4abff, 0xb3f3ccff, 0xb5ebf2ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

void PPU::reset() {
    _PPUCTRL.value = _PPUSTATUS.value = 0;
    _PPUMASK.value = 0x1E;

    _oddFrame = false;

    _v = 0;
    _x = 0;
    _w = 0;

    _OAMADDR = 0;

    _scanline = 261;
    _cycles = 0;
};

void PPU::step() {
    if (_scanline < 240) {  // Rendering
        if (_cycles > 0 && _cycles <= 256) {
            int x = _cycles - 1;
            int y = _scanline;
            bool bgOpaque = false;
            uint16_t paletteEntry = 0;

            // background enable
            if (_PPUMASK.bits.b) {
                int fineX = (_x + x) % 8;
                if (_PPUMASK.bits.m || x >= 8) {
                    uint16_t nameTableAddr = 0x2000 | (_v & 0x0FFF);
                    uint16_t tileIndex = read(nameTableAddr);

                    // 8*8 tile, get target line data by add fineY
                    uint16_t patternTableAddr = tileIndex * 16 + ((_v >> 12) & 0x7);
                    // add 0x1000 if high page
                    patternTableAddr |= _PPUCTRL.bits.B << 12;

                    paletteEntry = (read(patternTableAddr) >> (7 - fineX)) & 1;
                    paletteEntry |= ((read(patternTableAddr + 8) >> (7 - fineX)) & 1) << 1;

                    // The palette entry at $3F00 is the background colour and is used for transparency.
                    // Addresses $3F04/$3F08/$3F0C are not used by the PPU when normally rendering
                    bgOpaque = paletteEntry;

                    // if bgOpaque is false, keep the palette entry as $3F00 for transparency
                    // otherwise, continue to obtain the upper two bit
                    if (bgOpaque) {
                        uint16_t attributeTabelAddr = 0x23C0 | (_v & 0x0C00) | ((_v >> 4) & 0x38) | ((_v >> 2) & 0x07);
                        auto attribute = read(attributeTabelAddr);
                        // The layout of the byte is 33221100 where every two bits specifies the most significant two
                        // colour bits for the specified square.
                        // every suqre has 4 tile, map the tile to square to get the shift of attribute
                        // |---------------------|
                        // |          |          |
                        // | Square 0 | Square 1 |
                        // |          |          |
                        // |----------+ ---------|
                        // |          |          |
                        // | Square 2 | Square 3 |
                        // |          |          |
                        // |---------------------|
                        // Square bit 1:(coarse Y / 2) % 2
                        // Square bit 0:(coarse X / 2) % 2
                        // (bit 1|bit 0)*2 equals (bit 1|bit 0) << 1
                        int shift = ((_v >> 4) & 4) | (_v & 2);
                        paletteEntry |= ((attribute >> shift) & 0x3) << 2;
                    }
                }

                if (fineX == 7) {
                    if ((_v & 0x001F) == 31) {
                        _v &= ~0x001F;
                        _v ^= 0x400;
                    } else {
                        _v += 1;
                    }
                }
            }

            // sprite enable
            if (_PPUMASK.bits.s && (_PPUMASK.bits.M || x >= 8)) {
                for (auto i : _secondaryOAM) {
                    uint8_t topX = _OAM[i * 4 + 3];

                    if (x < topX || x >= topX + 8) {
                        continue;
                    }
                    // Sprite data is delayed by one scanline;
                    // you must subtract 1 from the sprite's Y coordinate before writing it,
                    // this is why we plus 1 while reading
                    uint8_t topY = _OAM[i * 4 + 0] + 1;
                    uint8_t tileIndex = _OAM[i * 4 + 1];

                    // 76543210
                    // ||||||||
                    // ||||||++- Palette (4 to 7) of sprite
                    // |||+++--- Unimplemented (read 0)
                    // ||+------ Priority (0: in front of background; 1: behind background)
                    // |+------- Flip sprite horizontally
                    // +-------- Flip sprite vertically
                    uint8_t attribute = _OAM[i * 4 + 2];

                    int height = (_PPUCTRL.bits.H) ? 16 : 8;

                    int xShift = x - topX;
                    int offsetY = (y - topY) % height;

                    // not flipping horizontally
                    if ((attribute & 0x40) == 0) {
                        xShift = 7 - xShift;
                    }
                    // flipping vertically
                    if ((attribute & 0x80) != 0) {
                        offsetY = (height - 1) - offsetY;
                    }

                    uint16_t patternTableAddr = 0;

                    if (!_PPUCTRL.bits.H) {
                        patternTableAddr = tileIndex * 16 + offsetY;
                        if (_PPUCTRL.bits.S) {
                            patternTableAddr += 0x1000;
                        }
                    } else {  // 8x16 sprites
                        // top tile and bottom tile
                        // memory map: top tile(byte 0-7, byte 8-15) bottom tile(byte 16-23, byte 24-31)
                        // bit-3 is one if it is the bottom tile of the sprite, multiply by two to get the next pattern
                        offsetY = (offsetY & 7) | ((offsetY & 8) << 1);
                        patternTableAddr = (tileIndex >> 1) * 32 + offsetY;
                        // For 8x16 sprites (bit 5 of PPUCTRL set), the PPU ignores the pattern table selection and
                        // selects a pattern table from bit 0 of this number.
                        patternTableAddr |= (tileIndex & 1) << 12;
                    }
                    uint8_t sprPaletteEntry = (read(patternTableAddr) >> (xShift)) & 1;      // bit 0 of palette entry
                    sprPaletteEntry |= ((read(patternTableAddr + 8) >> (xShift)) & 1) << 1;  // bit 1

                    // sprite is transparency
                    if (sprPaletteEntry == 0) {
                        continue;
                    }
                    sprPaletteEntry |= (attribute & 0x3) << 2;  // upper two bits
                    sprPaletteEntry |= 0x10;                    // Select sprite palette

                    // if sprite is foreground or backgournd is not opaque
                    if (!(attribute & 0x20) || !bgOpaque) {
                        paletteEntry = sprPaletteEntry;
                    }
                    // Sprite-0 hit detection
                    if (!_PPUSTATUS.bits.S && i == 0 && bgOpaque) {
                        _PPUSTATUS.bits.S = true;
                    }

                    break;
                }
            }
            _buffer[x][y] = systemPalette[read(paletteEntry + 0x3F00)];
        }
        if (_cycles == 256 && _PPUMASK.bits.b) {
            if ((_v & 0x7000) != 0x7000) {  // if fine Y < 7
                _v += 0x1000;               // increment fine Y
            } else {
                _v &= ~0x7000;               // fine Y = 0
                int y = (_v & 0x03E0) >> 5;  // let y = coarse Y
                if (y == 29) {
                    y = 0;             // coarse Y = 0
                    _v ^= 0x0800;      // switch vertical nametable
                } else if (y == 31) {  // coarse Y = 0, nametable not switched
                    y = 0;
                } else {  // increment coarse Y
                    y += 1;
                }
                _v = (_v & ~0x03E0) | (y << 5);  // put coarse Y back into v
            }
        }
        if (_cycles == 257 && renderEnable()) {
            // If rendering is enabled, the PPU copies all bits related to horizontal position from t to v
            _v &= ~0x41f;
            _v |= _t & 0x41f;
        }
        // update _secondaryOAM for next line
        if (_cycles == 340) {
            int height = (_PPUCTRL.bits.H) ? 16 : 8;
            _secondaryOAM.clear();

            for (int i = _OAMADDR / 4; i < 64; ++i) {
                int topY = _OAM[i * 4];
                if (_scanline >= topY && _scanline < topY + height) {
                    if (_secondaryOAM.size() >= 8) {
                        _PPUSTATUS.bits.O = true;
                        break;
                    }
                    _secondaryOAM.push_back(i);
                }
            }
        }
    } else if (_scanline == 240) {  // PostRender
        // update pixel once per frame
        if (_cycles == 1) {
            for (int i = 0; i < SCREEN_WIDTH; i++) {
                for (int j = 0; j < SCREEN_HEIGHT; j++) {
                    pixels[j * SCREEN_WIDTH + i] = _buffer[i][j];
                }
            }
        }
    } else if (_scanline < 261) {  // Vertical blanking
        if (_scanline == 241 && _cycles == 1) {
            _PPUSTATUS.bits.V = true;
            if (_PPUCTRL.bits.V) {
                cpu->setNMIPin();
            }
        }
    } else {  // PreRender
        if (_cycles == 1) {
            _PPUSTATUS.bits.V = false;
            _PPUSTATUS.bits.S = false;
            _PPUSTATUS.bits.O = false;
        } else if (_cycles == 257 && renderEnable()) {
            // If rendering is enabled, the PPU copies all bits related to horizontal position from t to v
            _v &= ~0x41f;
            _v |= _t & 0x41f;
        } else if (_cycles >= 280 && _cycles <= 304 && renderEnable()) {
            // If rendering is enabled, the PPU copies all bits related to vertical position from t to v:
            _v &= 0x41f;
            _v |= _t & ~0x41f;
        } else if (_cycles == 339 && (_oddFrame & renderEnable())) {
            // skipped end of the scanline
            _cycles++;
        }
    }

    _cycles++;
    if (_cycles == 341) {
        _cycles = 0;
        _scanline++;
        if (_scanline == 262) {
            _scanline = 0;
            _oddFrame = !_oddFrame;
        }
    }
}

uint8_t PPU::readPPUSTATUS() {
    uint8_t tmp = _PPUSTATUS.value;
    // Reading the status register will clear bit 7 mentioned above and also the address latch used by PPUSCROLL and
    // PPUADDR. It does not clear the sprite 0 hit or overflow bit.
    _PPUSTATUS.bits.V = 0;
    _w = 0;
    return tmp;
}

uint8_t PPU::readOAMDATA() { return _OAM[_OAMADDR]; }

uint8_t PPU::readPPUDATA() {
    uint8_t data = read(_v);
    // Outside of rendering, reads from or writes to $2007 will add either 1 or 32 to v depending on the VRAM increment
    // bit set via $2000.
    _v += addrInc();
    /// TODO: During  rendering it will update the _v in an odd way

    // When reading PPUDATA while the VRAM address is in the range 0–$3EFF (i.e., before the palettes), the read will
    // return the contents of an internal read buffer.
    if (_v < 0x3f00) {
        static uint8_t dataBuffer = 0;
        std::swap(data, dataBuffer);
    }
    return data;
}

void PPU::writePPUCTRL(uint8_t data) {
    _PPUCTRL.value = data;

    _t &= ~0xC00;
    _t |= _PPUCTRL.bits.NN << 10;
}

void PPU::writePPUCMASK(uint8_t data) { _PPUMASK.value = data; }

void PPU::writeOAMADDR(uint8_t addr) { _OAMADDR = addr; }

void PPU::writeOAMDATA(uint8_t data) {
    // Writes will increment OAMADDR after the write; reads do not
    _OAM[_OAMADDR++] = data;
    /// TODO:
    // Writes to OAMDATA during rendering (on the pre-render line and the visible lines 0–239, provided either sprite or
    // background rendering is enabled) do not modify values in OAM, but do perform a glitchy increment of OAMADDR,
    // bumping only the high 6 bits (i.e., it bumps the [n] value in PPU sprite evaluation – it's plausible that it
    // could bump the low bits instead depending on the current status of sprite evaluation).This extends to DMA
    // transfers via OAMDMA, since that uses writes to $2004. For emulation purposes, it is probably best to completely
    // ignore writes during rendering.
}

void PPU::writePPUSCROLL(uint8_t data) {
    if (!_w) {
        _t &= ~0x1f;
        _t |= (data >> 3) & 0x1f;
        _x = data & 0x7;
        _w = true;
    } else {
        _t &= ~0x73e0;
        _t |= ((data & 0x7) << 12) | ((data & 0xf8) << 2);
        _w = false;
    }
}

void PPU::writePPUADDR(uint8_t addr) {
    if (!_w) {
        _t &= ~0xff00;
        _t |= (addr & 0x3f) << 8;
        _w = true;
    } else {
        _t &= ~0xff;
        _t |= addr;
        _v = _t;
        _w = false;
    }
}

void PPU::writePPUDATA(uint8_t data) {
    write(_v, data);
    // Outside of rendering, reads from or writes to $2007 will add either 1 or 32 to v depending on the VRAM increment
    // bit set via $2000.
    _v += addrInc();
    /// TODO: During  rendering it will update the _v in an odd way
}

void PPU::OAMDMA(uint8_t* addr) {
    for (int i = 0; i < 0x100; i++) {
        _OAM[(_OAMADDR + i) % 0x100] = addr[i];
    }
}

uint8_t PPU::read(uint16_t addr) {
    addr &= 0x3FFF;
    if (addr < 0x2000) {
        return cartridge->getMapper()->readCHR(addr);
    } else if (addr < 0x3F00) {
        // Mirrors 0x2000-0x2EFF
        if (addr >= 0x3000) {
            addr -= 0X1000;
        }
        if (addr < 0x2400) {  // L1
            // NameTable0
            return _VRAM[addr - 0x2000];
        } else if (addr < 0x2800) {  // L2
            switch (cartridge->getMapper()->getNameTableMirroing()) {
                case Horizontal:
                    // NameTable0
                    return _VRAM[addr - 0x2400];
                case Vertical:
                    // NameTable1
                    return _VRAM[addr - 0x2000];
                default:
                    std::cerr << "unsupported mirroing" << std::endl;
                    exit(2);
            }
        } else if (addr < 0x2C00) {  // L3
            switch (cartridge->getMapper()->getNameTableMirroing()) {
                case Horizontal:
                    // NameTable1
                    return _VRAM[addr - 0x2400];
                case Vertical:
                    // NameTable0
                    return _VRAM[addr - 0x2800];
                default:
                    std::cerr << "unsupported mirroing" << std::endl;
                    exit(2);
            }
        } else {  // L4
            // NameTable1
            return _VRAM[addr - 0x2800];
        }
    } else {
        addr = addr & 0x1f;
        //  Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
        if (addr >= 0x10 && addr % 4 == 0) {
            addr &= 0xf;
        }
        return _palette[addr];
    }
}

void PPU::write(uint16_t addr, uint8_t data) {
    // mirrors 0x0000-0x3FFF
    addr &= 0x3FFF;
    if (addr < 0x2000) {
        cartridge->getMapper()->wirteCHR(addr, data);
    } else if (addr < 0x3F00) {
        // Mirrors 0x2000-0x2EFF
        if (addr >= 0x3000) {
            addr -= 0X1000;
        }
        if (addr < 0x2400) {  // L1
            // NameTable0
            _VRAM[addr - 0x2000] = data;
        } else if (addr < 0x2800) {  // L2
            switch (cartridge->getMapper()->getNameTableMirroing()) {
                case Horizontal:
                    // NameTable0
                    _VRAM[addr - 0x2400] = data;
                    break;
                case Vertical:
                    // NameTable1
                    _VRAM[addr - 0x2000] = data;
                    break;
                default:
                    std::cerr << "unsupported mirroing" << std::endl;
                    exit(2);
            }
        } else if (addr < 0x2C00) {  // L3
            switch (cartridge->getMapper()->getNameTableMirroing()) {
                case Horizontal:
                    // NameTable1
                    _VRAM[addr - 0x2400] = data;
                    break;
                case Vertical:
                    // NameTable0
                    _VRAM[addr - 0x2800] = data;
                    break;
                default:
                    std::cerr << "unsupported mirroing" << std::endl;
                    exit(2);
            }
        } else {  // L4
            // NameTable1
            _VRAM[addr - 0x2800] = data;
        }
    } else {
        addr = addr & 0x1f;
        //  Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
        if (addr >= 0x10 && addr % 4 == 0) {
            addr &= 0xf;
        }
        _palette[addr] = data;
    }
}

}  // namespace NebulaEmu