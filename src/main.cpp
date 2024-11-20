#include <SDL2/SDL.h>
#include <getopt.h>

#include <chrono>
#include <iostream>

#include "APU.h"
#include "CPU.h"
#include "Cartridge.h"
#include "Controller.h"
#include "PPU.h"

using namespace std;

namespace NebulaEmu {

uint32_t scale = 3;

uint32_t* pixels = nullptr;

Cartridge* cartridge = nullptr;
APU* apu = nullptr;
CPU* cpu = nullptr;
PPU* ppu = nullptr;
Controller* controller = nullptr;

void init() {
    cartridge = new Cartridge();
    apu = new APU();
    cpu = new CPU();
    ppu = new PPU();
    controller = new Controller();
    pixels = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
}

void audioCallback(void* userdata, uint8_t* stream, int len) {
    (void)userdata;
    static uint64_t index = 0;
    if (apu->getSampleIndex() <= index + len) {
        SDL_memset(stream, 128, len);
        return;
    }

    // synchronize
    if (apu->getSampleIndex() - index >= apu->getBuffer().size()) {
        index += apu->getBuffer().size();
        return;
    }

    SDL_memcpy(stream, apu->getBuffer().data() + index % apu->getBuffer().size(), len);
    index += len;
}

void run(string path) {
    cartridge->load(path);
    apu->reset();
    cpu->reset();
    ppu->reset();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

    SDL_Window* window = SDL_CreateWindow("NebulaEmu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_U8;
    spec.channels = 1;
    spec.samples = 1024;
    spec.callback = audioCallback;

    if (SDL_OpenAudio(&spec, NULL) < 0) {
        cerr << "Could not open audio" << SDL_GetError() << endl;
    }

    // audio play begin
    SDL_PauseAudio(0);

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        SDL_GameController* GameController = SDL_GameControllerOpen(i);
        if (GameController) {
            cout << "Opened controller " << SDL_GameControllerName(GameController) << endl;
        } else {
            cerr << "Could not open gamecontroller " << i << ": " << SDL_GetError() << endl;
        }
    }

    auto past = chrono::high_resolution_clock::now();
    chrono::high_resolution_clock::duration elapsedTime(0);
    // The NES master clock is 21.47727 MHz (NTSC).
    // The CPU operates at approximately 1.789772 MHz (master clock divided by 12).
    // The PPU operates at approximately 5.369318 MHz (master clock divided by 4).
    // The CPU completes one cycle in 1/1.789772 MHz = 559ns
    // The sequencer is clocked on every other CPU cycle, so 2 CPU cycles = 1 APU cycle
    chrono::nanoseconds cycleDuration(559 * 2);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        auto now = chrono::high_resolution_clock::now();
        elapsedTime += now - past;
        past = now;
        while (elapsedTime > cycleDuration) {
            apu->step();

            cpu->step();
            cpu->step();

            ppu->step();
            ppu->step();
            ppu->step();
            ppu->step();
            ppu->step();
            ppu->step();

            elapsedTime -= cycleDuration;
        }
        SDL_UpdateTexture(texture, nullptr, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            } else {
                controller->update(e);
            }
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_CloseAudio();

    SDL_Quit();
}

}  // namespace NebulaEmu


// avoid using WinMain in SDL2 on Windows (Defining main as SDL_main in SDL causes the use of WinMain)
#ifdef _WIN32

#undef main

#endif

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
        return 0;
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
                return 1;
        }
    }

    NebulaEmu::init();

    NebulaEmu::run(path);

    return 0;
}