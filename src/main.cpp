#include "stk/SineWave.h"
#include "stk/RtWvOut.h"

#include "SDL2/SDL.h"
#include "GL/glew.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui-knobs/imgui-knobs.h"
#include "toggleSwitch.h"
#include "imageLoading.h"

#include "voice.h"
#include "voiceAllocator.h"
#include "colors.h"
#include "midiReader.h"

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

const float KNOBSIZE = 40.0f;

template <typename UpdateFunc>
void createKnob(const char* label, float* value, float min, float max, float step, const char* format,
                UpdateFunc updateFunc) {
    if (ImGuiKnobs::Knob(label, value, min, max, step, "", ImGuiKnobVariant_Wiper, KNOBSIZE, ImGuiKnobFlags_NoInput | ImGuiKnobFlags_NoTitle)) {
        updateFunc(*value);
    }
}

struct adsrParameters {
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
};

void renderUi(Voice* voices[], int nVoices) {

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(800,600));
    ImGui::Begin("UI", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    // OSC SECTION
    static float osc1detune = 1.0f;
    static float osc2detune = 1.0f;
    static float osc1volume = 1.0f;
    static float osc2volume = 1.0f;
    static float xModAmount = 0.0f;
    static bool toggle_value1 = false;
    static bool toggle_value2 = false;
    ImGui::SetCursorPos(ImVec2(70, 108));
    createKnob("Osc1 tune", &osc1detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscDetune(1, v);
    });
    ImGui::SetCursorPos(ImVec2(71, 162));
    createKnob("Osc2 tune", &osc2detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscDetune(2, v);
    });
    ImGui::SetCursorPos(ImVec2(173, 127));
    if (ToggleSwitch("Toggle1", &toggle_value1)) {
        for (int i = 0; i < nVoices; ++i) {
            voices[i]->toggleOscWaveform(1, toggle_value1);
        }
    }
    ImGui::SetCursorPos(ImVec2(173, 182));
    if (ToggleSwitch("Toggle2", &toggle_value2)) {
        for (int i = 0; i < nVoices; ++i) {
            voices[i]->toggleOscWaveform(2, toggle_value2);
        }
    }
    ImGui::SetCursorPos(ImVec2(283, 109));
    createKnob("Osc1 volume", &osc1volume, 0.0f, 1.0f, 0.001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscVolume(1, v);
    });
    ImGui::SetCursorPos(ImVec2(282, 161));
    createKnob("Osc2 volume", &osc2volume, 0.0f, 1.0f, 0.001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscVolume(2, v);
    });
    ImGui::SetCursorPos(ImVec2(282, 161));
    createKnob("xmod amount", &xModAmount, 0.0f, 1.0f, 0.001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setOscVolume(2, v);
    });
    // FILTER SECTION
    static float cutoff = 1000.0f;
    static float resonance = 0.0f;
    static float fegAmount = 0.0f;
    ImGui::SetCursorPos(ImVec2(447, 128));
    createKnob("Cutoff", &cutoff, 0.1f, 1000.0f, 0.1f, "%.1f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setCutoff(v);
    });
    ImGui::SetCursorPos(ImVec2(525, 125));
    createKnob("Resonance", &resonance, 0.1f, 10.0f, 0.001f, "%.3f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setResonance(v);
    });
    ImGui::SetCursorPos(ImVec2(598, 118));
    createKnob("FEG Amount", &fegAmount, 0.0f, 1000.0f, 0.1f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegAmount(fegAmount);
    });

    // AEG
    static adsrParameters aeg = {0.001, 1.0, 1.0, 1.0};
    ImGui::SetCursorPos(ImVec2(123, 303));
    createKnob("aegAttack", &aeg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegAttack(v);
    });
    ImGui::SetCursorPos(ImVec2(182, 299));
    createKnob("aegDecay", &aeg.decayTime, 0.03f, 2.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegDecay(v);
    });
    ImGui::SetCursorPos(ImVec2(237, 291));
    createKnob("aegSustain", &aeg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegSustain(v);
    });
    ImGui::SetCursorPos(ImVec2(294, 297));
    createKnob("aegRelease", &aeg.releaseTime, 0.001f, 3.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setAegRelease(v);
    });

    // FEG
    static adsrParameters feg = {0.001, 1.0, 1.0, 1.0};
    ImGui::SetCursorPos(ImVec2(124, 372));
    createKnob("fegAttack", &feg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegAttack(v);
    });
    ImGui::SetCursorPos(ImVec2(184, 372));
    createKnob("fegDecay", &feg.decayTime, 0.03f, 2.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegDecay(v);
    });
    ImGui::SetCursorPos(ImVec2(240, 362));
    createKnob("fegSustain", &feg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegSustain(v);
    });
    ImGui::SetCursorPos(ImVec2(291, 364));
    createKnob("fegRelease", &feg.releaseTime, 0.001f, 3.0f, 0.001f, "%.3fs", [&](float v) {
        for (int i = 0; i < nVoices; ++i) voices[i]->setFegRelease(v);
    });

    ImGui::End();

}


int guiThread(Voice* voices[], int nVoices, voiceAllocator* allocator) {
    SDLContext sdlContext;
    if (!initializeSDL(sdlContext)) return 1;
    initializeImGui(sdlContext);

    int my_image_width = 0;
    int my_image_height = 0;
    GLuint my_image_texture = 0;
    bool ret = LoadTextureFromFile("../assets/ui_v0.jpg", &my_image_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    SDL_Event event;
    // imgui main loop
    while (running.load()) {
        // Poll SDL events
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running.store(false);
            }
            // else if (event.type == SDL_MOUSEBUTTONDOWN) {
            //     int mouseX = event.motion.x;
            //     // for (int i = 0; i < nVoices; ++i) {
            //         allocator->noteOn(static_cast<double>(mouseX));
            //         std::cout << event.motion.x << " " << event.motion.y << std::endl;
            //         // voices[i]->setFrequency(static_cast<double>(mouseX));
            //         // voices[i]->noteOn();
            //     // }
            // }
            // else if (event.type == SDL_MOUSEBUTTONUP) {
            //     int mouseX = event.motion.x;
            //     // for (int i = 0; i < nVoices; ++i) {
            //     allocator->noteOff(static_cast<double>(mouseX));
            //         // voices[i]->noteOff();
            //     // }
            // }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::Begin("OpenGL Texture Text", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::Image((ImTextureID)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
        ImGui::End();

        // Render UI elements
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        renderUi(voices, nVoices);
        // renderAEG(voices, nVoices);
        // renderFEG(voices, nVoices);
        // renderOscSection(voices, nVoices);
        // renderFilterSection(voices, nVoices);

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
    Voice* voice2 = new Voice(SAMPLERATE);
    Voice* voice3 = new Voice(SAMPLERATE);
    Voice* voice4 = new Voice(SAMPLERATE);
    Voice* voice5 = new Voice(SAMPLERATE);
    Voice* voice6 = new Voice(SAMPLERATE);
    Voice* voice7 = new Voice(SAMPLERATE);
    Voice* voice8 = new Voice(SAMPLERATE);
    Voice* voices[] = {voice1, voice2, voice3, voice4, voice5, voice6, voice7, voice8};
    voiceAllocator* allocator = new voiceAllocator(voices, sizeof(voices)/sizeof(voices[0]));
    MidiReader* reader = new MidiReader(allocator);

    std::thread audio(audioThread, voices, sizeof(voices)/sizeof(voices[0]));
    std::thread gui(guiThread, voices, sizeof(voices)/sizeof(voices[0]), allocator);

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