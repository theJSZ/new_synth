#ifndef VOICEALLOCATOR_H
#define VOICEALLOCATOR_H

#include "voice.h"

class voiceAllocator {
public:
    voiceAllocator(Voice* inputVoices[], int inputNVoices);
    void noteOn(float frequency);
    void noteOff(float frequency);
private:
    Voice* voices[8];
    int nVoices;
    bool voiceInUse[8];
    float notes[8] = {0};
    int nextVoice = 0;
};

#endif