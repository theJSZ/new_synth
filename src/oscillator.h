#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <memory>

#include "stk/BlitSaw.h"
#include "stk/BlitSquare.h"

enum Waveform
{
    SAW,
    SQUARE
};

class Oscillator
{
private:
    std::unique_ptr<stk::BlitSaw> saw;
    std::unique_ptr<stk::BlitSquare> square;
    Waveform currentWave;
    float baseFrequency;
    float detune;
    void updateFrequency();
public:
    Oscillator();
    ~Oscillator();
    void switchWave();
    double tick();
    void setBaseFrequency(float frequency);
    void setDetune(float value);
};

#endif