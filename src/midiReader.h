#ifndef MIDIREADER_H
#define MIDIREADER_H

#include "voiceAllocator.h"
#include "stk/RtMidi.h"
#include "stk/SKINImsg.h"
#include <queue>

class MidiReader {
public:
    MidiReader(voiceAllocator* allocator);
    ~MidiReader();
    void pollMidiEvents();
    std::pair<int, int> Read();

private:
    float midiNoteToHz(int midiNote);
    RtMidiIn *midi_in;
    unsigned int port;
};

#endif