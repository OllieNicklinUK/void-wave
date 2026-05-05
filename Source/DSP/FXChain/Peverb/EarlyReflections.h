/*
  ==============================================================================

    EarlyReflections.h
    Created: 26 Nov 2017 5:50:14pm
    Author:  Pelle Juul Christensen

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "Delay.h"
#include <vector>

class EarlyReflections
{
public:
    float left;
    float right;
    
    EarlyReflections(int sampleRate);
    void process(float l, float r);

private:
    float directGain = 0.5;
    float reflectGain = 0.5;
    PverbDelay delayL;
    PverbDelay delayR;
    
    IIRFilter binauralLowPassLeft;
    IIRFilter binauralLowPassRight;
    PverbDelay binauralDelayLeft;
    PverbDelay binauralDelayRight;
};
