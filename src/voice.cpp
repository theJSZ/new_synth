#include "voice.h"

void Voice::setFrequency(double frequency)
{
    this->saw->setFrequency(frequency);
    this->square->setFrequency(frequency);
}

Voice::Voice()
    : waveform(SAW)
{
    this->saw    = new stk::BlitSaw();
    this->square = new stk::BlitSquare();
    this->aeg    = new stk::ADSR();

    this->setFrequency(220.0);
    this->aeg->setAllTimes(0.5, 1.5, 0.4, 0.01);
    this->aeg->keyOn();
}

Voice::~Voice()
{
    delete this->saw;
    delete this->square;
    delete this->aeg;
}

double Voice::tick()
{
    return this->square->tick() * this->aeg->tick();
}

