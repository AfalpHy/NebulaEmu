#include "Emulator.h"

#include <SDL2/SDL.h>

using namespace std;

namespace NebulaEmu {

void Emulator::run(string path) {
    _cartridge.load(path);
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