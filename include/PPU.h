#pragma once

#include <cstdint>

namespace NebulaEmu {

class PPU {
public:
    void reset();

    void step();

    uint8_t readPPUSTATUS();

    uint8_t readOAMDATA();

    uint8_t readPPUDATA();

    void writePPUCTRL(uint8_t data);

    void writePPUCMASK(uint8_t data);

    void writeOAMADDR(uint8_t addr);

    void writeOAMDATA(uint8_t data);

    void writePPUSCROLL(uint8_t data);

    void writePPUADDR(uint8_t addr);

    void writePPUDATA(uint8_t data);

    void DMA(uint8_t addr);

private:
    uint8_t read(uint16_t addr);

    void write(uint16_t addr, uint8_t data);

    uint16_t addrInc() { return _PPUCTRL.bits.I == 0 ? 1 : 32; }

    union {
        struct {
            uint8_t NN : 2;  // nametable select
            uint8_t I : 1;   // increment mode
            uint8_t S : 1;   // sprite tile select
            uint8_t B : 1;   // background tile select
            uint8_t H : 1;   // sprite height
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
            uint8_t O : 1;  // sprite overflow
            uint8_t S : 1;  // sprite 0 hit
            uint8_t V : 1;  // vblank
        } bits;
        uint8_t value;
    } _PPUSTATUS;

    bool _oddFrame;

    uint16_t _v;  // current VRAM address
    uint16_t _t;  // temp VRAM address
    uint8_t _x;   // fine X scroll 3bits
    bool _w;      // write toggle 0-->first 1-->second

    uint8_t _OAMAddr;

    // NameTable0 begin at 0, NameTable1 begin at 0x400
    uint8_t _VRAM[0x800];
    uint8_t _palette[0x20];

    uint8_t _OAM[0x100];

    int _scanline;
    int _cycles;
};

}  // namespace NebulaEmu