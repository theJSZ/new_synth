#ifndef VOICEALLOCATOR_H
#define VOICEALLOCATOR_H

#include "voice.h"

class voiceAllocator {
public:
    voiceAllocator(Voice* voices[]);
    void playNote(float frequency);
private:
};

#endif