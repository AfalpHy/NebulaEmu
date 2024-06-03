#include <SDL2/SDL.h>
#include <getopt.h>

#include <chrono>
#include <iostream>

#include "CPU.h"
#include "Cartridge.h"
#include "Controller.h"
#include "PPU.h"

using namespace std;

namespace NebulaEmu {

const int SCREEN_WIDTH = 256 * 3;
const int SCREEN_HEIGHT = 240 * 3;

uint32_t* pixels = nullptr;

Cartridge* cartridge = nullptr;
CPU* cpu = nullptr;
PPU* ppu = nullptr;
Controller* controller = nullptr;

void init() {
    cartridge = new Cartridge();
    cpu = new CPU();
    ppu = new PPU();
    controller = new Controller();
    pixels = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
}

void run(string path) {
    cartridge->load(path);
    cpu->reset();
    ppu->reset();

    auto past = chrono::high_resolution_clock::now();
    chrono::high_resolution_clock::duration m_elapsedTime;
    chrono::nanoseconds cycleDuration(559);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

    SDL_Window* window =
        SDL_CreateWindow("NebulaEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_GameController* GameController = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            GameController = SDL_GameControllerOpen(i);
            if (GameController) {
                cout << "Opened controller " << SDL_GameControllerName(GameController) << endl;
                break;
            } else {
                cerr << "Could not open gamecontroller " << i << ": " << SDL_GetError() << endl;
            }
        }
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        auto now = chrono::high_resolution_clock::now();
        m_elapsedTime = now - past;
        past = now;
        while (m_elapsedTime > cycleDuration && !quit) {
            cpu->step();

            ppu->step();
            ppu->step();
            ppu->step();

            m_elapsedTime -= cycleDuration;
        }
        SDL_UpdateTexture(texture, nullptr, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_j:
                        controller->set(A);
                        break;
                    case SDLK_k:
                        controller->set(B);
                        break;
                    case SDLK_SPACE:
                        controller->set(Select);
                        break;
                    case SDLK_RETURN:
                        controller->set(Start);
                        break;
                    case SDLK_w:
                        controller->set(Up);
                        break;
                    case SDLK_s:
                        controller->set(Down);
                        break;
                    case SDLK_a:
                        controller->set(Left);
                        break;
                    case SDLK_d:
                        controller->set(Right);
                        break;
                    default:
                        cout << "Some other key pressed." << endl;
                        break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_j:
                        controller->clear(A);
                        break;
                    case SDLK_k:
                        controller->clear(B);
                        break;
                    case SDLK_SPACE:
                        controller->clear(Select);
                        break;
                    case SDLK_RETURN:
                        controller->clear(Start);
                        break;
                    case SDLK_w:
                        controller->clear(Up);
                        break;
                    case SDLK_s:
                        controller->clear(Down);
                        break;
                    case SDLK_a:
                        controller->clear(Left);
                        break;
                    case SDLK_d:
                        controller->clear(Right);
                        break;
                    default:
                        cout << "Some other key released." << endl;
                        break;
                }
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (e.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_A:
                        controller->set(A);
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        controller->set(B);
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK:
                        controller->set(Select);
                        break;
                    case SDL_CONTROLLER_BUTTON_START:
                        controller->set(Start);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        controller->set(Up);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        controller->set(Down);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        controller->set(Left);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        controller->set(Right);
                        break;
                    default:
                        cout << "Some other button pressed." << endl;
                        break;
                }
            } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                switch (e.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_A:
                        controller->clear(A);
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        controller->clear(B);
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK:
                        controller->clear(Select);
                        break;
                    case SDL_CONTROLLER_BUTTON_START:
                        controller->clear(Start);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        controller->clear(Up);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        controller->clear(Down);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        controller->clear(Left);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        controller->clear(Right);
                        break;
                    default:
                        cout << "Some other button released." << endl;
                        break;
                }
            } else if (e.type == SDL_CONTROLLERAXISMOTION) {
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    if (e.caxis.value < -8000) {
                        controller->set(Left);
                    } else if (e.caxis.value > 8000) {
                        controller->set(Right);
                    } else {
                        controller->clear(Left);
                        controller->clear(Right);
                    }
                } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (e.caxis.value < -8000) {
                        controller->set(Down);
                    } else if (e.caxis.value > 8000) {
                        controller->set(Up);
                    } else {
                        controller->clear(Up);
                        controller->clear(Down);
                    }
                }
            }
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

}  // namespace NebulaEmu

int main(int argc, char** argv) {
    string path;
    const struct option table[] = {
        {"help", no_argument, NULL, 'h'},
        {0, 0, NULL, 0},
    };

    auto displayHelpMessage = [&]() {
        printf("Usage: %s [OPTION...] path\n\n", argv[0]);
        printf("\t-h,--help\tDisplay available options\n");
        printf("\n");
    };

    if (argc == 1) {
        displayHelpMessage();
        exit(0);
    }
    int opt;
    while ((opt = getopt_long(argc, argv, "-h", table, NULL)) != -1) {
        switch (opt) {
            case 1:
                path = optarg;
                break;
            case 'h':
                displayHelpMessage();
                break;
            default:
                exit(-1);
        }
    }

    NebulaEmu::init();

    NebulaEmu::run(path);

    return 0;
}