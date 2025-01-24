#include "oscillator.h"
#include <stdio.h>

Oscillator::Oscillator() {
    std::cout << "creating oscillator\n";
    saw = std::make_unique<stk::BlitSaw>();
    square = std::make_unique<stk::BlitSquare>();
    currentWave = SAW;
    baseFrequency = 220.0f;
    detune = 1.0f;
    std::cout << "done creating oscillator\n";

}

Oscillator::~Oscillator() {}

void Oscillator::switchWave() {
    currentWave = (currentWave == SAW) ? SQUARE : SAW;
}

double Oscillator::tick() {
    if (currentWave == SAW) {
        return saw->tick();
    } else {
        return square->tick();
    }
}

void Oscillator::setBaseFrequency(float frequency) {
    baseFrequency = frequency;
    updateFrequency();
}

void Oscillator::setDetune(float value) {
    detune = value;
    updateFrequency();
}

void Oscillator::updateFrequency() {
    saw->setFrequency(baseFrequency * detune);
    square->setFrequency(baseFrequency * detune);
}