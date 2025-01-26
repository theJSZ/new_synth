#include "voiceAllocator.h"
#include <stdio.h>

voiceAllocator::voiceAllocator(Voice* inputVoices[], int inputNVoices)
    : nVoices(inputNVoices) {
    for (int i = 0; i < nVoices && i < 8; ++i) {
        voices[i] = inputVoices[i];
    }
    for (int i = 0; i < 8; i++) {
        voiceInUse[i] = false;
    }
}

void voiceAllocator::noteOn(float frequency) {
    for (int i = nextVoice; i < nVoices + nextVoice && i < 8 + nextVoice; ++i) {
        std::cout << "i: " << i << std::endl;
        int j = i % nVoices;
        std::cout << "j: " << j << std::endl;

        if (!voiceInUse[j]) {
            // use this voice
            std::cout << "voice: " << j << std::endl;

            voices[j]->setFrequency(frequency);
            voices[j]->noteOn();
            voiceInUse[j] = true;
            notes[j] = frequency;
            nextVoice++;
            nextVoice %= nVoices;
            return;
        }
    }
}

void voiceAllocator::noteOff(float frequency) {
    for (int i = 0; i < nVoices && i < 8; ++i) {
        // if (notes[i] == frequency) {
            voices[i]->noteOff();
            voiceInUse[i] = false;
        // }
    }
}

/*
Allocator should never play a note on a voice that is already in use,
except if all voices are in use. In that case it should replace the voice
that was triggered first? Or just ignore incoming requests for now.
*/