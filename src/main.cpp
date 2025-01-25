#include "stk/SineWave.h"
#include "stk/RtWvOut.h"

#include "SDL2/SDL.h"
#include "GL/glew.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui-knobs/imgui-knobs.h"
#include "voice.h"
#include "colors.h"
#include "synthParameters.h"

#include <cstdlib>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>


const double SAMPLERATE = 44100.0;
const int BUFFER_FRAMES = 64;

std::atomic<bool> running(true); // flag that is shared across threads

using namespace stk;

void inline setRenderColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

int guiThread(Voice* voices[], int nVoices) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

     // Setup SDL OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Synthesizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, // x and y position
        800, 600,                                       // width and height
        SDL_WINDOW_OPENGL
    );

    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // Set the UI style to dark

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

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

        // ImGui content
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(80, 480));
        ImGui::Begin("AEG", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
            static adsrParameters aeg = {0.001, 1.0, 1.0, 1.0};
            if (ImGuiKnobs::Knob("AEG\n\nAttack", &aeg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setAegAttack(aeg.attackTime);
                }
            }
            if (ImGuiKnobs::Knob("Decay", &aeg.decayTime, 0.03f, 2.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick, 0, ImGuiKnobFlags_Logarithmic)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setAegDecay(aeg.decayTime);
                }
            }
            if (ImGuiKnobs::Knob("Sustain", &aeg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setAegSustain(aeg.sustainLevel);
                }
            }
            if (ImGuiKnobs::Knob("Release", &aeg.releaseTime, 0.001f, 3.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick, 0, ImGuiKnobFlags_Logarithmic)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setAegRelease(aeg.releaseTime);
                }
            }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(100, 10));
        ImGui::SetNextWindowSize(ImVec2(80, 480));

        // ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // Transparent background

        ImGui::Begin("FEG", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
            static adsrParameters feg = {0.001, 1.0, 1.0, 1.0};
            if (ImGuiKnobs::Knob("FEG\n\nAttack", &feg.attackTime, 0.001f, 5.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFegAttack(feg.attackTime);
                }
            }
            if (ImGuiKnobs::Knob("Decay", &feg.decayTime, 0.001f, 5.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFegDecay(feg.decayTime);
                }
            }
            if (ImGuiKnobs::Knob("Sustain", &feg.sustainLevel, 0.0f, 1.0f, 0.001f, "%.2f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFegSustain(feg.sustainLevel);
                }
            }
            if (ImGuiKnobs::Knob("Release", &feg.releaseTime, 0.001f, 5.0f, 0.001f, "%.3fs", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFegRelease(feg.releaseTime);
                }
            }
        ImGui::End();

        // ImGui::PopStyleColor(); // Restore the previous style color


        ImGui::SetNextWindowPos(ImVec2(190, 10));
        ImGui::SetNextWindowSize(ImVec2(80, 480));
        ImGui::Begin("OSC", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
            static float osc1detune = 1.0f;
            static float osc2detune = 1.0f;
            if (ImGuiKnobs::Knob("Osc 1 tune", &osc1detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setOscDetune(1, osc1detune);
                }
            }
            if (ImGuiKnobs::Knob("Osc 2 tune", &osc2detune, 1.0/1.05946f, 1.05946f, 0.0001f, "%.3f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setOscDetune(2, osc2detune);
                }
            }
        ImGui::End();


        ImGui::SetNextWindowPos(ImVec2(280, 10));
        ImGui::SetNextWindowSize(ImVec2(80, 480));
        ImGui::Begin("Filter", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
            static float cutoff = 1000.0f;
            static float resonance = 0.0f;
            static float fegAmount = 0.0f;
            if (ImGuiKnobs::Knob("Cutoff", &cutoff, 0.1f, 1000.0f, 0.1f, "%.1f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setCutoff(cutoff);
                }
            }
            if (ImGuiKnobs::Knob("Resonance", &resonance, 0.1f, 10.0f, 0.001f, "%.3f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setResonance(resonance);
                }
            }
            if (ImGuiKnobs::Knob("FEG Amount", &fegAmount, 0.0f, 1000.0f, 0.1f, "%.2f", ImGuiKnobVariant_Tick)) {
                for (int i = 0; i < nVoices; ++i) {
                    voices[i]->setFegAmount(fegAmount);
                }
            }
        ImGui::End();



        // Rendering
        ImGui::Render();
        glViewport(0, 0, 1280, 720);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void audioThread(Voice* voices[], int nVoices) {
    Stk::setSampleRate( SAMPLERATE );

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
    Voice *voice1 = new Voice(SAMPLERATE);
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