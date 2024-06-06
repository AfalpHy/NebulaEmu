#pragma once

#include <cstdint>
#include <vector>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240

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

    bool renderEnable() { return _PPUMASK.bits.b & _PPUMASK.bits.s; }

    union {
        struct {
            uint8_t NN : 2;  // nametable select
            bool I : 1;      // increment mode
            bool S : 1;      // sprite tile select, 0: 0x0000, 1:0x1000
            bool B : 1;      // background tile select, 0: 0x0000, 1:0x1000
            bool H : 1;      // sprite height, 0: 8*8 pixels, 1: 8*16 pixels
            bool P : 1;      // PPU master/slave
            bool V : 1;      // NMI enable
        } bits;
        uint8_t value;
    } _PPUCTRL;

    union {
        struct {
            bool G : 1;       // greyscale
            bool m : 1;       // background left 8 column enable
            bool M : 1;       // sprite left 8 column enable
            bool b : 1;       // background enable
            bool s : 1;       // sprite enable
            uint8_t BGR : 3;  // color emphasis
        } bits;
        uint8_t value;
    } _PPUMASK;

    union {
        struct {
            uint8_t U : 5;  // unused
            bool O : 1;     // sprite overflow This flag is set during sprite evaluation and cleared at dot 1
                            // (the second dot) of the pre-render line.

            bool S : 1;  // sprite 0 hit.
                         // Sprite 0 hit is not detected at x=255, nor is it detected at x=0 through 7
                         // if the background or sprites are hidden in this area.
            bool V : 1;  // vblank
        } bits;
        uint8_t value;
    } _PPUSTATUS;

    bool _oddFrame;

    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    // current VRAM address
    uint16_t _v;
    uint16_t _t;  // temporary VRAM address
    uint8_t _x;   // fine X scroll, 3bits
    bool _w;      // write toggle, 0-->first, 1-->second

    uint8_t _OAMADDR;

    // NameTable0 begin at 0, NameTable1 begin at 0x400
    uint8_t _VRAM[0x800];
    uint8_t _palette[0x20];

    uint8_t _OAM[0x100];
    std::vector<uint8_t> _secondaryOAM;

    uint32_t _buffer[SCREEN_HEIGHT][SCREEN_WIDTH];

    int _scanline;
    int _cycles;
};

}  // namespace NebulaEmu