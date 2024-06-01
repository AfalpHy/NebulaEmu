#pragma once

#include <cstdint>

namespace NebulaEmu {

class PPU {
public:
    void reset();

    void step();

    // address 0x2002
    uint8_t readPPUSTATUS();

    // address 0x2004
    uint8_t readOAMDATA();

    // address 0x2007
    uint8_t readPPUDATA();

    // address 0x2000
    void writePPUCTRL(uint8_t data);

    // address 0x2001
    void writePPUCMASK(uint8_t data);

    // address 0x2003
    void writeOAMADDR(uint8_t addr);

    // address 0x2004
    void writeOAMDATA(uint8_t data);

    // address 0x2005
    void writePPUSCROLL(uint8_t data);

    // address 0x2006
    void writePPUADDR(uint8_t addr);

    // address 0x2007
    void writePPUDATA(uint8_t data);

    // address 0x4014
    void OAMDMA(uint8_t* addr);

private:
    uint8_t read(uint16_t addr);

    void write(uint16_t addr, uint8_t data);

    uint16_t addrInc() { return _PPUCTRL.bits.I == 0 ? 1 : 32; }

    union {
        struct {
            uint8_t NN : 2;  // nametable select
            uint8_t I : 1;   // increment mode
            uint8_t S : 1;   // sprite tile select, 0: 0x0000, 1:0x1000
            uint8_t B : 1;   // background tile select, 0: 0x0000, 1:0x1000
            uint8_t H : 1;   // sprite height, 0: 8*8 pixels, 1: 8*16 pixels
            uint8_t P : 1;   // PPU master/slave
            uint8_t V : 1;   // NMI enable
        } bits;
        uint8_t value;
    } _PPUCTRL;

    union {
        struct {
            uint8_t G : 1;    // greyscale
            uint8_t m : 1;    // background left column enable
            uint8_t M : 1;    // sprite left column enable
            uint8_t b : 1;    // background enable
            uint8_t s : 1;    // sprite enable
            uint8_t BGR : 3;  // color emphasis
        } bits;
        uint8_t value;
    } _PPUMASK;

    union {
        struct {
            uint8_t U : 5;  // unused
            uint8_t O : 1;  // sprite overflow This flag is set during sprite evaluation and cleared at dot 1
                            // (the second dot) of the pre-render line.

            uint8_t S : 1;  // sprite 0 hit.
                            // Sprite 0 hit is not detected at x=255, nor is it detected at x=0 through 7
                            // if the background or sprites are hidden in this area.
            uint8_t V : 1;  // vblank
        } bits;
        uint8_t value;
    } _PPUSTATUS;

    bool _oddFrame;

    uint16_t _v;  // current VRAM address
    uint16_t _t;  // temporary VRAM address
    uint8_t _x;   // fine X scroll, 3bits
    bool _w;      // write toggle, 0-->first, 1-->second

    uint8_t _OAMADDR;

    // NameTable0 begin at 0, NameTable1 begin at 0x400
    uint8_t _VRAM[0x800];
    uint8_t _palette[0x20];

    uint8_t _OAM[0x100];

    int _scanline;
    int _cycles;
};

}  // namespace NebulaEmu