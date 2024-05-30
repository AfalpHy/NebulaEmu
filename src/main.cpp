#include <SDL2/SDL.h>
#include <getopt.h>

#include <iostream>

#include "CPU.h"
#include "Cartridge.h"
#include "PPU.h"

using namespace std;

namespace NebulaEmu {

Cartridge* cartridge = nullptr;
CPU* cpu = nullptr;
PPU* ppu = nullptr;

void init() {
    cartridge = new Cartridge();
    cpu = new CPU();
    ppu = new PPU();
}

void run(std::string path) {
    cartridge->load(path);
    cpu->reset();

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("NebulaEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000);
    SDL_DestroyRenderer(renderer);
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