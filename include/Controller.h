#pragma once

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>

#include <cstdint>

namespace NebulaEmu {

class Controller {
public:
    uint8_t readJoyStick1Data();
    uint8_t readJoyStick2Data();

    void strobe(uint8_t b);

    void update(SDL_Event& e);

    enum Button { A = 1, B = 2, Select = 4, Start = 8, Up = 16, Down = 32, Left = 64, Right = 128 };

private:
    // joystick 1
    SDL_GameController* _controller1 = nullptr;
    uint8_t _state1 = 0;
    uint8_t _shift1 = 0;
    // joystick 2
    SDL_GameController* _controller2 = nullptr;
    uint8_t _state2 = 0;
    uint8_t _shift2 = 0;

    bool _strobe;
};

}  // namespace NebulaEmu