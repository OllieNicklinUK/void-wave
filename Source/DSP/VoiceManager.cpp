#include "VoiceManager.h"

// ── Voice ─────────────────────────────────────────────────────────────────────

void Voice::prepare(double sr, int bs)
{
    osc1.prepare(sr, bs);
    osc2.prepare(sr, bs);
    filterL.prepare(sr, bs);
    filterR.prepare(sr, bs);
    env1.prepare(sr);
    env2.prepare(sr);
    env3.prepare(sr);
    pitchEnv.prepare(sr);
    lfo1.prepare(sr);
    lfo2.prepare(sr);
    glide.reset(sr, 0.0);
    glide.setCurrentAndTargetValue(440.0f);
    tmpL .assign(static_cast<size_t>(bs), 0.0f);
    tmpR .assign(static_cast<size_t>(bs), 0.0f);
    tmpL2.assign(static_cast<size_t>(bs), 0.0f);
    tmpR2.assign(static_cast<size_t>(bs), 0.0f);
}

void Voice::reset()
{
    osc1.reset();  osc2.reset();
    filterL.reset(); filterR.reset();
    env1.reset();  env2.reset();  env3.reset();  pitchEnv.reset();
    lfo1.reset();  lfo2.reset();
    active    = false;
    releasing = false;
    midiNote  = -1;
}

// ── VoiceManager ─────────────────────────────────────────────────────────────

VoiceManager::VoiceManager() = default;

void VoiceManager::prepare(double sr, int bs)
{
    sampleRate = sr;
    blockSize  = bs;
    for (auto& v : voices) v.prepare(sr, bs);
}

void VoiceManager::setMaxVoices(int n)       { maxVoices = juce::jlimit(1, MAX_VOICES, n); }
void VoiceManager::setMono(bool m)           { monoMode  = m; }
void VoiceManager::setGlideMode(GlideMode m) { glideMode = m; }
void VoiceManager::setStealMode(StealMode m) { stealMode = m; }
void VoiceManager::setModMatrix(const ModMatrix* m) { modMatrix = m; }
void VoiceManager::setPitchBend(float s)            { pitchBend = s; }

void VoiceManager::setGlideTime(float seconds)
{
    glideTime = seconds;
    for (auto& v : voices)
        v.glide.reset(sampleRate, static_cast<double>(seconds));
}

float VoiceManager::midiNoteToHz(int note, float pbSemis, float tuneCents)
{
    float total = static_cast<float>(note - 69) + pbSemis + tuneCents / 100.0f;
    return 440.0f * std::pow(2.0f, total / 12.0f);
}

static inline float oscHz(float baseHz, int coarseSemis, float fineCents)
{
    return baseHz * std::pow(2.0f, (static_cast<float>(coarseSemis) + fineCents / 100.0f) / 12.0f);
}

