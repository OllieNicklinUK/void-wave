/*
  ==============================================================================

    LateReverb.h
    Created: 8 Dec 2017 9:00:13am
    Author:  Pelle Juul Christensen

  ==============================================================================
*/

#pragma once
#include "AllpassFilter.h"
#include "Wrap.h"
#include "Nested.h"
#include "Delay.h"
#include "JuceHeader.h"

class LateReverb
{
public:
    float left;
    float right;
    float gain;
    LateReverb(int sampleRate);
    void process(float t, float l, float r);
    
private:
    AllpassFilter allpassAL;
    AllpassFilter allpassBL;
    AllpassFilter allpassCL;
    AllpassFilter allpassDL;
    Wrap wrapAL;
    Nested nestedAL;
    Nested nestedBL;
    PverbDelay delayAL;
    PverbDelay delayBL;
    PverbDelay delayCL;
    PverbDelay delayDL;
    IIRFilter feedbackFilterL;
    
    AllpassFilter allpassAR;
    AllpassFilter allpassBR;
    AllpassFilter allpassCR;
    AllpassFilter allpassDR;
    Wrap wrapAR;
    Nested nestedAR;
    Nested nestedBR;
    PverbDelay delayAR;
    PverbDelay delayBR;
    PverbDelay delayCR;
    PverbDelay delayDR;
    IIRFilter feedbackFilterR;
    
    
};
