# VOID WAVE — Full Feature Reference

## OSCILLATORS

Two independent wavetable oscillators with identical capabilities.

**Per oscillator:**
- 64 wavetable slots: 9 factory (Sine, Saw, Square, Spectral, Formant A, Formant E, Bass 01, Bass 02, Noise) + 2 user import slots
- **LOAD WAV** — button in each OSC section imports any single-cycle WAV or AIFF, resampled to 2048 samples across 256 frames. Lights up when a user-imported table is active.
- 256 frames × 2048 samples per table, 4× internal oversampling with anti-aliasing
- **Wavetable Mode:** Single (fixed frame) / Morph (manual position) / Scan (auto-advancing 0–4 Hz)
- **Phase Mode:** Free / Fixed / Random (per note trigger)
- **Unison:** 1, 2, 3, 4, 6, or 8 stacked voices per oscillator
- Coarse ±24 semitones, Fine ±100 cents
- Level 0–1, Pan (L/R), Detune (unison spread 0–200 cents), Width (0 = mono, 1 = full stereo)

**Pitch Envelope** (hardwired to both OSCs): Attack 0.001–0.5s, Amount ±24 semitones

**Analogue character (always on):**
- Per-voice brownian pitch drift ±1.5 cents
- Wavetable position spectral drift
- 3-cent startup transient blip (~4ms) simulating transistor settling
- Per-unison-voice micro-LFOs (pitch, level, pan) for organic beating

---

## OSCILLATOR MIXING

- **Blend** — crossfade OSC1 / OSC2 (0 = OSC1 only, 1 = OSC2 only)
- **Ring** — multiplicative (OSC1 × OSC2)
- **Sync** — hard sync (planned; currently falls back to blend)
- **FM** — frequency modulation, ratio 0.5–8×

---

## FILTER

**8 filter types:**

| Type | Algorithm | Character |
|------|-----------|-----------|
| LP12 | SVF (Simper TPT) | Smooth 12dB/oct lowpass |
| LP24 | Moog Huovilainen ladder | Warm, self-oscillating 24dB/oct lowpass |
| HP12 | SVF | 12dB/oct highpass |
| HP24 | Cascaded SVF | 24dB/oct highpass |
| BP | SVF | Bandpass |
| Notch | SVF | Band-reject |
| Comb | Resonant delay | Pitch-based comb |
| Formant | 3× bandpass | Vowel filter (800 / 1200 / 2600 Hz) |

**LP24 ladder detail:** Huovilainen 2004 model, 4 tanh-saturated one-pole stages, resonant feedback `res4 = 3.9 × res_exp`. Self-oscillates cleanly near maximum resonance. Analogue-style state saturation above resonance 0.45.

**Controls:**
- **Cutoff** 20–20kHz (logarithmic, skewed)
- **Resonance** 0–1 (exponential curve for musical spread)
- **ENV DEP** — bipolar filter envelope depth (lives in filter panel, next to RES)
- **Drive** 0–1 (arctangent soft-clip pre-filter)
- **Key Tracking** 0–2 (0 = none, 1 = full, 2 = double)
- **Vel Tracking** 0–1

**OSC Route buttons (1+2 / 1 / 2):** Routes both oscillators through the filter, OSC1 only (OSC2 dry), or OSC2 only (OSC1 dry).

**Always-on warmth:** 2% asymmetric even-harmonic saturation (x·|x|) at filter input.

---

## ENVELOPES

Three AHDSHR envelopes. All ENV1 and ENV2 controls are **always visible simultaneously** — the FILTER ENV / AMP ENV tabs above the ADSR visualisation only switch which envelope's shape is drawn.

- **FILTER ENV (ENV1):** ATK DCY SUS HLD REL — depth control lives in Filter panel
- **AMP ENV (ENV2):** ATK DCY SUS HLD REL
- *(ENV3 available in APVTS for mod matrix, hidden in UI)*

**Per envelope:**
- Attack 0.001–10s, Hold 0–5s, Decay 0.001–10s, Sustain 0–1, Release 0.001–15s
- Curve: Linear / Exponential / Logarithmic
- Loop toggle (re-trigger from release for LFO-style behaviour)
- Velocity Sensitivity 0–1

**F.CRV / A.CRV** — separate curve type dropdowns for filter and amp envelopes, side by side at panel bottom.

---

## LFOs

Two LFOs. Both sets of controls are **always visible simultaneously** — the LFO 1 / LFO 2 tabs control only which LFO's waveform is animated in the preview.

**Shapes (7):** Sine, Triangle, Sawtooth, Ramp, Square, Sample & Hold, Smooth Random

**Rate:** Free 0.01–20 Hz, or tempo-synced (1/32 to 8 bars at host BPM)

**Per LFO:** Rate, Depth (bipolar ±1), Phase 0–360°, Fade-in 0–10s, Trigger Mode (Free / Retrig / One-shot), Tempo Sync toggle

**Default routings:** LFO1 → WT position, LFO2 → filter cutoff (overridable via mod matrix)

---

## SUB OSC + NOISE

