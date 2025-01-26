#include "stk/SineWave.h"
#include "stk/RtWvOut.h"

#include "SDL2/SDL.h"
#include "GL/glew.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui-knobs/imgui-knobs.h"
#include "toggleSwitch.h"

#include "voice.h"
#include "colors.h"

#include <cstdlib>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>


const double SAMPLERATE = 48000.0;
const int BUFFER_FRAMES = 64;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

std::atomic<bool> running(true); // flag that is shared across threads

using namespace stk;

void inline setRenderColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

struct SDLContext {
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    ~SDLContext() {
        if (glContext) SDL_GL_DeleteContext(glContext);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

bool initializeSDL(SDLContext& sdlContext) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    sdlContext.window = SDL_CreateWindow("Synthesizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!sdlContext.window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    sdlContext.glContext = SDL_GL_CreateContext(sdlContext.window);
    SDL_GL_MakeCurrent(sdlContext.window, sdlContext.glContext);
    SDL_GL_SetSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    return true;
}

void initializeImGui(SDLContext& sdlContext) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(sdlContext.window, sdlContext.glContext);
    ImGui_ImplOpenGL3_Init("#version 330");
}

template <typename UpdateFunc>
void createKnob(const char* label, float* value, float min, float max, float step, const char* format,
                UpdateFunc updateFunc) {
    if (ImGuiKnobs::Knob(label, value, min, max, step, format, ImGuiKnobVariant_Tick)) {
        updateFunc(*value);
    }
}

struct adsrParameters {
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
};

void renderAEG(Voice* voices[], int nVoices) {
    static adsrParameters aeg = {0.001, 1.0, 1.0, 1.0};
    ImGui::Begin("AEG", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    createKnob("Attack", &aeg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegAttack(v);
    });
    createKnob("Decay", &aeg.decayTime, 0.03f, 2.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegDecay(v);
    });
    createKnob("Sustain", &aeg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegSustain(v);
    });
    createKnob("Release", &aeg.releaseTime, 0.001f, 3.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegRelease(v);
    });
    ImGui::End();
}

void renderFEG(Voice* voices[], int nVoices) {
    static adsrParameters feg = {0.001, 1.0, 1.0, 1.0};
    ImGui::Begin("FEG", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    createKnob("Attack", &feg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegAttack(v);
    });
    createKnob("Decay", &feg.decayTime, 0.03f, 2.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegDecay(v);
    });
    createKnob("Sustain", &feg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegSustain(v);
    });
    createKnob("Release", &feg.releaseTime, 0.001f, 3.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegRelease(v);
    });
    ImGui::End();
}

void renderOscSection(Voice* voices[], int nVoices) {
    static float osc1detune = 1.0f;
    static float osc2detune = 1.0f;
    static bool toggle_value1 = false;
    static bool toggle_value2 = false;
    ImGui::Begin("Osc", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    createKnob("Osc1 tune", &osc1detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscDetune(1, v);
    });
    createKnob("Osc2 tune", &osc2detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscDetune(2, v);
    });
    if (ToggleSwitch("Toggle1", &toggle_value1))
        {
            for (int i = 0; i < nVoices; ++i) {
                voices[i]->toggleOscWaveform(1, toggle_value1);
            }
        }
    if (ToggleSwitch("Toggle2", &toggle_value2))
        {
            for (int i = 0; i < nVoices; ++i) {
                voices[i]->toggleOscWaveform(2, toggle_value2);
            }
        }
    ImGui::End();
}

void renderFilterSection(Voice* voices[], int nVoices) {
    static float cutoff = 1000.0f;
    static float resonance = 0.0f;
    static float fegAmount = 0.0f;
    ImGui::Begin("Filter", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    createKnob("Cutoff", &cutoff, 0.1f, 1000.0f, 0.1f, "%.1f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setCutoff(v);
    });
    createKnob("Resonance", &resonance, 0.1f, 10.0f, 0.001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setResonance(v);
    });
    createKnob("FEG Amount", &fegAmount, 0.0f, 1000.0f, 0.1f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegAmount(fegAmount);
    });
    ImGui::End();
}


int guiThread(Voice* voices[], int nVoices) {
    SDLContext sdlContext;
    if (!initializeSDL(sdlContext)) return 1;
    initializeImGui(sdlContext);

    SDL_Event event;
    // imgui main loop
    while (running.load()) {
        // Poll SDL events
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running.store(false);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.motion.x;
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFrequency(static_cast<double>(mouseX));
                    voices[i]->noteOn();
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->noteOff();
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render UI elements
        renderAEG(voices, nVoices);
        renderFEG(voices, nVoices);
        renderOscSection(voices, nVoices);
        renderFilterSection(voices, nVoices);

        // Rendering
        ImGui::Render();
        // glViewport(0, 0, 1280, 720);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(sdlContext.window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(sdlContext.glContext);
    SDL_DestroyWindow(sdlContext.window);
    SDL_Quit();

    return 0;
}

void audioThread(Voice* voices[], int nVoices) {
    Stk::setSampleRate(SAMPLERATE);
    RtWvOut *dac = 0;

    try {
        dac = new RtWvOut( 2, SAMPLERATE, 0, BUFFER_FRAMES );
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
                outputSample += voices[i]->tick() / nVoices;
            }
            dac->tick(outputSample);
        }
        catch ( StkError & ) {
            break;
        }
    }

    delete dac;

}

int main()
{
    Voice* voice1 = new Voice(SAMPLERATE);
    Voice* voices[] = {voice1};

    std::thread audio(audioThread, voices, sizeof(voices)/sizeof(voices[0]));
    std::thread gui(guiThread, voices, sizeof(voices)/sizeof(voices[0]));

    gui.join();
    running.store(false);
    audio.join();

    for (Voice* voice : voices) {
        delete voice;
    }

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