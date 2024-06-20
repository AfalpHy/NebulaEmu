#include "APU.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "CPU.h"

namespace NebulaEmu {

extern CPU* cpu;

static uint8_t lengthTable[] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                                12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static uint8_t triangleSequence[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                                     0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static uint16_t noiseTimerPeriod[] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

APU::APU() {
    _pulseTable.push_back(0.0);
    for (int i = 1; i < 31; i++) {
        _pulseTable.push_back(95.52 / (8128.0 / i + 100));
    }
    _tndTable.push_back(0.0);
    for (int i = 1; i < 203; i++) {
        _tndTable.push_back(163.67 / (24329.0 / i + 100));
    }
}

void APU::reset() { _buffer.resize(65536); }

void APU::step() {
    _cycles++;

    _pulse1.sequencer.clock(_pulse1.timer);
    _pulse2.sequencer.clock(_pulse2.timer);

    // this timer ticks at the rate of the CPU clock rather than the APU (CPU/2) clock
    _triangle.sequencer.clock(_triangle.timer);
    _triangle.sequencer.clock(_triangle.timer);

    _noise.clock();
    _noise.clock();

    if (_cycles == 3729) {
        quarterFrameClock();
    } else if (_cycles == 7457) {
        quarterFrameClock();
        halfFrameClock();
    } else if (_cycles == 11186) {
        quarterFrameClock();
    } else if (_cycles == 14915) {
        if (!_M) {
            quarterFrameClock();
            halfFrameClock();
            if (!_I) {
                cpu->setIRQPin();
            }
            _cycles = 0;
        }
    } else if (_cycles == 18641) {
        quarterFrameClock();
        halfFrameClock();
        _cycles = 0;
    }

    // With an audio sampling rate of 441 kHz, approximately one sample is taken every 20 APU cycles.
    if (_cycles % 20 == 0) {
        sample();
    }
}

void APU::sample() {
    // _buffer[_sampleIndex++ % _buffer.size()] = linearApproximationMix() * 255;
    _buffer[_sampleIndex++ % _buffer.size()] = lookupTable() * 255;
}

uint8_t APU::readStatus() {
    uint8_t ret = (_noise.lengthCounter > 0) << 3 | (_triangle.lengthCounter > 0) << 2 |
                  (_pulse2.lengthCounter > 0) << 1 | (_pulse1.lengthCounter > 0);
    return ret;
}

void APU::writePulseReg0(bool pulse1, uint8_t data) {
    if (pulse1) {
        uint8_t duty = data >> 6;
        switch (duty) {
            case 0:
                _pulse1.sequence = 0b01000000;
                break;
            case 1:
                _pulse1.sequence = 0b01100000;
                break;
            case 2:
                _pulse1.sequence = 0b01111000;
                break;
            case 3:
                _pulse1.sequence = 0b10011111;
                break;
        }
        _pulse1.envelope.loop = _pulse1.lengthCounterHalt = data & 0x20;
        _pulse1.envelope.constantVolume = data & 0x10;
        _pulse1.envelope.volume = data & 0x0F;

        // side effects
        _pulse1.envelope.start = true;
    } else {
        uint8_t duty = data >> 6;
        switch (duty) {
            case 0:
                _pulse2.sequence = 0b01000000;
                break;
            case 1:
                _pulse2.sequence = 0b01100000;
                break;
            case 2:
                _pulse2.sequence = 0b01111000;
                break;
            case 3:
                _pulse2.sequence = 0b10011111;
                break;
        }
        _pulse2.envelope.loop = _pulse2.lengthCounterHalt = data & 0x20;
        _pulse2.envelope.constantVolume = data & 0x10;
        _pulse2.envelope.volume = data & 0x0F;

        // side effects
        _pulse2.envelope.start = true;
    }
}

void APU::writePulseReg1(bool pulse1, uint8_t data) {
    if (pulse1) {
        _pulse1.sweep.enable = data & 0x80;
        _pulse1.sweep.period = (data >> 4) & 0x7;
        _pulse1.sweep.negate = data & 0x8;
        _pulse1.sweep.shiftCount = data & 0x07;
        // side effects
        _pulse1.sweep.reload = true;
    } else {
        _pulse2.sweep.enable = data & 0x80;
        _pulse2.sweep.period = (data >> 4) & 0x7;
        _pulse2.sweep.negate = data & 0x8;
        _pulse2.sweep.shiftCount = data & 0x07;
        // side effects
        _pulse2.sweep.reload = true;
    }
}

void APU::writePulseReg2(bool pulse1, uint8_t data) {
    if (pulse1) {
        _pulse1.timer &= 0xFF00;
        _pulse1.timer |= data;
    } else {
        _pulse2.timer &= 0xFF00;
        _pulse2.timer |= data;
    }
}

void APU::writePulseReg3(bool pulse1, uint8_t data) {
    if (pulse1) {
        _pulse1.timer &= 0x00FF;
        _pulse1.timer |= (data & 0x7) << 8;
        _pulse1.lengthCounter = lengthTable[data >> 3];

        // side effects
        _pulse1.sequencer.sequence = _pulse1.sequence;
        _pulse1.envelope.start = true;
    } else {
        _pulse2.timer &= 0x00FF;
        _pulse2.timer |= (data & 0x7) << 8;
        _pulse2.lengthCounter = lengthTable[data >> 3];

        // side effects
        _pulse2.sequencer.sequence = _pulse2.sequence;
        _pulse2.envelope.start = true;
    }
}

void APU::writeTriangleReg0(uint8_t data) {
    _triangle.lengthCounterHalt = data >> 7;
    _triangle.counterReload = data & 0x7F;
}

void APU::writeTriangleReg2(uint8_t data) {
    _triangle.timer &= 0xFF00;
    _triangle.timer |= data;
}

void APU::writeTriangleReg3(uint8_t data) {
    _triangle.timer &= 0x00FF;
    _triangle.timer |= (data & 0x7) << 8;
    _triangle.lengthCounter = lengthTable[data >> 3];

    // side effects
    _triangle.linearCounterReload = true;
}

void APU::writeNoiseReg0(uint8_t data) {
    _noise.lengthCounterHalt = (data >> 5) & 1;
    _noise.envelope.loop = _noise.envelope.constantVolume = (data >> 4) & 1;
    _noise.envelope.volume = data & 0x0F;
}

void APU::writeNoiseReg2(uint8_t data) {
    _noise.mode = data >> 7;
    _noise.noisePeriod = noiseTimerPeriod[data & 0x0F];
}

void APU::writeNoiseReg3(uint8_t data) {
    _noise.lengthCounter = lengthTable[data >> 3];
    _noise.envelope.start = true;
}

void APU::writeDMCReg0(uint8_t data) {
    _DMC.IRQenable = data >> 7;
    _DMC.loop = (data >> 6) & 1;
    _DMC.frequency = data & 0x0F;
}

void APU::writeDMCReg1(uint8_t data) { _DMC.loadCounter = data & 0x7F; }

void APU::writeDMCReg2(uint8_t data) { _DMC.sampleAddress = data; }

void APU::writeDMCReg3(uint8_t data) { _DMC.sampleLength = data; }

void APU::writeStatus(uint8_t data) {
    _State.value = data & 0x1F;

    // side effects
    _pulse1.envelope.start = true;
    _pulse2.envelope.start = true;
}

void APU::writeFrameCounter(uint8_t data) {
    _M = data >> 7;
    _I = (data >> 6) & 1;
    // If the mode flag is set, then both "quarter frame" and "half frame" signals are also generated
    if (_M) {
        quarterFrameClock();
        halfFrameClock();
    }
}

float APU::linearApproximationMix() {
    float pulseOut = 0.00752 * (calculatePulse(_pulse1) + calculatePulse(_pulse2));
    float tndOut = 0.00851 * calculateTriangle() + 0.00494 * calculateNoise() + 0.00335 * calculateDMC();
    return pulseOut + tndOut;
}

float APU::lookupTable() {
    return _pulseTable[calculatePulse(_pulse1) + calculatePulse(_pulse2)] +
           _tndTable[3 * calculateTriangle() + 2 * calculateNoise() + calculateDMC()];
}

uint8_t APU::calculatePulse(PulseChannel& pulse) {
    if (pulse.sweep.mute || pulse.lengthCounter == 0) {
        return 0;
    }
    return pulse.envelope.output * pulse.sequencer.output;
}

uint8_t APU::calculateTriangle() {
    if (_triangle.linearCounter && _triangle.lengthCounter) {
        return triangleSequence[_triangle.sequencer.index % 32];
    }
    return 0;
}

uint8_t APU::calculateNoise() {
    if ((_noise.shiftReg & 0x1) || _noise.lengthCounter == 0) {
        return 0;
    }
    return _noise.envelope.output;
}

uint8_t APU::calculateDMC() { return 0; }

void APU::Envelope::clock() {
    if (!start) {
        if (divider == 0) {
            divider = volume;
            if (decayLevelCounter == 0) {
                if (loop) {
                    decayLevelCounter = 15;
                }
            } else {
                decayLevelCounter--;
            }
        } else {
            divider--;
        }
    } else {
        start = false;
        decayLevelCounter = 15;
        divider = volume;
    }

    if (constantVolume) {
        output = volume;
    } else {
        output = decayLevelCounter;
    }
}

void APU::PulseChannel::Sweep::clock(bool pulse1, uint16_t& timer) {
    if (divider == 0) {
        if (enable && shiftCount != 0 && !mute) {
            int changeAmount = timer >> shiftCount;
            if (negate) {
                changeAmount = -changeAmount - pulse1;
            }
            if (timer + changeAmount < 0) {
                timer = 0;
            } else {
                timer = timer + changeAmount;
            }
        }
    }

    if (divider == 0 || reload) {
        reload = false;
        divider = period;
    } else {
        divider--;
    }

    mute = timer < 8 || timer > 0x7FF;
}

void APU::PulseChannel::Sequencer::clock(uint16_t timer) {
    if (this->timer == 0) {
        this->timer = timer;
        output = sequence & 0x80;
        sequence = (sequence << 1) | (sequence >> 7);
    } else {
        this->timer--;
    }
}

void APU::TriangleChannel::Sequencer::clock(uint16_t timer) {
    if (this->timer == 0) {
        this->timer = timer;
        index++;
    } else {
        this->timer--;
    }
}

void APU::NoiseChannel::clock() {
    if (timer == 0) {
        timer = noisePeriod;
        bool feedback = (shiftReg & 0x1) ^ ((shiftReg >> (mode ? 6 : 1)) & 0x1);
        shiftReg >>= 1;
        shiftReg |= feedback << 14;
    } else {
        timer--;
    }
}

void APU::quarterFrameClock() {
    _pulse1.envelope.clock();
    _pulse2.envelope.clock();
    _noise.envelope.clock();

    // If the linear counter reload flag is set, the linear counter is reloaded with the counter reload value, otherwise
    // if the linear counter is non-zero, it is decremented
    if (_triangle.linearCounterReload) {
        _triangle.linearCounter = _triangle.counterReload;
    } else {
        if (_triangle.linearCounter > 0) {
            _triangle.linearCounter--;
        }
    }
    // If the control flag is clear, the linear counter reload flag is cleared.
    if (!_triangle.lengthCounterHalt) {
        _triangle.linearCounterReload = false;
    }
}

void APU::halfFrameClock() {
    if (_State.bits.P1) {
        if (!_pulse1.lengthCounterHalt && _pulse1.lengthCounter) {
            _pulse1.lengthCounter--;
        }
    } else {
        _pulse1.lengthCounter = 0;
    }

    if (_State.bits.P2) {
        if (!_pulse2.lengthCounterHalt && _pulse2.lengthCounter) {
            _pulse2.lengthCounter--;
        }
    } else {
        _pulse2.lengthCounter = 0;
    }

    if (_State.bits.T) {
        if (!_triangle.lengthCounterHalt && _triangle.lengthCounter) {
            _triangle.lengthCounter--;
        }
    } else {
        _triangle.lengthCounter = 0;
    }

    if (_State.bits.N) {
        if (!_noise.lengthCounterHalt && _noise.lengthCounter) {
            _noise.lengthCounter--;
        }
    } else {
        _noise.lengthCounter = 0;
    }

    _pulse1.sweep.clock(true, _pulse1.timer);
    _pulse2.sweep.clock(false, _pulse2.timer);
}

}  // namespace NebulaEmu