#pragma once

#include <cstdint>
#include <vector>

namespace NebulaEmu {

class APU {
public:
    APU();

    void reset();

    void step();

    std::vector<uint8_t> &getBuffer() { return _buffer; }

    uint64_t getSampleIndex() { return _sampleIndex; }

    uint8_t readStatus();

    // pulse
    void writePulseReg0(bool pulse1, uint8_t data);

    void writePulseReg1(bool pulse1, uint8_t data);

    void writePulseReg2(bool pulse1, uint8_t data);

    void writePulseReg3(bool pulse1, uint8_t data);

    // triangle
    void writeTriangleReg0(uint8_t data);

    void writeTriangleReg2(uint8_t data);

    void writeTriangleReg3(uint8_t data);

    // noise
    void writeNoiseReg0(uint8_t data);

    void writeNoiseReg2(uint8_t data);

    void writeNoiseReg3(uint8_t data);

    // DMC
    void writeDMCReg0(uint8_t data);

    void writeDMCReg1(uint8_t data);

    void writeDMCReg2(uint8_t data);

    void writeDMCReg3(uint8_t data);

    void writeStatus(uint8_t data);

    void writeFrameCounter(uint8_t data);

private:
    struct Envelope {
        bool start;
        bool loop;
        bool constantVolume;
        uint8_t volume;
        uint8_t divider;
        uint8_t decayLevelCounter;
        uint8_t output;

        void clock();
    };

    struct PulseChannel {
        uint8_t sequence;
        bool lengthCounterHalt;

        uint16_t timer;
        uint8_t lengthCounter;

        Envelope envelope;

        struct Sweep {
            bool reload;
            bool enable;
            bool negate;
            uint8_t period;
            uint8_t shiftCount;
            uint8_t divider;

            bool mute;
            void clock(bool pulse1, uint16_t &timer);
        } sweep;

        struct Sequencer {
            uint16_t timer;
            uint8_t sequence;
            bool output;

            void clock(uint16_t timer);
        } sequencer;

    } _pulse1, _pulse2;

    struct TriangleChannel {
        // Control flag (this bit is also the length counter halt flag)
        bool lengthCounterHalt;
        uint8_t counterReload;
        uint8_t linearCounter;
        uint16_t timer;
        uint8_t lengthCounter;

        struct Sequencer {
            uint16_t timer;
            uint8_t index;

            void clock(uint16_t timer);
        } sequencer;

        bool linearCounterReload;
    } _triangle;

    struct NoiseChannel {
        bool lengthCounterHalt;

        bool mode;

        uint16_t noisePeriod;
        uint8_t lengthCounter;

        uint16_t timer;
        uint8_t shiftReg;

        Envelope envelope;

        void clock();
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

    float lookupTable();

    uint8_t calculatePulse(PulseChannel &pulse);

    uint8_t calculateTriangle();

    uint8_t calculateNoise();

    uint8_t calculateDMC();

    void quarterFrameClock();

    void halfFrameClock();

    void sample();

    uint64_t _cycles = 0;

    uint64_t _sampleIndex = 0;

    std::vector<uint8_t> _buffer;
    std::vector<float> _pulseTable;
    std::vector<float> _tndTable;
};

}  // namespace NebulaEmu