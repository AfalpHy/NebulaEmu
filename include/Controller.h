#pragma once

#include <cstdint>

namespace NebulaEmu {

enum Button { A, B, Select, Start, Up, Down, Left, Right };

class Controller {
public:
    uint8_t read();

    void strobe(uint8_t b);

    void set(Button button);

    void clear(Button button);

private:
    uint8_t _buffer = 0;
    uint8_t _state = 0;
    bool _strobe;
};

}  // namespace NebulaEmu