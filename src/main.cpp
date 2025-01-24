// rtsine.cpp STK tutorial program

#include "stk/SineWave.h"
#include "stk/RtWvOut.h"

#include "SDL2/SDL.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "voice.h"

#include <cstdlib>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

std::atomic<bool> running(true); // flag that is shared across threads

using namespace stk;

int guiThread(Voice* voices[], int nVoices) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Synthesizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, // x and y position
        800, 600,                                       // width and height
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_QUIT;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_QUIT;
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // r, g, b, alpha

    SDL_Event event;

    // SDL main loop
    while (running.load()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running.store(false);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.motion.x;
                for (int i = 0; i < nVoices; ++i) {
                   voices[i]->setFrequency(static_cast<double>(mouseX));
                }
            }
        }
        // fill window with selected color
        SDL_RenderClear(renderer);
        // update window
        SDL_RenderPresent(renderer);
        // wait 16ms for approx 60 fps
        SDL_Delay(16);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void audioThread(Voice* voices[], int nVoices) {
    Stk::setSampleRate( 44100.0 );
    Stk::showWarnings( true );

    RtWvOut *dac = 0;

    try {
        dac = new RtWvOut( 1 );
    }
    catch ( StkError & ) {
        exit( 1 );
    }

    for (int i = 0; i < nVoices; ++i) {
        voices[i]->setFrequency(110.0 * (i+1));
    }

    while (running.load()) {
        try {
            double outputSample = 0;
            for (int i = 0; i < nVoices; ++i) {
                outputSample += voices[i]->tick();
            }
            dac->tick(outputSample / nVoices);
        }
        catch ( StkError & ) {
            break;
        }
    }

    delete dac;

}

int main()
{
    Voice *voice1 = new Voice;
    // Voice *voice2 = new Voice;
    // Voice *voice3 = new Voice;
    // Voice *voice4 = new Voice;

    // Voice* voices[] = {voice1, voice2, voice3, voice4};
    Voice* voices[] = {voice1};
    std::thread t1(audioThread, voices, sizeof(voices)/sizeof(voices[0]));
    std::thread t2(guiThread, voices, sizeof(voices)/sizeof(voices[0]));

    t2.join();
    running.store(false);
    t1.join();

    delete voice1;
    // delete voice2;
    // delete voice3;
    // delete voice4;

    return 0;
}

/*

             Midi thread
                  |
                  |
GUI thread ---- Voices
                  |
                  |
            Audio thread


*/