void VoiceManager::applyParamsToVoice(Voice& v, const SynthParamSnapshot& p) const
{
    // OSC 1
    v.osc1.setTableIndex(p.osc1Table);
    if (wavetableBank) v.osc1.setTableData(wavetableBank->getTableData(p.osc1Table));
    v.osc1.setPosition(p.osc1Position);
    v.osc1.setLevel(p.osc1Level);
    v.osc1.setNumVoices(p.osc1UnisonVoices);
    v.osc1.setDetune(p.osc1Detune);
    v.osc1.setWidth(p.osc1Width);
    v.osc1.setPhaseMode(p.osc1PhaseMode);
    v.osc1.setWTMode(p.osc1WTMode);

    // OSC 2
    v.osc2.setTableIndex(p.osc2Table);
    if (wavetableBank) v.osc2.setTableData(wavetableBank->getTableData(p.osc2Table));
    v.osc2.setPosition(p.osc2Position);
    v.osc2.setLevel(p.osc2Level);
    v.osc2.setNumVoices(p.osc2UnisonVoices);
    v.osc2.setDetune(p.osc2Detune);
    v.osc2.setWidth(p.osc2Width);
    v.osc2.setPhaseMode(p.osc2PhaseMode);
    v.osc2.setWTMode(p.osc2WTMode);

    // Filter (both channels same params)
    auto applyFilter = [&](Filter& f) {
        f.setType(p.filterType);
        f.setCutoff(p.filterCutoff);
        f.setResonance(p.filterRes);
        f.setDrive(p.filterDrive);
    };
    applyFilter(v.filterL);
    applyFilter(v.filterR);

    // Envelopes
    auto applyEnv = [](EnvelopeGenerator& e, float atk, float dcy, float sus,
                       float hld, float rel, CurveType crv, bool lp, float vs) {
        e.setAttack(atk); e.setDecay(dcy); e.setSustain(sus);
        e.setHold(hld);   e.setRelease(rel); e.setCurve(crv);
        e.setLoop(lp);    e.setVelocitySensitivity(vs);
    };
    applyEnv(v.env1, p.env1Attack, p.env1Decay, p.env1Sustain, p.env1Hold, p.env1Release, p.env1Curve, p.env1Loop, p.env1VelSens);
    applyEnv(v.env2, p.env2Attack, p.env2Decay, p.env2Sustain, p.env2Hold, p.env2Release, p.env2Curve, p.env2Loop, p.env2VelSens);
    applyEnv(v.env3, p.env3Attack, p.env3Decay, p.env3Sustain, p.env3Hold, p.env3Release, p.env3Curve, p.env3Loop, p.env3VelSens);

    // LFOs
    auto applyLfo = [](LFO& l, LFOShape sh, float rate, bool ts, float div,
                       float phase, int trig, float fade, float depth, double bpm) {
        l.setShape(sh); l.setRate(rate); l.setTempoSync(ts); l.setSyncDivision(div);
        l.setPhaseOffset(phase); l.setTriggerMode(trig); l.setFadeIn(fade);
        l.setDepth(depth); l.setHostBPM(bpm);
    };
    applyLfo(v.lfo1, p.lfo1Shape, p.lfo1Rate, p.lfo1TempoSync, p.lfo1SyncDiv, p.lfo1Phase, p.lfo1Trigger, p.lfo1Fade, p.lfo1Depth, p.hostBPM);
    applyLfo(v.lfo2, p.lfo2Shape, p.lfo2Rate, p.lfo2TempoSync, p.lfo2SyncDiv, p.lfo2Phase, p.lfo2Trigger, p.lfo2Fade, p.lfo2Depth, p.hostBPM);
}

void VoiceManager::updateFromParams(const SynthParamSnapshot& p)
{
    lastParams = p;
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices[i].active) applyParamsToVoice(voices[i], p);
}

Voice* VoiceManager::findFreeVoice()
{
    for (int i = 0; i < maxVoices; ++i)
        if (!voices[i].active) return &voices[i];
    return nullptr;
}

Voice* VoiceManager::stealVoice()
{
    Voice* target = nullptr;
    int    oldest = -1;
    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active && voices[i].age > oldest)
            { oldest = voices[i].age; target = &voices[i]; }
    return target;
}

