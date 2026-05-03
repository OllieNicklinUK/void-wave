#pragma once
#include <JuceHeader.h>

enum class CurveType  { LINEAR, EXPONENTIAL, LOGARITHMIC };
enum class TrigMode   { ONE_SHOT, GATE, TRIGGER };

class EnvelopeGenerator
{
public:
    EnvelopeGenerator();

    void prepare(double sampleRate);
    void reset();

    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);   // 0.0–1.0
    void setHold(float seconds);
    void setRelease(float seconds);
    void setCurve(CurveType curve);
    void setLoop(bool shouldLoop);
    void setVelocitySensitivity(float sens);  // 0.0–1.0

    void noteOn(float velocity = 1.0f);
    void noteOff();

    float getNextSample();
    bool  isActive() const;

private:
    enum class Stage { IDLE, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE };

    double sampleRate  = 44100.0;
    Stage  stage       = Stage::IDLE;

    float attackTime   = 0.01f;
    float decayTime    = 0.1f;
    float sustainLevel = 0.7f;
    float holdTime     = 0.0f;
    float releaseTime  = 0.3f;
    CurveType curve    = CurveType::EXPONENTIAL;
    bool  loop         = false;
    float velSens      = 0.5f;

    float output              = 0.0f;
    float velocity            = 1.0f;
    float releaseStartLevel   = 0.0f;
    int   sampleCounter       = 0;

    float attackSamples  = 0.0f;
    float decaySamples   = 0.0f;
    float holdSamples    = 0.0f;
    float releaseSamples = 0.0f;

    void recalculate();
    float applyCurve(float linear) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeGenerator)
};
