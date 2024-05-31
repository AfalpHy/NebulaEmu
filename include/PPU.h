#pragma once

#include <cstdint>

namespace NebulaEmu {

class PPU {
public:
    void reset();

    void step();

    uint8_t readPPUSTATUS() { return _PPUSTATUS.value; }

    uint8_t readOAMDATA() { return _OAMDATA; }

    uint8_t readPPUDATA() { return _PPUDATA; }

    void writePPUCTRL(uint8_t data) { _PPUCTRL.value = data; }

    void writePPUCMASK(uint8_t data) { _PPUMASK.value = data; }

    void writeOAMADDR(uint8_t data) { _OAMADDR = data; }

    void writeOAMDATA(uint8_t data) { _OAMDATA = data; }

    void writePPUSCROLL(uint8_t data) { _PPUSCROLL = data; }

    void writePPUADDR(uint8_t data) { _PPUADDR = data; }

    void writePPUDATA(uint8_t data) { _PPUDATA = data; }

    void DMA(uint8_t addr) {
        /// TODO:
    }

private:
    uint8_t readByte(uint16_t addr);

    union {
        struct {
            uint8_t NN : 2;  // nametable select
            uint8_t I : 1;   // increment mode
            uint8_t S : 1;   //  sprite tile select
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
            uint8_t m : 1;    //  background left column enable
            uint8_t M : 1;    //  sprite left column enable
            uint8_t b : 1;    //  background enable
            uint8_t s : 1;    // sprite enable
            uint8_t BGR : 3;  // color emphasis
        } bits;
        uint8_t value;
    } _PPUMASK;

    union {
        struct {
            uint8_t U : 5;  // unused
            uint8_t O : 1;  //  sprite overflow
            uint8_t S : 1;  //  sprite 0 hit
            uint8_t V : 1;  // vblank
        } bits;
        uint8_t value;
    } _PPUSTATUS;

    uint8_t _OAMADDR;
    uint8_t _OAMDATA;
    uint8_t _PPUSCROLL;
    uint8_t _PPUADDR;
    uint8_t _PPUDATA;

    bool _oddFrame;

    uint8_t _vRAM[0x800];
};

}  // namespace NebulaEmu