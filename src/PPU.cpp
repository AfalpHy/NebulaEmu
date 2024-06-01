#include "PPU.h"

#include <algorithm>
#include <iostream>

#include "CPU.h"
#include "Cartridge.h"
namespace NebulaEmu {

extern Cartridge* cartridge;
extern CPU* cpuu;

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
    _PPUCTRL.value = _PPUMASK.value = _PPUSTATUS.value = 0;

    _oddFrame = false;

    _v = 0;
    _x = 0;
    _w = 0;

    _OAMADDR = 0;

    _scanline = 0;
    _cycles = 0;
};

void PPU::step() {}

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
    _VRAM[_v] = data;
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
        //  every four bytes in the palettes is a copy of $3F00
        if (addr % 4 == 0) {
            addr = 0;
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
        //  every four bytes in the palettes is a copy of $3F00
        if (addr % 4 == 0) {
            addr = 0;
        }
        _palette[addr] = data;
    }
}

}  // namespace NebulaEmu