void VoiceManager::noteOn(int note, float vel)
{
    // LEGATO_ONLY glide: only glide if a voice is already playing the same or recent note
    bool anyActive = false;
    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active && !voices[i].releasing) { anyActive = true; break; }

    Voice* v = findFreeVoice();
    if (!v) v = stealVoice();
    if (!v) return;

    v->midiNote  = note;
    v->velocity  = vel;
    v->active    = true;
    v->releasing = false;
    v->age       = 0;

    applyParamsToVoice(*v, lastParams);

    float baseHz = midiNoteToHz(note, pitchBend, lastParams.masterTune);

    // Glide: LEGATO_ONLY only slides from previous note; ALWAYS always slides
    bool shouldGlide = (glideMode == GlideMode::ALWAYS) ||
                       (glideMode == GlideMode::LEGATO_ONLY && anyActive);
    if (!shouldGlide)
        v->glide.setCurrentAndTargetValue(baseHz);
    else
        v->glide.setTargetValue(baseHz);

    v->osc1.setFrequency(oscHz(baseHz, lastParams.osc1Coarse, lastParams.osc1Fine));
    v->osc2.setFrequency(oscHz(baseHz, lastParams.osc2Coarse, lastParams.osc2Fine));
    v->osc1.triggerNote();
    v->osc2.triggerNote();
    v->env1.noteOn(vel);
    v->env2.noteOn(vel);
    v->env3.noteOn(vel);
    v->pitchEnv.setAttack(lastParams.pitchEnvAttack);
    v->pitchEnv.setDecay(0.05f); v->pitchEnv.setSustain(0.0f); v->pitchEnv.setRelease(0.01f);
    v->pitchEnv.noteOn(vel);
    v->lfo1.noteOn();
    v->lfo2.noteOn();
    v->filterL.reset();
    v->filterR.reset();
    // Reset drift state so each note starts with the same character
    // (drift then develops organically from silence)
    v->pitchDrift    = 0.0f;
    v->pitchDriftVel = 0.0f;
    v->wtDriftPos    = 0.0f;
    v->wtDriftVel    = 0.0f;
}

void VoiceManager::noteOff(int note)
{
    if (sustainPedal)
    {
        sustainedNotes.addIfNotAlreadyThere(note);
        return;
    }
    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active && voices[i].midiNote == note && !voices[i].releasing)
        {
            voices[i].releasing = true;
            voices[i].env1.noteOff();
            voices[i].env2.noteOff();
            voices[i].env3.noteOff();
        }
}

void VoiceManager::setSustain(bool on)
{
    sustainPedal = on;
    if (!on)
    {
        for (int note : sustainedNotes)
            for (int i = 0; i < maxVoices; ++i)
                if (voices[i].active && voices[i].midiNote == note && !voices[i].releasing)
                {
                    voices[i].releasing = true;
                    voices[i].env1.noteOff();
                    voices[i].env2.noteOff();
                    voices[i].env3.noteOff();
                }
        sustainedNotes.clear();
    }
}

void VoiceManager::allNotesOff()
{
    for (auto& v : voices) { v.env1.noteOff(); v.env2.noteOff(); v.env3.noteOff(); }
}