Located in the Character panel (top third). Runs per-voice through the main filter.

- **Sub** — pure sine wave at −1 or −2 octaves below played note. Level 0–1, octave toggle button (-1 / -2).
- **Noise** — filtered noise source. Level 0–1, Color knob (0 = 200Hz LP dark noise → 1 = white noise).

Both default to Level = 0 in all presets.

---

## MODULATION MATRIX

12 slots, each with Source → Destination × Depth (bipolar ±1) + enable toggle.

**15 sources:** ENV1, ENV2, ENV3, LFO1, LFO2, Velocity, Note, Aftertouch, Mod Wheel, Breath, Macro 1, Macro 2, Macro 3, Macro 4, MIDI CC

**19 destinations:** OSC1 Pitch, OSC2 Pitch, OSC1 WT Pos, OSC2 WT Pos, OSC1 Level, OSC2 Level, OSC1 Pan, OSC2 Pan, OSC1 Uni Detune, OSC2 Uni Detune, Filter Cutoff, Filter Resonance, Filter Drive, LFO1 Rate, LFO2 Rate, LFO1 Depth, LFO2 Depth, ENV1 Depth, Amp

---

## CHARACTER PANEL

The rightmost middle panel is split into two sections:

**Top third — Sub Osc + Noise** (described above)

**Bottom two-thirds — Character Macros**
Three vertical faders labeled COLOUR, SATURATION, SPACE. Each maps to Macro 1/2/3 in the mod matrix and defaults to 0.

- **COLOUR** (Macro 1) — default: routed to filter cutoff
- **SATURATION** (Macro 2) — default: routed to filter drive
- **SPACE** (Macro 3) — default: boosts reverb mix (`mix + macro × (1 − mix)`)

---

## FX CHAIN

Four serial stages: **Distortion → Modulation → Delay → Reverb**

**Distortion:** Types: Soft Clip / Hard Clip / Tube Sat (tanh) / Foldback / Bitcrusher. Controls: Drive, Tone (post high-pass 20–20kHz), Mix.

**Modulation:** Types: Chorus / Ensemble / Flanger / Phaser (4-stage all-pass). Controls: Rate (0.01–20 Hz), Depth, Mix. Stereo LFO for natural width.

**Delay:** Types: Mono / Ping-Pong / Stereo (slight L/R offset). Controls: Time (1–2000ms or tempo-synced), Feedback, Damping, Mix.

**Reverb (Peverb algorithm):** Time-varying nested all-pass with early reflections. Controls: Size, Damping, Pre-delay 0–200ms, Mix. Presets: Room / Hall / Plate.

---

## VOICE ARCHITECTURE

- **Polyphony:** 1–16 voices (default 8), oldest-first voice stealing
- **Anti-click steal:** 3ms per-sample fade-out on stolen voice before new note fires
- **Anti-click attack:** 5ms oscillator ramp-in applied before the filter on every new note — prevents filter resonance burst when a note starts
- **Glide / Portamento:** 0–5s, Always or Legato-only mode
- **Sustain pedal:** CC64 (holds releasing voices)
- **Pitch Bend:** ±1–24 semitones (configurable range)
- **Mod Wheel:** CC1 (mod matrix source)
- **Aftertouch:** channel pressure (mod matrix source)
- **Per-voice soft clipper:** activates above 72% amplitude (analogue intermodulation simulation)

---

## MIDI LEARN

All APVTS parameters are MIDI-learnable. Assign any CC to any parameter; mappings persist with patch state.

---

## GLOBAL

- **Master Volume** — rotary knob in top bar, 0–1
- **Master Tune** — ±100 cents
- **Auto-play sequence** (standalone only) — 16-step pattern for testing, toggled via AUTO button

---

## PRESETS

109 factory presets across 5 categories:

| Category | Count |
|----------|-------|
| BASS | 36 |
| LEAD | 27 |
| PAD | 18 |
| PLUCK | 14 |
| STAB | 14 |

Loads on **Bass / Armada Bass** by default. All presets have sub_level, noise_level, noise_color, and macro defaults set to 0.

---

## DISTRIBUTION

| Platform | Formats | Install location |
|----------|---------|-----------------|
| macOS | VST3 + AU | `~/Library/Audio/Plug-Ins/VST3/` and `.../Components/` |
| Windows | VST3 | `C:\Program Files\Common Files\VST3\` |

Built and distributed via GitHub Actions on every push to `main`.

---

## NUMBERS

| | |
|---|---|
| Oscillators | 2 (up to 8 unison voices each = 16 per voice) |
| Filter types | 8 |
| Envelopes | 3 (2 visible, all controls always shown) |
| LFOs | 2 (7 shapes, both rows always visible) |
| Mod matrix slots | 12 |
| Mod sources | 15 |
| Mod destinations | 19 |
| FX stages | 4 serial |
| Wavetable slots | 64 (9 factory + 2 user import per OSC) |
| Max polyphony | 16 voices |
| Factory presets | 109 |
| Formats | VST3 + AU (Mac), VST3 (Windows) |
