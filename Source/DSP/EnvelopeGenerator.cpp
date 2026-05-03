#include "EnvelopeGenerator.h"

EnvelopeGenerator::EnvelopeGenerator() = default;

void EnvelopeGenerator::prepare(double sr)
{
    sampleRate = sr;
    recalculate();
}

void EnvelopeGenerator::reset()
{
    stage        = Stage::IDLE;
    output       = 0.0f;
    sampleCounter = 0;
}

void EnvelopeGenerator::setAttack(float s)  { attackTime   = s; recalculate(); }
void EnvelopeGenerator::setDecay(float s)   { decayTime    = s; recalculate(); }
void EnvelopeGenerator::setSustain(float l) { sustainLevel = l; }
void EnvelopeGenerator::setHold(float s)    { holdTime     = s; recalculate(); }
void EnvelopeGenerator::setRelease(float s) { releaseTime  = s; recalculate(); }
void EnvelopeGenerator::setCurve(CurveType c) { curve = c; }
void EnvelopeGenerator::setLoop(bool l)       { loop  = l; }
void EnvelopeGenerator::setVelocitySensitivity(float v) { velSens = v; }

void EnvelopeGenerator::recalculate()
{
    auto toSamples = [&](float t) { return t * static_cast<float>(sampleRate); };
    attackSamples  = toSamples(attackTime);
    decaySamples   = toSamples(decayTime);
    holdSamples    = toSamples(holdTime);
    releaseSamples = toSamples(releaseTime);
}

void EnvelopeGenerator::noteOn(float vel)
{
    velocity = 1.0f - velSens + velSens * vel;
    stage         = Stage::ATTACK;
    sampleCounter = 0;
}

void EnvelopeGenerator::noteOff()
{
    if (stage != Stage::IDLE)
    {
        releaseStartLevel = output;
        stage             = Stage::RELEASE;
        sampleCounter     = 0;
    }
}

bool EnvelopeGenerator::isActive() const
{
    return stage != Stage::IDLE;
}

float EnvelopeGenerator::applyCurve(float linear) const
{
    switch (curve)
    {
        case CurveType::EXPONENTIAL:  return linear * linear;
        case CurveType::LOGARITHMIC:  return std::sqrt(linear);
        case CurveType::LINEAR:
        default:                      return linear;
    }
}

float EnvelopeGenerator::getNextSample()
{
    switch (stage)
    {
        case Stage::IDLE:
            return 0.0f;

        case Stage::ATTACK:
        {
            float t = (attackSamples > 0.0f) ? static_cast<float>(sampleCounter) / attackSamples : 1.0f;
            output = applyCurve(juce::jlimit(0.0f, 1.0f, t)) * velocity;
            ++sampleCounter;
            if (sampleCounter >= static_cast<int>(attackSamples))
            {
                output = velocity;
                stage = (holdTime > 0.0f) ? Stage::HOLD : Stage::DECAY;
                sampleCounter = 0;
            }
            break;
        }

        case Stage::HOLD:
        {
            output = velocity;
            ++sampleCounter;
            if (sampleCounter >= static_cast<int>(holdSamples))
            {
                stage = Stage::DECAY;
                sampleCounter = 0;
            }
            break;
        }

        case Stage::DECAY:
        {
            float t = (decaySamples > 0.0f) ? 1.0f - static_cast<float>(sampleCounter) / decaySamples : 0.0f;
            float decayRange = velocity - sustainLevel * velocity;
            output = sustainLevel * velocity + decayRange * applyCurve(juce::jlimit(0.0f, 1.0f, t));
            ++sampleCounter;
            if (sampleCounter >= static_cast<int>(decaySamples))
            {
                output = sustainLevel * velocity;
                stage  = Stage::SUSTAIN;
                sampleCounter = 0;
            }
            break;
        }

        case Stage::SUSTAIN:
            output = sustainLevel * velocity;
            break;

        case Stage::RELEASE:
        {
            float t = (releaseSamples > 0.0f) ? 1.0f - static_cast<float>(sampleCounter) / releaseSamples : 0.0f;
            output = releaseStartLevel * applyCurve(juce::jlimit(0.0f, 1.0f, t));
            ++sampleCounter;
            if (sampleCounter >= static_cast<int>(releaseSamples))
            {
                if (loop)
                {
                    noteOn(velocity);
                }
                else
                {
                    output = 0.0f;
                    stage  = Stage::IDLE;
                }
            }
            break;
        }
    }

    return output;
}
