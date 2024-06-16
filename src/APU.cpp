#include "APU.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "CPU.h"

namespace NebulaEmu {

extern CPU* cpu;

uint8_t lengthTable[] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                         12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

void APU::reset() {
    _M = false;
    _I = false;
    _buffer.resize(65536);
}

void APU::step() {
    _cycles++;

    _pulse1.sequencer.clock(_pulse1.timer);
    _pulse2.sequencer.clock(_pulse2.timer);

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

void APU::sample() { _buffer[_sampleIndex++ % _buffer.size()] = linearApproximationMix() * 255; }

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
    _triangle.linearCounterLoad = data & 0x7F;
}

void APU::writeTriangleReg2(uint8_t data) {
    _triangle.timer &= 0xFF00;
    _triangle.timer |= data;
}

void APU::writeTriangleReg3(uint8_t data) {
    _triangle.timer &= 0x00FF;
    _triangle.timer |= (data & 0x7) << 8;
}

void APU::writeNoiseReg0(uint8_t data) {
    _noise.lengthCounterHalt = (data >> 5) & 1;
    _noise.constantVolume = (data >> 4) & 1;
    _noise.volume = data & 0x0F;
}

void APU::writeNoiseReg2(uint8_t data) {
    _noise.loopNoise = data >> 7;
    _noise.noisePreiod = data & 0x0F;
}

void APU::writeNoiseReg3(uint8_t data) { _noise.lengthCounter = lengthTable[data >> 3]; }

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

uint8_t APU::calculatePulse(PulseChannel& pulse) {
    if (pulse.sweep.mute || pulse.lengthCounter == 0) {
        return 0;
    }
    return pulse.envelope.output * pulse.sequencer.output;
}

uint8_t APU::calculateTriangle() { return 0; }

uint8_t APU::calculateNoise() { return 0; }

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
    this->timer--;
    if (this->timer == 0) {
        this->timer = timer;
        output = sequence & 0x80;
        sequence = (sequence << 1) | (sequence >> 7);
    }
}

void APU::quarterFrameClock() {
    _pulse1.envelope.clock();
    _pulse2.envelope.clock();
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