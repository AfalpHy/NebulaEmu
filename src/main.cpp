#include <SDL2/SDL.h>
#include <getopt.h>

#include <iostream>

#include "CPU.h"
#include "Cartridge.h"
#include "PPU.h"

using namespace std;

namespace NebulaEmu {

const int SCREEN_WIDTH = 256 * 3;
const int SCREEN_HEIGHT = 240 * 3;

uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

Cartridge* cartridge = nullptr;
CPU* cpu = nullptr;
PPU* ppu = nullptr;

void init() {
    cartridge = new Cartridge();
    cpu = new CPU();
    ppu = new PPU();
}

void run(string path) {
    cartridge->load(path);
    cpu->reset();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window =
        SDL_CreateWindow("NebulaEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        cout << "Up arrow key pressed." << endl;
                        break;
                    case SDLK_DOWN:
                        cout << "Down arrow key pressed." << endl;
                        break;
                    case SDLK_LEFT:
                        cout << "Left arrow key pressed." << endl;
                        break;
                    case SDLK_RIGHT:
                        cout << "Right arrow key pressed." << endl;
                        break;
                    default:
                        cout << "Some other key pressed." << endl;
                        break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        cout << "Up arrow key released." << endl;
                        break;
                    case SDLK_DOWN:
                        cout << "Down arrow key released." << endl;
                        break;
                    case SDLK_LEFT:
                        cout << "Left arrow key released." << endl;
                        break;
                    case SDLK_RIGHT:
                        cout << "Right arrow key released." << endl;
                        break;
                    default:
                        cout << "Some other key released." << endl;
                        break;
                }
            }
            SDL_UpdateTexture(texture, nullptr, pixels, SCREEN_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
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