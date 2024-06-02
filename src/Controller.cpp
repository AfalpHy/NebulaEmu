#include "Controller.h"

#include <iostream>
namespace NebulaEmu {

uint8_t Controller::read() {
    uint8_t ret;
    if (_strobe) {
        ret = _buffer & 1;
    } else {
        ret = (_state & 1);
        _state >>= 1;
    }

    return ret | 0x40;
}

void Controller::strobe(uint8_t b) {
    _strobe = (b & 1);
    if (!_strobe) {
        _state = _buffer;
    }
}

void Controller::set(Button button) { _buffer |= (1 << button); }

void Controller::clear(Button button) { _buffer &= ~(1 << button); }

}  // namespace NebulaEmu