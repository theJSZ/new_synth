#ifndef VOICE_H
#define VOICE_H

#include "stk/BlitSaw.h"
#include "stk/BlitSquare.h"
#include "stk/ADSR.h"
#include <stdio.h>
#include "OberheimVariationModel.h"
#include "oscillator.h"
#include "memory"

class Voice
{
private:
    std::unique_ptr<Oscillator> osc1;
    std::unique_ptr<Oscillator> osc2;
    float osc1volume = 1.0f;
    float osc2volume = 1.0f;
    float xModVolume = 0.0f;

    std::unique_ptr<OberheimVariationMoog> filter;
    std::unique_ptr<stk::ADSR> aeg;
    std::unique_ptr<stk::ADSR> feg;
    float fegAmount;
    float baseCutoff;

public:
    Voice(float samplerate);
    ~Voice() = default;
    double tick();
    void setFrequency(double frequency);
    void noteOn();
    void noteOff();

    void setAegAttack(float value);
    void setAegDecay(float value);
    void setAegSustain(float value);
    void setAegRelease(float value);

    void setFegAttack(float value);
    void setFegDecay(float value);
    void setFegSustain(float value);
    void setFegRelease(float value);

    void setOscDetune(int osc, float value);
    void setOscVolume(int osc, float value);
    void setXModAmount(float value);
    void toggleOscWaveform(int osc, bool value);

    void setCutoff(float value);
    void setResonance(float value);
    void setFegAmount(float value);

    void setXModVolume(float value);
};

#endif