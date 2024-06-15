#include "APU.h"

#include <algorithm>
#include <iostream>

#include "CPU.h"

namespace NebulaEmu {

extern CPU* cpu;

uint8_t lengthTable[] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
                         12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

void APU::reset() {
    _M = false;
    _I = false;
}

void APU::step() {
    _cycles++;
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
    } else if (_cycles == 186401) {
        quarterFrameClock();
        halfFrameClock();
    }
}

uint8_t APU::sample() {
    _sampleIndex++;
    return linearApproximationMix() * 255;
}

uint8_t APU::readStatus() {
    uint8_t ret = (_noise.lengthCounter > 0) << 3 | (_triangle.lengthCounter > 0) << 2 |
                  (_pulse2.lengthCounter > 0) << 1 | (_pulse1.lengthCounter > 0);
    return ret;
}

void APU::writePulse0(bool one, uint8_t data) {
    if (one) {
        _pulse1.duty = data >> 6;
        _pulse1.envelopeLoop = _pulse1.lengthCounterHalt = (data >> 5) & 1;
        _pulse1.constantVolume = (data >> 4) & 1;
        _pulse1.volume = data & 0x0F;
    } else {
        _pulse2.duty = data >> 6;
        _pulse2.envelopeLoop = _pulse2.lengthCounterHalt = (data >> 5) & 1;
        _pulse2.constantVolume = (data >> 4) & 1;
        _pulse2.volume = data & 0x0F;
    }
}

void APU::writePulse1(bool one, uint8_t data) {
    if (one) {
        _pulse1.sweepUnit.value = data;
        // side effect
        _pulse1.sweepReload = true;
    } else {
        _pulse2.sweepUnit.value = data;
        // side effect
        _pulse2.sweepReload = true;
    }
}

void APU::writePulse2(bool one, uint8_t data) {
    if (one) {
        _pulse1.timer &= 0xFF00;
        _pulse1.timer |= data;
    } else {
        _pulse2.timer &= 0xFF00;
        _pulse2.timer |= data;
    }
}

void APU::writePulse3(bool one, uint8_t data) {
    if (one) {
        _pulse1.timer &= 0x00FF;
        _pulse1.timer |= (data & 0x7) << 8;
        _pulse1.lengthCounter = lengthTable[data >> 3];

        // side effect
        _pulse1.envelopStart = true;
    } else {
        _pulse2.timer &= 0x00FF;
        _pulse2.timer |= (data & 0x7) << 8;
        _pulse2.lengthCounter = lengthTable[data >> 3];

        // side effect
        _pulse2.envelopStart = true;
    }
}

void APU::writeTriangle0(uint8_t data) {
    _triangle.lengthCounterHalt = data >> 7;
    _triangle.linearCounterLoad = data & 0x7F;
}

void APU::writeTriangle2(uint8_t data) {
    _triangle.timer &= 0xFF00;
    _triangle.timer |= data;
}

void APU::writeTriangle3(uint8_t data) {
    _triangle.timer &= 0x00FF;
    _triangle.timer |= (data & 0x7) << 8;
}

void APU::writeNoise0(uint8_t data) {
    _noise.lengthCounterHalt = (data >> 5) & 1;
    _noise.constantVolume = (data >> 4) & 1;
    _noise.volume = data & 0x0F;
}

void APU::writeNoise2(uint8_t data) {
    _noise.loopNoise = data >> 7;
    _noise.noisePreiod = data & 0x0F;
}

void APU::writeNoise3(uint8_t data) { _noise.lengthCounter = lengthTable[data >> 3]; }

void APU::writeDMC0(uint8_t data) {
    _DMC.IRQenable = data >> 7;
    _DMC.loop = (data >> 6) & 1;
    _DMC.frequency = data & 0x0F;
}

void APU::writeDMC1(uint8_t data) { _DMC.loadCounter = data & 0x7F; }

void APU::writeDMC2(uint8_t data) { _DMC.sampleAddress = data; }

void APU::writeDMC3(uint8_t data) { _DMC.sampleLength = data; }

void APU::writeStatus(uint8_t data) { _State.value = data & 0x1F; }

void APU::writeFrameCounter(uint8_t data) {
    (void)data;
    // _M = data >> 7;
    // _I = (data >> 6) & 1;
    // _cycles = 0;
}

float APU::linearApproximationMix() {
    float pulseOut = 0.00752 * (calculatePulse(_pulse1) + calculatePulse(_pulse2));
    float tndOut = 0.00851 * calculateTriangle() + 0.00494 * calculateNoise() + 0.00335 * calculateDMC();
    return pulseOut + tndOut;
}

uint8_t APU::calculatePulse(PulseChannel& pulse) { return pulse.output; }

uint8_t APU::calculateTriangle() { return 0; }

uint8_t APU::calculateNoise() { return 0; }

uint8_t APU::calculateDMC() { return 0; }

void APU::quarterFrameClock() {
    if (_pulse1.envelopStart) {
        // if the start flag is clear, the divider is clocked, otherwise the start flag is cleared
        _pulse1.envelopStart = false;
        _pulse1.envelopDivider = _pulse1.volume;
        _pulse1.decayLevel = 15;
    } else {
        if (_pulse1.envelopDivider == 0) {
            _pulse1.envelopDivider = _pulse1.volume;
            if (_pulse1.decayLevel != 0) {
                _pulse1.decayLevel--;
            } else {
                if (_pulse1.envelopeLoop) {
                    _pulse1.decayLevel = 15;
                }
            }
        } else {
            _pulse1.envelopDivider--;
        }
    }
    if (_pulse1.constantVolume) {
        _pulse1.output = _pulse1.volume;
    } else {
        _pulse1.output = _pulse1.decayLevel;
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

    if (_State.bits.N) {
        if (!_noise.lengthCounterHalt && _noise.lengthCounter) {
            _noise.lengthCounter--;
        }
    } else {
        _noise.lengthCounter = 0;
    }
}

}  // namespace NebulaEmu