void VoiceManager::process(juce::AudioBuffer<float>& buffer)
{
    const int  n    = buffer.getNumSamples();
    float*     bufL = buffer.getWritePointer(0);
    float*     bufR = buffer.getWritePointer(1);

    for (int i = 0; i < maxVoices; ++i)
    {
        Voice& v = voices[i];
        if (!v.active) continue;
        v.age += n;

        // ── Base frequency with coarse/fine ──────────────────────────────
        float baseHz = v.glide.getCurrentValue();
        v.glide.skip(n);

        // ── Analogue pitch drift — brownian random walk ±1.5 cents ───────
        // Inaudible as vibrato, but the voice never sounds frozen.
        v.pitchDriftVel += (v.rng.nextFloat() - 0.5f) * 0.0012f;
        v.pitchDriftVel *= 0.993f;
        v.pitchDrift = juce::jlimit(-1.5f, 1.5f, v.pitchDrift + v.pitchDriftVel);

        // ── Startup transient — brief +3 cent blip on trigger (~4ms) ─────
        // Real oscillators take a moment to settle; makes attacks feel organic.
        float startupCents = 0.0f;
        const float startupSamples = static_cast<float>(sampleRate) * 0.004f;
        if (v.age < static_cast<int>(startupSamples))
        {
            float t = 1.0f - static_cast<float>(v.age) / startupSamples;
            startupCents = 3.0f * t * t;
        }

        // ── Pitch envelope — fast attack/decay hardwired to both OSCs ────────
        // Essential for punchy bass transients and pluck character.
        float pitchEnvVal = v.pitchEnv.getNextSample();
        float pitchEnvMult = std::pow(2.0f, pitchEnvVal * lastParams.pitchEnvAmount / 12.0f);

        float driftMult = std::pow(2.0f, (v.pitchDrift + startupCents) / 1200.0f) * pitchEnvMult;
        float osc1Hz = oscHz(baseHz, lastParams.osc1Coarse, lastParams.osc1Fine) * driftMult;
        float osc2Hz = oscHz(baseHz, lastParams.osc2Coarse, lastParams.osc2Fine) * driftMult;
        v.osc1.setFrequency(osc1Hz);
        v.osc2.setFrequency(osc2Hz);

        // ── Advance LFOs by full block (phaseInc is per-sample) ──────────
        float lfo1Val = v.lfo1.processBlock(n) * lastParams.lfo1Depth;
        float lfo2Val = v.lfo2.processBlock(n) * lastParams.lfo2Depth;
        float env3Val = v.env3.getNextSample();

        // ── Populate mod sources ─────────────────────────────────────────
        auto& src = v.modValues.sources;
        src[int(ModSource::ENV3)]     = env3Val;
        src[int(ModSource::LFO1)]     = lfo1Val;
        src[int(ModSource::LFO2)]     = lfo2Val;
        src[int(ModSource::VELOCITY)] = v.velocity;
        src[int(ModSource::NOTE)]     = juce::jmap(static_cast<float>(v.midiNote), 0.f, 127.f, -1.f, 1.f);
        src[int(ModSource::MACRO1)]    = lastParams.macro[0];
        src[int(ModSource::MACRO2)]    = lastParams.macro[1];
        src[int(ModSource::MACRO3)]    = lastParams.macro[2];
        src[int(ModSource::MACRO4)]    = lastParams.macro[3];
        src[int(ModSource::MOD_WHEEL)] = lastParams.modWheel;
        src[int(ModSource::AFTERTOUCH)]= lastParams.aftertouch;

        // ── Run mod matrix ────────────────────────────────────────────────
        if (modMatrix) modMatrix->process(v.modValues);
        auto& dst = v.modValues.dests;

        // ── Apply block-level destinations ────────────────────────────────
        // WT position: param + mod matrix + LFO1 default + brownian spectral drift
        // The drift is a slow random walk that makes the wavetable position wander
        // unpredictably — Serum-style "always something subtly moving".
        v.wtDriftVel += (v.rng.nextFloat() - 0.5f) * 0.0005f;
        v.wtDriftVel *= 0.996f;
        v.wtDriftPos = juce::jlimit(-0.055f, 0.055f, v.wtDriftPos + v.wtDriftVel);

        v.osc1.setPosition(juce::jlimit(0.f, 1.f,
            lastParams.osc1Position + dst[int(ModDest::OSC1_WT_POS)]
            + lfo1Val * 0.5f + v.wtDriftPos));
        v.osc2.setPosition(juce::jlimit(0.f, 1.f,
            lastParams.osc2Position + dst[int(ModDest::OSC2_WT_POS)]
            + lfo1Val * 0.5f + v.wtDriftPos));

        // Pitch mod (additive semitones from mod matrix — uses drift-applied freqs)
        if (std::abs(dst[int(ModDest::OSC1_PITCH)]) > 0.001f)
            v.osc1.setFrequency(osc1Hz * std::pow(2.f, dst[int(ModDest::OSC1_PITCH)] / 12.f));
        if (std::abs(dst[int(ModDest::OSC2_PITCH)]) > 0.001f)
            v.osc2.setFrequency(osc2Hz * std::pow(2.f, dst[int(ModDest::OSC2_PITCH)] / 12.f));

        // Filter key + velocity tracking
        float noteOffset = static_cast<float>(v.midiNote - 60);
        float trackedCutoff = lastParams.filterCutoff
            * std::pow(2.f, noteOffset * lastParams.filterKeyTrack / 12.f)
            * (1.f - lastParams.filterVelTrack + lastParams.filterVelTrack * v.velocity);
        float baseCutoff = juce::jlimit(20.f, 20000.f,
            trackedCutoff + dst[int(ModDest::FILTER_CUTOFF)] * 10000.f);

        float baseRes = juce::jlimit(0.f, 1.f,
            lastParams.filterRes + dst[int(ModDest::FILTER_RES)]);
        v.filterL.setResonance(baseRes);
        v.filterR.setResonance(baseRes);

        float baseDrive = juce::jlimit(0.f, 1.f,
            lastParams.filterDrive + dst[int(ModDest::FILTER_DRIVE)]);
        v.filterL.setDrive(baseDrive);
        v.filterR.setDrive(baseDrive);

        // ── Render oscillators ────────────────────────────────────────────
        v.osc1.process(v.tmpL.data(), v.tmpR.data(), n);

        float oscMix = juce::jlimit(0.f, 1.f, lastParams.oscMix);
        if (oscMix > 0.001f)
        {
            v.osc2.process(v.tmpL2.data(), v.tmpR2.data(), n);
            switch (lastParams.oscMixMode)
            {
                case 1: // Ring
                    for (int s = 0; s < n; ++s)
                    {
                        v.tmpL[s] = v.tmpL[s] * v.tmpL2[s];
                        v.tmpR[s] = v.tmpR[s] * v.tmpR2[s];
                    }
                    break;
                default: // Blend (0), Sync/FM fall back to blend until implemented
                    for (int s = 0; s < n; ++s)
                    {
                        v.tmpL[s] = v.tmpL[s] * (1.f - oscMix) + v.tmpL2[s] * oscMix;
                        v.tmpR[s] = v.tmpR[s] * (1.f - oscMix) + v.tmpR2[s] * oscMix;
                    }
                    break;
            }
        }

        // ── Per-sample: ENV1+LFO2 → filter, ENV2 → VCA ───────────────────
        float ampMod = 1.f + dst[int(ModDest::AMP)];

        for (int s = 0; s < n; ++s)
        {
            float env2Val = v.env2.getNextSample();
            float env1Val = v.env1.getNextSample();

            // Update ENV sources for next block's matrix evaluation
            src[int(ModSource::ENV1)] = env1Val;
            src[int(ModSource::ENV2)] = env2Val;

            // Filter cutoff = base + ENV1 + LFO2 + filter FM (input level → cutoff)
            // Filter FM: the input signal's instantaneous amplitude modulates cutoff
            // at audio rate — gives the "breathing, alive" quality of analogue filters.
            float inputLevel = (std::abs(v.tmpL[s]) + std::abs(v.tmpR[s])) * 0.5f;
            float filterFM   = inputLevel * 55.0f;   // ~55Hz per unit amplitude

            float cutoff = baseCutoff
                + env1Val * lastParams.env1Depth * (baseCutoff - 20.f)
                + lfo2Val * 4000.f
                + filterFM;
            cutoff = juce::jlimit(20.f, 20000.f, cutoff);

            v.filterL.setCutoff(cutoff);
            v.filterR.setCutoff(cutoff);

            v.tmpL[s] = v.filterL.processSample(v.tmpL[s]);
            v.tmpR[s] = v.filterR.processSample(v.tmpR[s]);

            // ── Voice-level soft clipper — activates above 70% amplitude ─────
            // When multiple voices sum and peak together, creates intermodulation
            // that sounds like analogue crosstalk in a hardware poly synth.
            float gain = env2Val * ampMod;
            auto voiceClip = [](float x) -> float {
                const float thr = 0.72f;
                float ax = std::abs(x);
                if (ax <= thr) return x;
                float over  = ax - thr;
                float comp  = thr + over / (1.0f + over);
                return std::copysign(comp, x);
            };
            bufL[s] += voiceClip(v.tmpL[s] * gain);
            bufR[s] += voiceClip(v.tmpR[s] * gain);
        }

        if (!v.env2.isActive()) v.reset();
    }
}
