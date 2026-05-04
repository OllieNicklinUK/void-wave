# VOID WAVE — Ship Checklist
Last updated: 2026-04-30

Legend: [ ] todo  [x] done  [-] in progress

---

## 🔴 AUDIO BROKEN WITHOUT THESE (do first)

- [x] 1.  OSC2 rendered and mixed with OSC1
- [x] 2.  Mix modes: Blend + Ring implemented; Sync/FM fall back to Blend (TODO later)
- [x] 3.  LFO1/2 sampled each block, output routed to mod sources
- [x] 4.  ModMatrix called per voice per block
- [x] 5.  Mod sources populated: velocity, note, LFO1/2, ENV1/2/3, macros
- [x] 6.  ENV3 sampled each block
- [ ] 7.  Macro routing absent — MacroAssignment struct and multi-dest routing not built (new file needed)
- [x] 8.  Filter key tracking + velocity tracking applied in voice loop
- [x] 9.  Master tune applied in `midiNoteToHz()`
- [x] 10. Glide mode (ALWAYS / LEGATO_ONLY) wired from param
- [x] 11. Coarse/fine pitch offsets applied to both OSC1 and OSC2 frequencies

---

## 🟠 FEATURE INCOMPLETE

- [x] 12. LFO tempo sync toggle — SYNC button added to LFOSection, wired to `lfo1/2_tempo_sync`
- [x] 13. Delay tempo sync UI — SYNC button added to FXSection delay slot
- [ ] 14. LFO default routing — LFO1→WT pos, LFO2→filter cutoff not pre-wired at init
- [ ] 15. Formant filter vowel not sweepable — hardcoded frequencies
- [ ] 16. WTMode MORPH/SCAN not implemented
- [x] 17. Coarse/fine pitch offsets — done in previous session
- [x] 18. MIDI mod wheel (CC1), aftertouch, sustain pedal (CC64) — all parsed in processBlock
- [x] 19. Chorus feedback — applied as depth × 0.85 in updateFXFromParams
- [ ] 20. Full wavetable bank — only 3 tables; spec needs 64
- [ ] 21. WAV drag-drop import — stub
- [ ] 22. Voice steal mode — hardcoded OLDEST
- [ ] 23. Polyphony / glide controls not in UI

---

## 🟡 PERFORMANCE / CORRECTNESS

- [ ] 24. `std::vector<float> tmpL/R` allocated every block — pre-allocate in `prepareToPlay()` (`VoiceManager::process()`)
- [ ] 25. `filter.setCutoff()` called per-sample — cache coefficient, only recompute on change (`VoiceManager::process()`)
- [ ] 26. Distortion FOLDBACK `while` loop — can run forever on hot input (`Distortion.cpp`)
- [ ] 27. Filter processes one channel — both L/R share same SVF state (`Filter` / `VoiceManager`)

---

## 🟡 UI / POLISH

- [ ] 28. Fonts not embedded — need Orbitron/Rajdhani/Share Tech Mono in BinaryData (`CMakeLists.txt`, `LookAndFeel`)
- [ ] 29. Plugin scale factor 1x/1.25x/1.5x/2x — right-click menu + AffineTransform (`PluginEditor`)
- [ ] 30. WavetableDisplay — static sine; needs live waveform, OSC2, WT position bar (`WavetableDisplay.cpp`)
- [ ] 31. Filter response curve visualizer (`FilterSection`)
- [ ] 32. ADSR envelope visualizer (`EnvelopeSection`)
- [ ] 33. LFO waveform shape preview (`LFOSection`)
- [ ] 34. Mod matrix source/dest labels — shows int index not name (`ModMatrixSection`)
- [ ] 35. Preset browser panel — slide-down, category filter, search, save dialog (`PresetBrowser.cpp`)
- [ ] 36. Value readout tooltips on all knobs (`LookAndFeel`)
- [ ] 37. Master volume knob in top bar — param exists, not in UI (`PluginEditor`)
- [ ] 38. Per-FX bypass button (`FXSection`)

---

## 🔵 TESTS (spec requires all pass before Stage 4 UI)

- [ ] 39. Oscillator: correct frequency at A4, no aliasing above 16kHz
- [ ] 40. UnisonVoice: 8-voice produces 8 FFT peaks, symmetric detune
- [ ] 41. Filter LP24: –24dB/octave slope, self-oscillates at res=1.0
- [ ] 42. EnvelopeGenerator: times accurate to ±1ms
- [ ] 43. LFO: tempo sync tracks host BPM changes in real time
- [ ] 44. ModMatrix: additive, no slot bleed between 12 slots
- [ ] 45. VoiceManager: stealing correct at poly limit
- [ ] 46. FXChain: bypass produces bit-identical output
- [ ] 47. State round-trip: save/load produces no parameter drift
- [ ] 48. No audio thread allocations (JUCE_ENABLE_AUDIO_GUARD)
- [ ] 49. CPU <10% at 16 voices, 4× oversampling, all FX on (M1 + Intel i7)

---

## ⚪ PACKAGING (Stage 5)

- [ ] 50. Factory presets embedded as BinaryData (not user folder)
- [ ] 51. Plugin icon (.icns macOS, .ico Windows)
- [ ] 52. macOS code signing + hardened runtime
- [ ] 53. macOS notarization
- [ ] 54. Universal Binary verified (arm64 + x86_64 lipo)
- [ ] 55. Windows VST3 build + test
- [ ] 56. macOS installer (pkgbuild + productbuild)
- [ ] 57. Windows installer (InnoSetup)

---

## Build commands (quick reference)

```bash
cd VoidWave/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -Wno-dev   # configure
cmake --build . --config Debug -j4            # build all
cmake --build . --config Release -j4          # release build

python3 generate_presets.py                   # regenerate preset XMLs
open build/VoidWave_artefacts/Debug/Standalone/Void\ Wave.app
```
