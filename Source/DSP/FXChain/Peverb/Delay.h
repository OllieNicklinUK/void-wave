/*
  ==============================================================================

    PverbDelay.h
    Created: 8 Dec 2017 8:59:44am
    Author:  Pelle Juul Christensen

  ==============================================================================
*/

#pragma once
#include "Processor.h"
#include <vector>

class PverbDelay : public Processor
{
public:
    PverbDelay(int sampleRate);
    PverbDelay(float maxDelay, int sampleRate);
    void resize(float maxDelay);
    void write(float sample);
    float read(float delay);
    float read();
    virtual float process(float x) override;
    
    float getMaxDelay() const { return maxDelay; };
    
private:
    float maxDelay;
    int sampleRate;
    int index;
    std::vector<float> delayBuffer;
};