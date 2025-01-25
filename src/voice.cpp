#include "voice.h"
#include <stdio.h>

void Voice::setFrequency(double frequency) {
    osc1->setBaseFrequency(frequency);
    osc2->setBaseFrequency(frequency);
}

Voice::Voice(float samplerate)
{
    std::cout << "creating voice\n";
    osc1   = std::make_unique<Oscillator>();
    osc2   = std::make_unique<Oscillator>();
    filter = std::make_unique<OberheimVariationMoog>(samplerate);
    aeg    = std::make_unique<stk::ADSR>();
    feg    = std::make_unique<stk::ADSR>();
    this->setFrequency(220.0);
    aeg->setAllTimes(0.01, 1.5, 0.0, 0.1);
    feg->setAllTimes(0.01, 1.5, 0.0, 0.1);
}

double Voice::tick() {
    float sample = (osc1->tick() + osc2->tick()) / 2;
    filter->SetCutoff(baseCutoff + (feg->tick() * fegAmount));
    filter->Process(&sample, 1);
    return (double) sample * aeg->tick();
}

void Voice::noteOn() {
    aeg->keyOn();
    feg->keyOn();
}

void Voice::noteOff() {
    aeg->keyOff();
    feg->keyOff();
}


// AEG
void Voice::setAegAttack(float value) {
    aeg->setAttackTime(value);
}
void Voice::setAegDecay(float value) {
    aeg->setDecayTime(value);
}
void Voice::setAegSustain(float value) {
    aeg->setSustainLevel(value);
}
void Voice::setAegRelease(float value) {
    aeg->setReleaseTime(value);
}


// FEG
void Voice::setFegAttack(float value) {
    feg->setAttackTime(value);
}
void Voice::setFegDecay(float value) {
    feg->setDecayTime(value);
}
void Voice::setFegSustain(float value) {
    feg->setSustainLevel(value);
}
void Voice::setFegRelease(float value) {
    feg->setReleaseTime(value);
}

void Voice::setOscDetune(int osc, float value) {
    if (osc == 1) {
        osc1->setDetune(value);
    } else if (osc == 2) {
        osc2->setDetune(value);
    }
}


// FILTER
void Voice::setCutoff(float value) {
    baseCutoff = value;
}
void Voice::setResonance(float value) {
    filter->SetResonance(value);
}
void Voice::setFegAmount(float value) {
    fegAmount = value;
}