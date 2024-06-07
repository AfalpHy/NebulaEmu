#pragma once

#include <cstdint>

namespace NebulaEmu {

class APU {
public:
    void reset();

    void step();

    uint8_t sample();

    uint8_t readStatus();

    // pulse
    void writePulse0(bool one, uint8_t data);

    void writePulse1(bool one, uint8_t data);

    void writePulse2(bool one, uint8_t data);

    void writePulse3(bool one, uint8_t data);

    // triangle
    void writeTriangle0(uint8_t data);

    void writeTriangle2(uint8_t data);

    void writeTriangle3(uint8_t data);

    // noise
    void writeNoise0(uint8_t data);

    void writeNoise2(uint8_t data);

    void writeNoise3(uint8_t data);

    // DMC
    void writeDMC0(uint8_t data);

    void writeDMC1(uint8_t data);

    void writeDMC2(uint8_t data);

    void writeDMC3(uint8_t data);

    void writeStatus(uint8_t data);

    void writeFrameCounter(uint8_t data);

private:
    struct PulseChannel {
        uint8_t duty;
        bool envelopeLoop;
        bool lengthCounterHalt;
        bool constantVolume;
        uint8_t volume;
        union {
            struct {
                uint8_t S : 3;  // shift
                bool N : 1;     // negate
                uint8_t P : 3;  // period
                bool E : 1;     // enable
            } bits;
            uint8_t value;
        } sweepUnit;
        uint16_t timer;
        uint8_t lengthCounter;

        bool envelopStart;
        uint8_t divider;
        uint8_t decayLevel;
        uint8_t output;
    } _pulse1, _pulse2;

    struct TriangleChannel {
        bool lengthCounterHalt;
        uint8_t linearCounterLoad;
        uint16_t timer;
        uint8_t lengthCounter;
    } _triangle;

    struct NoiseChannel {
        bool lengthCounterHalt;
        bool constantVolume;
        uint8_t volume;
        bool loopNoise;
        uint8_t noisePreiod;
        uint8_t lengthCounter;
    } _noise;

    struct DMCChannel {
        bool IRQenable;
        bool loop;
        uint8_t frequency;
        uint8_t loadCounter;
        uint8_t sampleAddress;
        uint8_t sampleLength;
    } _DMC;

    union {
        struct {
            bool P1 : 1;  // pulse1
            bool P2 : 1;  // pulse2
            bool T : 1;   // triangle
            bool N : 1;   // noise
            bool D : 1;   // enable DMC
            bool U : 1;   // unused
            bool F : 1;   // frame interrupt
            bool I : 1;   // DMC interrupt
        } bits;
        uint8_t value;
    } _State;

    bool _M;
    bool _I;

    float linearApproximationMix();

    uint8_t calculatePulse(PulseChannel &pulse);

    uint8_t calculateTriangle();

    uint8_t calculateNoise();

    uint8_t calculateDMC();

    void quarterFrameClock();

    void halfFrameClock();

    uint64_t _cycles = 0;
    uint64_t _sampleIndex = 0;
};

}  // namespace NebulaEmu