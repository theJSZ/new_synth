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
    return (osc1->tick() + osc2->tick()) * aeg->tick() / 2;
}

void Voice::noteOn() {
    this->aeg->keyOn();
}

void Voice::noteOff() {
    this->aeg->keyOff();
}


// AEG
void Voice::setAegAttack(float value) {
    this->aeg->setAttackTime(value);
}
void Voice::setAegDecay(float value) {
    this->aeg->setDecayTime(value);
}
void Voice::setAegSustain(float value) {
    this->aeg->setSustainLevel(value);
}
void Voice::setAegRelease(float value) {
    this->aeg->setReleaseTime(value);
}

void Voice::setOscDetune(int osc, float value) {
    if (osc == 1) {
        osc1->setDetune(value);
    } else if (osc == 2) {
        osc2->setDetune(value);
    }
}
