#include "Controller.h"

#include <iostream>
namespace NebulaEmu {

// While S (strobe) is high, the shift registers in the controllers are continuously reloaded from the button
// states, and reading $4016/$4017 will keep returning the current state of the first button (A).
// Once S goes low, this reloading will stop.

// In the NES and Famicom, the top three (or five) bits are not driven, and so retain the bits of the previous byte
// on the bus. Usually this is the most significant byte of the address of the controller port—0x40. Certain games
// (such as Paperboy) rely on this behavior and require that reads from the controller ports return exactly $40 or
// $41 as appropriate
uint8_t Controller::readJoyStick1Data() {
    uint8_t ret;
    if (_strobe) {
        ret = _state1 & 1;
    } else {
        ret = (_shift1 & 1);
        _shift1 >>= 1;
    }

    return ret | 0x40;
}

uint8_t Controller::readJoyStick2Data() {
    uint8_t ret;
    if (_strobe) {
        ret = _state2 & 1;
    } else {
        ret = (_shift2 & 1);
        _shift2 >>= 1;
    }

    return ret | 0x40;
}

void Controller::strobe(uint8_t b) {
    _strobe = (b & 1);
    if (_strobe) {
        _shift1 = _state1;
        _shift2 = _state2;
    }
}

void Controller::update(SDL_Event &e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_j:
                _state1 |= A;
                break;
            case SDLK_k:
                _state1 |= B;
                break;
            case SDLK_SPACE:
                _state1 |= Select;
                break;
            case SDLK_RETURN:
                _state1 |= Start;
                break;
            case SDLK_w:
                _state1 |= Up;
                break;
            case SDLK_s:
                _state1 |= Down;
                break;
            case SDLK_a:
                _state1 |= Left;
                break;
            case SDLK_d:
                _state1 |= Right;
                break;
            default:
                break;
        }
    } else if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
            case SDLK_j:
                _state1 &= ~A;
                break;
            case SDLK_k:
                _state1 &= ~B;
                break;
            case SDLK_SPACE:
                _state1 &= ~Select;
                break;
            case SDLK_RETURN:
                _state1 &= ~Start;
                break;
            case SDLK_w:
                _state1 &= ~Up;
                break;
            case SDLK_s:
                _state1 &= ~Down;
                break;
            case SDLK_a:
                _state1 &= ~Left;
                break;
            case SDLK_d:
                _state1 &= ~Right;
                break;
            default:
                break;
        }
    } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        if (e.cbutton.which == 0) {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    _state1 |= A;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    _state1 |= B;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    _state1 |= Select;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    _state1 |= Start;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    _state1 |= Up;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    _state1 |= Down;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    _state1 |= Left;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    _state1 |= Right;
                    break;
                default:
                    break;
            }
        } else {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    _state2 |= A;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    _state2 |= B;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    _state2 |= Select;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    _state2 |= Start;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    _state2 |= Up;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    _state2 |= Down;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    _state2 |= Left;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    _state2 |= Right;
                    break;
                default:
                    break;
            }
        }
    } else if (e.type == SDL_CONTROLLERBUTTONUP) {
        if (e.cbutton.which == 0) {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    _state1 &= ~A;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    _state1 &= ~B;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    _state1 &= ~Select;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    _state1 &= ~Start;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    _state1 &= ~Up;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    _state1 &= ~Down;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    _state1 &= ~Left;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    _state1 &= ~Right;
                    break;
                default:
                    break;
            }
        } else {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    _state2 &= ~A;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    _state2 &= ~B;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    _state2 &= ~Select;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    _state2 &= ~Start;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    _state2 &= ~Up;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    _state2 &= ~Down;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    _state2 &= ~Left;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    _state2 &= ~Right;
                    break;
                default:
                    break;
            }
        }
    } else if (e.type == SDL_CONTROLLERAXISMOTION) {
        if (e.caxis.which == 0) {
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                if (e.caxis.value < -8000) {
                    _state1 |= Left;
                } else if (e.caxis.value > 8000) {
                    _state1 |= Right;
                } else {
                    _state1 &= ~Left;
                    _state1 &= ~Right;
                }
            } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                if (e.caxis.value < -8000) {
                    _state1 |= Up;
                } else if (e.caxis.value > 8000) {
                    _state1 |= Down;
                } else {
                    _state1 &= ~Up;
                    _state1 &= ~Down;
                }
            }
        } else {
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                if (e.caxis.value < -8000) {
                    _state2 |= Left;
                } else if (e.caxis.value > 8000) {
                    _state2 |= Right;
                } else {
                    _state2 &= ~Left;
                    _state2 &= ~Right;
                }
            } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                if (e.caxis.value < -8000) {
                    _state2 |= Up;
                } else if (e.caxis.value > 8000) {
                    _state2 |= Down;
                } else {
                    _state2 &= ~Up;
                    _state2 &= ~Down;
                }
            }
        }
    }
}

}  // namespace NebulaEmu