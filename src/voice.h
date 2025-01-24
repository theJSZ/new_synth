#ifndef VOICE_H
#define VOICE_H

#include "stk/BlitSaw.h"
#include "stk/BlitSquare.h"
#include "stk/ADSR.h"
#include <stdio.h>
// #include "OberheimVariationModel.h"

enum Waveform
{
    SAW,
    SQUARE
};

class Voice
{
private:
    stk::BlitSaw *saw;
    stk::BlitSquare *square;
    Waveform waveform;
    // OberheimVariationMoog *filter;
    stk::ADSR *aeg;
    // stk::ADSR *feg;
public:
    Voice();
    ~Voice();
    double tick();
    void setFrequency(double frequency);
    void noteOn();
    void noteOff();

    void setAegAttack(float value);
    void setAegDecay(float value);
    void setAegSustain(float value);
    void setAegRelease(float value);
};

#endif