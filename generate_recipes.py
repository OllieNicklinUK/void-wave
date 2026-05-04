#!/usr/bin/env python3
"""
generate_recipes.py  ·  VOID WAVE recipe-driven preset generator
─────────────────────────────────────────────────────────────────
CRITICAL FORMULA NOTES (from VoiceManager.cpp):

  WT position: osc1Position + lfo1Val * 0.5 + wtDrift
  Filter cutoff: baseCutoff + env1Val * env1Depth * (baseCutoff - 20)
               + lfo2Val * 4000  (lfo2Val = lfoOutput * lfo2Depth)

  So:
  - lfo1_depth=0.5 (old default) = WT wobbles ±25% on EVERY preset → fix: default 0.0
  - lfo2_depth=0.5 (old default) = filter wobbles ±2000Hz on EVERY preset → fix: default 0.0
  - env1 sweep range = env1Depth * (baseCutoff - 20). At cutoff=400Hz, depth=0.8
    only gives ±304Hz. For acid screams use cutoff≥2000Hz.

Run:
  python generate_recipes.py
  python generate_recipes.py --outdir=/path
  python generate_recipes.py --dry-run
"""
import sys
from pathlib import Path
from xml.etree import ElementTree as ET

# ─── Symbolic constants ───────────────────────────────────────────────────────

SINE, SAW, SQUARE, SPECTRAL, FORMANT_A, FORMANT_E, BASS01, BASS02, NOISE = range(9)

# filter_type: LP12=0 LP24=1 HP12=2 HP24=3 BP=4 Notch=5 Comb=6 Formant=7
LP12, LP24, HP12, HP24, BP, NOTCH, COMB, FORMANT_F = 0, 1, 2, 3, 4, 5, 6, 7

SINE_LFO, TRI_LFO, SAW_LFO, RAMP_LFO, SQ_LFO, SH_LFO, SMOOTH_LFO = range(7)

UNI_1, UNI_2, UNI_3, UNI_4, UNI_6, UNI_8 = 0, 1, 2, 3, 4, 5   # choice → 1,2,3,4,6,8

WT_SINGLE, WT_MORPH, WT_SCAN = 0, 1, 2

# ─── Parameter defaults ───────────────────────────────────────────────────────
# KEY: lfo1_depth and lfo2_depth are 0.0 here.
# Old value was 0.5, which caused WT-pos and filter-cutoff modulation on
# every single preset, making them all sound identical.

DEFAULTS = {
    "osc1_table": SINE,      "osc1_position": 0.0,   "osc1_coarse": 0,
    "osc1_fine":  0.0,       "osc1_level":    0.8,   "osc1_pan":    0.0,
    "osc1_unison": UNI_1,   "osc1_detune":   0.0,   "osc1_phase_mode": 0,
    "osc1_wt_mode": WT_SINGLE, "osc1_width":  1.0,
    "osc2_table": SINE,      "osc2_position": 0.0,   "osc2_coarse": 0,
    "osc2_fine":  0.0,       "osc2_level":    0.8,   "osc2_pan":    0.0,
    "osc2_unison": UNI_1,   "osc2_detune":   0.0,   "osc2_phase_mode": 0,
    "osc2_wt_mode": WT_SINGLE, "osc2_width":  1.0,
    # osc_mix=0.0: OSC2 is bypassed unless recipe sets osc_mix > 0
    "osc_mix": 0.0,          "osc_mix_mode":  0,     "osc_fm_ratio": 1.0,
    "pitchenv_attack": 0.01, "pitchenv_amount": 0.0,
    "filter_type": LP24,     "filter_cutoff": 5000.0, "filter_res":  0.0,
    "filter_drive": 0.0,     "filter_keytrack": 0.0,  "filter_veltrack": 0.0,
    "env1_attack": 0.01,  "env1_decay":  0.2,  "env1_sustain": 0.5,
    "env1_hold":   0.0,   "env1_release": 0.3, "env1_curve":   1,
    "env1_depth":  0.0,   "env1_loop":   0,    "env1_vel_sens": 0.0,
    "env2_attack": 0.01,  "env2_decay":  0.2,  "env2_sustain": 0.8,
    "env2_hold":   0.0,   "env2_release": 0.4, "env2_curve":   1,
    "env2_loop":   0,     "env2_vel_sens": 0.0,
    "env3_attack": 0.01,  "env3_decay":  0.1,  "env3_sustain": 0.7,
    "env3_hold":   0.0,   "env3_release": 0.3, "env3_curve":   1,
    "env3_loop":   0,     "env3_vel_sens": 0.0,
    # LFO depths default 0.0 — CRITICAL FIX from 0.5
    "lfo1_shape": SINE_LFO, "lfo1_rate": 1.0,  "lfo1_tempo_sync": 0,
    "lfo1_sync_div": 0.5,   "lfo1_phase": 0.0, "lfo1_trigger": 0,
    "lfo1_fade": 0.0,        "lfo1_depth": 0.0,
    "lfo2_shape": SINE_LFO, "lfo2_rate": 1.0,  "lfo2_tempo_sync": 0,
    "lfo2_sync_div": 0.5,   "lfo2_phase": 0.0, "lfo2_trigger": 0,
    "lfo2_fade": 0.0,        "lfo2_depth": 0.0,
    **{f"mod{i}_source":  0   for i in range(1, 13)},
    **{f"mod{i}_dest":    0   for i in range(1, 13)},
    **{f"mod{i}_depth":   0.0 for i in range(1, 13)},
    **{f"mod{i}_enabled": 0   for i in range(1, 13)},
    "macro1": 0.0, "macro2": 0.0, "macro3": 0.0, "macro4": 0.0,
    "voice_poly": 8,    "voice_glide": 0.0, "voice_glide_mode": 1,
    "voice_pitch_bend": 2, "master_tune": 0.0, "master_volume": 0.8,
    # FX off by default (mix=0)
    "fx1_type": 0,  "fx1_param1": 0.0,   "fx1_param2": 8000.0, "fx1_param3": 0.0,
    "fx2_type": 0,  "fx2_param1": 1.0,   "fx2_param2": 0.5,    "fx2_param3": 0.0,
    "fx3_type": 1,  "fx3_param1": 375.0, "fx3_param2": 0.4,    "fx3_param3": 0.0,
    "fx3_sync": 0,
    "fx4_param1": 0.6, "fx4_param2": 0.5, "fx4_param3": 0.0,   "fx4_param4": 0.0,
}

INT_PARAMS = {k for k, v in DEFAULTS.items() if isinstance(v, int)}

# ─── FX helpers ───────────────────────────────────────────────────────────────

def dist(drive=0.4, tone=5000.0, mix=0.5):
    return {"fx1_param1": drive, "fx1_param2": tone, "fx1_param3": mix}

def chorus(rate=0.8, depth=0.45, mix=0.4):
    return {"fx2_param1": rate, "fx2_param2": depth, "fx2_param3": mix}

def delay(ms=375.0, feedback=0.35, mix=0.25):
    return {"fx3_param1": ms, "fx3_param2": feedback, "fx3_param3": mix}

def reverb(size=0.6, damp=0.5, mix=0.3):
    return {"fx4_param1": size, "fx4_param2": damp, "fx4_param4": mix}

def mono(glide=0.0):
    return {"voice_poly": 1, "voice_glide": glide, "voice_glide_mode": 1}

def lfo1(rate=1.0, shape=SINE_LFO, depth=0.2, fade=0.0):
    """LFO1 → WT position.  depth 0.2 = ±10% of WT range."""
    return {"lfo1_rate": rate, "lfo1_shape": shape, "lfo1_depth": depth,
            "lfo1_fade": fade, "lfo1_trigger": 1}   # retrigger on note

def lfo2(rate=1.0, shape=SINE_LFO, depth=0.2):
    """LFO2 → filter cutoff.  depth 0.2 = ±800Hz swing (via 4000Hz scale)."""
    return {"lfo2_rate": rate, "lfo2_shape": shape, "lfo2_depth": depth,
            "lfo2_trigger": 1}

# ─── RECIPES ─────────────────────────────────────────────────────────────────
# (name, category, sub, overrides)
#
# env1_depth formula: cutoff_peak = baseCutoff + depth * (baseCutoff - 20)
#   → max sweep is roughly baseCutoff * (1 + depth)
#   → use HIGH baseCutoff for large sweeps; LOW baseCutoff + depth=0 for static dark sounds
#
# lfo2_depth formula: cutoff += lfoOutput * lfo2Depth * 4000Hz
#   → depth=0.2 → ±800Hz swing; depth=0.4 → ±1600Hz

RECIPES = [

    # ══════════════════════════════════════════════════════════════════════════
    # BASS (15)
    # ══════════════════════════════════════════════════════════════════════════

    ("Warehouse Sub", "BASS", "SUB", {
        # Clean 808 sub. SINE only — zero harmonics. Filter fully closed (cutoff=80Hz,
        # depth=0). All character from pitch transient and amplitude shape.
        "osc1_table": SINE, "osc1_coarse": -12, "osc1_level": 0.92,
        "filter_type": LP24, "filter_cutoff": 80.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.06, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.8,
        "pitchenv_attack": 0.003, "pitchenv_amount": 8.0,
        **mono(), **reverb(size=0.3, damp=0.8, mix=0.1),
    }),

    ("Acid Pipeline", "BASS", "ACID", {
        # TB-303. baseCutoff=2200, depth=0.88 → envelope peaks at 4136Hz.
        # Resonance 0.88 creates the squelch. Pitch blip = 303 "click".
        "osc1_table": SAW, "osc1_coarse": -12, "osc1_level": 0.85,
        "filter_type": LP24, "filter_cutoff": 2200.0, "filter_res": 0.88,
        "filter_drive": 0.2,
        "env1_attack": 0.002, "env1_decay": 0.10, "env1_sustain": 0.0,
        "env1_release": 0.06, "env1_depth": 0.88,
        "env2_attack": 0.002, "env2_decay": 0.0,  "env2_sustain": 1.0,
        "env2_release": 0.12,
        "pitchenv_attack": 0.003, "pitchenv_amount": 10.0,
        **mono(glide=0.04),
        **dist(drive=0.25, tone=5000.0, mix=0.35),
        **delay(ms=340.0, feedback=0.28, mix=0.18),
    }),

    ("Reese Machine", "BASS", "REESE", {
        # Two SAWs 15 cents apart — beating creates the movement, NOT the filter.
        # Filter wide open (cutoff=6000, depth=0). All character from distortion.
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc1_unison": UNI_3, "osc1_detune": 0.35, "osc1_width": 0.8,
        "osc2_table": SAW, "osc2_coarse": 0, "osc2_fine": 15.0,
        "osc2_level": 0.8, "osc_mix": 0.42,
        "filter_type": LP24, "filter_cutoff": 6000.0, "filter_res": 0.05,
        "filter_drive": 0.2,
        "env1_depth": 0.0,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        **mono(), **dist(drive=0.55, tone=2500.0, mix=0.6),
    }),

    ("Neurofunk Growl", "BASS", "NEURO", {
        # FORMANT_A table for vowel character. LFO2 @ 3.5Hz on filter = growl.
        # lfo2 depth=0.25 → ±1000Hz swing around 1800Hz base.
        "osc1_table": FORMANT_A, "osc1_position": 0.3, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "osc2_table": SAW, "osc2_coarse": 0, "osc2_fine": 15.0,
        "osc2_level": 0.7, "osc_mix": 0.35,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.65,
        "filter_drive": 0.35,
        "env1_attack": 0.005, "env1_decay": 0.25, "env1_sustain": 0.1,
        "env1_release": 0.2,  "env1_depth": 0.7,
        "env2_attack": 0.005, "env2_decay": 0.0,  "env2_sustain": 1.0,
        "env2_release": 0.2,
        "pitchenv_attack": 0.003, "pitchenv_amount": 6.0,
        **lfo2(rate=3.5, shape=SINE_LFO, depth=0.25),
        **mono(), **dist(drive=0.45, tone=3500.0, mix=0.5),
    }),

    ("808 Trap", "BASS", "808", {
        # Trap 808. Long VCA tail (3.5s release). Filter locked shut.
        # Pitch env fires +12st then falls = 808 boom with pitchy tail.
        "osc1_table": SINE, "osc1_coarse": -12, "osc1_level": 0.92,
        "filter_type": LP24, "filter_cutoff": 90.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.04, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 3.5,
        "pitchenv_attack": 0.003, "pitchenv_amount": 12.0,
        **mono(), **reverb(size=0.25, damp=0.85, mix=0.08),
    }),

    ("Techno Groove", "BASS", "TECHNO", {
        # Driving SAW bass. baseCutoff=1200, depth=0.55 → peaks at 1859Hz.
        # Short gate, punchy — sits right under a kick drum.
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.85,
        "filter_type": LP24, "filter_cutoff": 1200.0, "filter_res": 0.3,
        "filter_drive": 0.15,
        "env1_attack": 0.006, "env1_decay": 0.18, "env1_sustain": 0.15,
        "env1_release": 0.12, "env1_depth": 0.55,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.1,
        **mono(), **dist(drive=0.18, tone=5500.0, mix=0.25),
    }),

    ("Deep House Sub", "BASS", "HOUSE", {
        # Warm house bass. SINE root + SAW sub layer. Filter slightly warm.
        # No resonance, gentle attack. Smooth and deep.
        "osc1_table": SINE, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc2_table": SAW, "osc2_coarse": -12, "osc2_level": 0.6,
        "osc_mix": 0.3,
        "filter_type": LP12, "filter_cutoff": 450.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.02, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.6,
        **mono(glide=0.03), **reverb(size=0.35, damp=0.65, mix=0.12),
    }),

    ("EBM Hammer", "BASS", "EBM", {
        # Industrial stomp. SQUARE + SAW octave down. Hard clipped (fx1_type=1).
        # Tight gate. baseCutoff=900, depth=0.5 → peak=1345Hz.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc2_table": SAW, "osc2_coarse": -12, "osc2_level": 0.7,
        "osc_mix": 0.38,
        "filter_type": LP24, "filter_cutoff": 900.0, "filter_res": 0.22,
        "filter_drive": 0.4,
        "env1_attack": 0.003, "env1_decay": 0.15, "env1_sustain": 0.2,
        "env1_release": 0.12, "env1_depth": 0.5,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.15,
        "fx1_type": 1,  # Hard Clip
        **dist(drive=0.6, tone=3000.0, mix=0.65),
        **mono(),
    }),

    ("Formant Wobble", "BASS", "WOBBLE", {
        # Dubstep vowel wobble. FORMANT_A + LFO2 at 2Hz.
        # baseCutoff=1800, lfo2 depth=0.22 → ±880Hz swing (900–2680Hz).
        "osc1_table": FORMANT_A, "osc1_position": 0.4, "osc1_coarse": -12,
        "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.6,
        "filter_drive": 0.2,
        "env1_attack": 0.01, "env1_decay": 0.35, "env1_sustain": 0.2,
        "env1_release": 0.25, "env1_depth": 0.55,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.25,
        "pitchenv_attack": 0.003, "pitchenv_amount": 5.0,
        **lfo2(rate=2.0, shape=SINE_LFO, depth=0.22),
        **mono(), **dist(drive=0.3, tone=4000.0, mix=0.4),
    }),

    ("Trance Pumper", "BASS", "TRANCE", {
        # Uplifting trance bass. Large filter sweep: baseCutoff=4500, depth=0.75
        # → peak at 7858Hz. Fast decay gives the "pump".
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.18,
        "filter_type": LP24, "filter_cutoff": 4500.0, "filter_res": 0.15,
        "env1_attack": 0.004, "env1_decay": 0.3, "env1_sustain": 0.0,
        "env1_release": 0.18, "env1_depth": 0.75,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        **mono(), **chorus(rate=0.6, depth=0.3, mix=0.2),
        **delay(ms=330.0, feedback=0.25, mix=0.15),
    }),

    ("Jungle Growl", "BASS", "JUNGLE", {
        # BASS01 wavetable has built-in grit. baseCutoff=1500, depth=0.75
        # → peaks at 2618Hz. High resonance + pitch blip = jungle character.
        "osc1_table": BASS01, "osc1_coarse": -12, "osc1_level": 0.9,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.52,
        "filter_drive": 0.25,
        "env1_attack": 0.003, "env1_decay": 0.15, "env1_sustain": 0.0,
        "env1_release": 0.12, "env1_depth": 0.75,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.2,
        "pitchenv_attack": 0.003, "pitchenv_amount": 8.0,
        **mono(), **dist(drive=0.35, tone=4000.0, mix=0.45),
    }),

    ("Synth Pulse", "BASS", "MINIMAL", {
        # Kraftwerk precision SQUARE wave. Short gate, metronomic.
        # baseCutoff=1400, depth=0.45 → modest sweep to 2024Hz.
        "osc1_table": SQUARE, "osc1_coarse": -12, "osc1_level": 0.85,
        "filter_type": LP24, "filter_cutoff": 1400.0, "filter_res": 0.18,
        "env1_attack": 0.004, "env1_decay": 0.1, "env1_sustain": 0.0,
        "env1_release": 0.08, "env1_depth": 0.45,
        "env2_attack": 0.004, "env2_decay": 0.25, "env2_sustain": 0.0,
        "env2_release": 0.18,
        **mono(), **delay(ms=300.0, feedback=0.2, mix=0.12),
    }),

    ("Sub Drone", "BASS", "DRONE", {
        # Sustained drone. Two sines 2 cents apart = slow beating shimmer.
        # No LFO, no filter sweep. Pure hypnotic low frequency.
        "osc1_table": SINE, "osc1_coarse": -12, "osc1_level": 0.9,
        "osc2_table": SINE, "osc2_coarse": -12, "osc2_fine": 2.0,
        "osc2_level": 0.7, "osc_mix": 0.4,
        "filter_type": LP12, "filter_cutoff": 220.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 2.0,
        **reverb(size=0.55, damp=0.6, mix=0.18),
    }),

    ("Electro Blip", "BASS", "ELECTRO", {
        # Short sharp electro blip. Two-voice SAW, fast decay, no sustain.
        # baseCutoff=2500, depth=0.6 → brief peak at 3992Hz.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.2, "osc1_width": 0.6,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.4,
        "env1_attack": 0.002, "env1_decay": 0.09, "env1_sustain": 0.0,
        "env1_release": 0.08, "env1_depth": 0.6,
        "env2_attack": 0.002, "env2_decay": 0.15, "env2_sustain": 0.0,
        "env2_release": 0.12,
        "pitchenv_attack": 0.003, "pitchenv_amount": 6.0,
        **mono(),
    }),

    ("Dark Arpeggio", "BASS", "ARP", {
        # SQUARE root + SQUARE octave above. Short gate, resonant snap.
        # baseCutoff=2000, depth=0.45 → peak at 2891Hz — crisp metallic click.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc2_table": SQUARE, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.35,
        "filter_type": LP24, "filter_cutoff": 2000.0, "filter_res": 0.38,
        "env1_attack": 0.002, "env1_decay": 0.12, "env1_sustain": 0.0,
        "env1_release": 0.08, "env1_depth": 0.45,
        "env2_attack": 0.002, "env2_decay": 0.18, "env2_sustain": 0.0,
        "env2_release": 0.12,
        **mono(), **delay(ms=280.0, feedback=0.28, mix=0.18),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # LEAD (15)
    # ══════════════════════════════════════════════════════════════════════════

    ("Plasma 303", "LEAD", "ACID", {
        # SAW + SQUARE acid lead. baseCutoff=3200, depth=0.72 → 5505Hz peak.
        # Velocity opens filter further (veltrack=0.4).
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc2_table": SQUARE, "osc2_coarse": 0, "osc2_level": 0.8,
        "osc_mix": 0.32,
        "filter_type": LP24, "filter_cutoff": 3200.0, "filter_res": 0.68,
        "filter_drive": 0.2, "filter_veltrack": 0.4,
        "env1_attack": 0.003, "env1_decay": 0.15, "env1_sustain": 0.0,
        "env1_release": 0.09, "env1_depth": 0.72, "env1_vel_sens": 0.6,
        "env2_attack": 0.002, "env2_decay": 0.0,  "env2_sustain": 1.0,
        "env2_release": 0.18,
        "pitchenv_attack": 0.004, "pitchenv_amount": 12.0,
        **mono(glide=0.025),
        **dist(drive=0.22, tone=5500.0, mix=0.3),
        **delay(ms=350.0, feedback=0.32, mix=0.22),
    }),

    ("Supersaw Drift", "LEAD", "SUPER", {
        # 4-voice supersaw. OSC2 adds +1 octave accent. LFO1 on WT position
        # at 0.14Hz depth=0.18 → ±9% WT drift. Slow organic movement.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_4, "osc1_detune": 0.4, "osc1_width": 0.95,
        "osc2_table": SAW, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.62,
        "filter_type": LP24, "filter_cutoff": 8000.0, "filter_res": 0.08,
        "env1_attack": 0.01, "env1_decay": 0.5, "env1_sustain": 0.75,
        "env1_release": 0.5, "env1_depth": 0.18,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.55,
        **lfo1(rate=0.14, shape=SINE_LFO, depth=0.18),
        **chorus(rate=0.75, depth=0.45, mix=0.35),
        **reverb(size=0.55, damp=0.45, mix=0.28),
    }),

    ("Boing Mono", "LEAD", "PITCH", {
        # Pitch env −14st: every note drops from above.
        # SQUARE OSC1 + SAW fifth (7st up) for body.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.9,
        "osc2_table": SAW, "osc2_coarse": 7, "osc2_level": 0.6,
        "osc_mix": 0.42,
        "filter_type": LP24, "filter_cutoff": 5500.0, "filter_res": 0.25,
        "env1_attack": 0.005, "env1_decay": 0.28, "env1_sustain": 0.2,
        "env1_release": 0.22, "env1_depth": 0.3,
        "env2_attack": 0.002, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.2,
        "pitchenv_attack": 0.003, "pitchenv_amount": -14.0,
        **mono(glide=0.04),
        **delay(ms=310.0, feedback=0.38, mix=0.28),
        **reverb(size=0.4, damp=0.55, mix=0.18),
    }),

    ("Trance Saw", "LEAD", "TRANCE", {
        # 4-voice wide supersaw. Filter open (8500Hz). Lush chorus + reverb.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_4, "osc1_detune": 0.3, "osc1_width": 0.9,
        "filter_type": LP24, "filter_cutoff": 8500.0, "filter_res": 0.08,
        "env1_attack": 0.01, "env1_decay": 0.3, "env1_sustain": 0.8,
        "env1_release": 0.4, "env1_depth": 0.15,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.5,
        **chorus(rate=0.6, depth=0.4, mix=0.3),
        **reverb(size=0.6, damp=0.45, mix=0.32),
    }),

    ("FM Bell Lead", "LEAD", "SPECTRAL", {
        # SPECTRAL table at high position (lots of upper harmonics).
        # OSC2 an octave up adds bell overtones.
        "osc1_table": SPECTRAL, "osc1_position": 0.72, "osc1_coarse": 0,
        "osc2_table": SINE, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.38,
        "filter_type": LP24, "filter_cutoff": 7500.0, "filter_res": 0.15,
        "env1_attack": 0.003, "env1_decay": 0.4, "env1_sustain": 0.5,
        "env1_release": 0.35, "env1_depth": 0.3,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        "pitchenv_attack": 0.004, "pitchenv_amount": 5.0,
        **mono(glide=0.02),
        **delay(ms=375.0, feedback=0.3, mix=0.2),
        **reverb(size=0.5, damp=0.5, mix=0.22),
    }),

    ("Sync Rip", "LEAD", "SYNC", {
        # High WT position on SAW = harmonic-dense "sync-like" tone.
        # HP24 filter under the lead removes mud, leaves only the bite.
        "osc1_table": SAW, "osc1_position": 0.82, "osc1_coarse": 0,
        "filter_type": HP24, "filter_cutoff": 300.0, "filter_res": 0.3,
        "filter_drive": 0.25,
        "env1_attack": 0.003, "env1_decay": 0.4, "env1_sustain": 0.45,
        "env1_release": 0.3, "env1_depth": 0.0,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        "pitchenv_attack": 0.004, "pitchenv_amount": 7.0,
        **mono(glide=0.02),
        **dist(drive=0.28, tone=5000.0, mix=0.35),
        **reverb(size=0.35, damp=0.55, mix=0.18),
    }),

    ("Electro Lead", "LEAD", "EBM", {
        # SQUARE wave. HP filter removes low end. Classic EBM mono melody.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.9,
        "filter_type": HP12, "filter_cutoff": 120.0, "filter_res": 0.1,
        "env1_depth": 0.0,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.25,
        **mono(), **dist(drive=0.2, tone=5000.0, mix=0.25),
        **delay(ms=330.0, feedback=0.28, mix=0.2),
    }),

    ("Saw Vibrato", "LEAD", "WARM", {
        # Warm saw mono lead. OSC2 adds 5-cent detune for natural width.
        # LFO1 at 5.5Hz depth=0.07 = subtle vibrato (±3.5% WT movement).
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.88,
        "osc2_table": SAW, "osc2_fine": 5.0, "osc2_level": 0.5,
        "osc_mix": 0.28,
        "filter_type": LP24, "filter_cutoff": 5500.0, "filter_res": 0.12,
        "env1_attack": 0.01, "env1_decay": 0.4, "env1_sustain": 0.65,
        "env1_release": 0.35, "env1_depth": 0.22,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.35,
        **lfo1(rate=5.5, shape=SINE_LFO, depth=0.07, fade=0.3),
        **mono(glide=0.06),
        **delay(ms=360.0, feedback=0.3, mix=0.22),
        **reverb(size=0.45, damp=0.5, mix=0.2),
    }),

    ("Formant Scream", "LEAD", "FORMANT", {
        # FORMANT_A table sounds vocal. 2-voice unison for richness.
        # BP filter makes it nasal and midrange-focused — cuts through a mix.
        "osc1_table": FORMANT_A, "osc1_position": 0.4, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.15,
        "filter_type": BP, "filter_cutoff": 2200.0, "filter_res": 0.45,
        "filter_drive": 0.2,
        "env1_depth": 0.0,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        **mono(glide=0.03),
        **dist(drive=0.22, tone=4500.0, mix=0.28),
        **reverb(size=0.5, damp=0.5, mix=0.22),
    }),

    ("Retro Poly", "LEAD", "POLY", {
        # 80s polyphonic: 2-voice SAW + octave sub on OSC2. Chorus is the sound.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.15, "osc1_width": 0.7,
        "osc2_table": SAW, "osc2_coarse": -12, "osc2_level": 0.45,
        "osc_mix": 0.32,
        "filter_type": LP24, "filter_cutoff": 4500.0, "filter_res": 0.08,
        "env1_attack": 0.01, "env1_decay": 0.5, "env1_sustain": 0.7,
        "env1_release": 0.5, "env1_depth": 0.25,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.5,
        **chorus(rate=0.7, depth=0.5, mix=0.42),
        **reverb(size=0.55, damp=0.45, mix=0.28),
    }),

    ("Industrial Lead", "LEAD", "INDUSTRIAL", {
        # 3-voice SAW with heavy drive. Filter at mid-freq (3000Hz) with
        # hard clip distortion — sounds brutalised and authoritative.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_3, "osc1_detune": 0.25, "osc1_width": 0.7,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.4,
        "filter_drive": 0.4,
        "env1_attack": 0.005, "env1_decay": 0.35, "env1_sustain": 0.2,
        "env1_release": 0.3, "env1_depth": 0.45,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.3,
        "fx1_type": 1,  # Hard Clip
        **dist(drive=0.6, tone=3500.0, mix=0.65),
        **mono(),
    }),

    ("Minimal Mono", "LEAD", "MINIMAL", {
        # Single SAW, no extras. Filter at 6000Hz opens slightly with env.
        # The precision of simplicity.
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.85,
        "filter_type": LP24, "filter_cutoff": 6000.0, "filter_res": 0.18,
        "env1_attack": 0.008, "env1_decay": 0.5, "env1_sustain": 0.6,
        "env1_release": 0.4, "env1_depth": 0.28,
        "env2_attack": 0.008, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.4,
        **mono(), **delay(ms=375.0, feedback=0.28, mix=0.18),
    }),

    ("Glide Monster", "LEAD", "GLIDE", {
        # Extreme legato glide (0.18s). The portamento IS the sound design.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "filter_type": LP24, "filter_cutoff": 5000.0, "filter_res": 0.2,
        "env1_attack": 0.01, "env1_decay": 0.4, "env1_sustain": 0.65,
        "env1_release": 0.35, "env1_depth": 0.25,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.4,
        **mono(glide=0.18),
        **delay(ms=340.0, feedback=0.32, mix=0.22),
        **reverb(size=0.5, damp=0.5, mix=0.2),
    }),

    ("Void Sequence", "LEAD", "SCAN", {
        # SCAN mode at 0.5Hz. Spectral wavetable continuously sweeps —
        # the timbre changes with every note. No LFO needed; SCAN does it.
        "osc1_table": SPECTRAL, "osc1_position": 0.12, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP24, "filter_cutoff": 5500.0, "filter_res": 0.2,
        "env1_attack": 0.005, "env1_decay": 0.4, "env1_sustain": 0.4,
        "env1_release": 0.4, "env1_depth": 0.3,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.4,
        **mono(glide=0.02),
        **delay(ms=350.0, feedback=0.3, mix=0.22),
        **reverb(size=0.45, damp=0.5, mix=0.18),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # PAD (11)
    # ══════════════════════════════════════════════════════════════════════════

    ("Void Drift", "PAD", "SCAN", {
        # SCAN mode 0.72Hz. Filter opens over 1.5s. LFO1 adds WT drift on top.
        # lfo1 depth=0.18 → ±9% WT range on top of the continuous scan.
        "osc1_table": SPECTRAL, "osc1_position": 0.18, "osc1_wt_mode": WT_SCAN,
        "osc1_unison": UNI_3, "osc1_detune": 0.3, "osc1_width": 0.9,
        "osc2_table": SINE, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.5,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.06,
        "env1_attack": 1.5, "env1_decay": 4.0, "env1_sustain": 0.5,
        "env1_release": 3.0, "env1_depth": 0.55,
        "env2_attack": 1.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 4.5,
        **lfo1(rate=0.07, shape=SINE_LFO, depth=0.18),
        **chorus(rate=0.4, depth=0.5, mix=0.4),
        **reverb(size=0.82, damp=0.38, mix=0.52),
    }),

    ("Membrane", "PAD", "DENSE", {
        # 4-voice SPECTRAL + SAW fifth. Massive slow build.
        "osc1_table": SPECTRAL, "osc1_position": 0.5,
        "osc1_unison": UNI_4, "osc1_detune": 0.25, "osc1_width": 0.85,
        "osc2_table": SAW, "osc2_coarse": 7, "osc2_level": 0.7,
        "osc_mix": 0.48,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.08,
        "env1_attack": 2.5, "env1_decay": 5.0, "env1_sustain": 0.4,
        "env1_release": 4.0, "env1_depth": 0.5,
        "env2_attack": 2.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 5.5,
        **lfo1(rate=0.04, shape=SINE_LFO, depth=0.12),
        **chorus(rate=0.35, depth=0.55, mix=0.45),
        **reverb(size=0.88, damp=0.32, mix=0.58),
    }),

    ("Glasswing", "PAD", "SHIMMER", {
        # 6-voice SINE shimmer. OSC2 two octaves up adds harmonics.
        # Filter wide open — all character from unison detune.
        "osc1_table": SINE, "osc1_position": 0.2,
        "osc1_unison": UNI_6, "osc1_detune": 0.2, "osc1_width": 1.0,
        "osc2_table": SINE, "osc2_coarse": 24, "osc2_level": 0.45,
        "osc_mix": 0.55,
        "filter_type": LP12, "filter_cutoff": 12000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 1.8, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 4.5,
        **lfo1(rate=0.09, shape=SINE_LFO, depth=0.12),
        **chorus(rate=0.6, depth=0.4, mix=0.35),
        **reverb(size=0.72, damp=0.48, mix=0.44),
    }),

    ("Synthwave Dream", "PAD", "SYNTHWAVE", {
        # 4-voice SAW with slow filter swell. Warm chorus = Juno character.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_4, "osc1_detune": 0.25, "osc1_width": 0.9,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.1,
        "env1_attack": 0.6, "env1_decay": 2.5, "env1_sustain": 0.65,
        "env1_release": 2.5, "env1_depth": 0.55,
        "env2_attack": 0.8, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 3.0,
        **chorus(rate=0.65, depth=0.5, mix=0.42),
        **reverb(size=0.65, damp=0.45, mix=0.38),
    }),

    ("Techno Atmos", "PAD", "TECHNO", {
        # SPECTRAL SCAN at very slow rate (0.4Hz). Dark and industrial.
        # No filter movement — atmosphere comes purely from spectral scanning.
        "osc1_table": SPECTRAL, "osc1_position": 0.1, "osc1_wt_mode": WT_SCAN,
        "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 2.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 6.0,
        **reverb(size=0.92, damp=0.28, mix=0.58),
    }),

    ("Blade Runner", "PAD", "CINEMATIC", {
        # SAW 3-voice + SPECTRAL fifth. Dark tension.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_3, "osc1_detune": 0.2, "osc1_width": 0.85,
        "osc2_table": SPECTRAL, "osc2_position": 0.4, "osc2_coarse": 7,
        "osc2_level": 0.55, "osc_mix": 0.42,
        "filter_type": LP24, "filter_cutoff": 2200.0, "filter_res": 0.08,
        "env1_attack": 1.8, "env1_decay": 4.0, "env1_sustain": 0.5,
        "env1_release": 4.0, "env1_depth": 0.48,
        "env2_attack": 1.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 4.5,
        **lfo1(rate=0.05, shape=SINE_LFO, depth=0.12),
        **chorus(rate=0.4, depth=0.5, mix=0.38),
        **reverb(size=0.88, damp=0.36, mix=0.52),
    }),

    ("Choir Pad", "PAD", "CHOIR", {
        # FORMANT_A and FORMANT_E blended = synthetic choir.
        # No LFO, no filter modulation — purity of the vocal tone.
        "osc1_table": FORMANT_A, "osc1_unison": UNI_4, "osc1_detune": 0.15,
        "osc1_width": 0.9,
        "osc2_table": FORMANT_E, "osc2_level": 0.7, "osc_mix": 0.42,
        "filter_type": LP24, "filter_cutoff": 7000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 2.0, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 4.5,
        **reverb(size=0.82, damp=0.38, mix=0.52),
    }),

    ("Neural Wash", "PAD", "IDM", {
        # Two SPECTRAL oscillators — SCAN on OSC1, static on OSC2.
        # Dual spectral layers create complex evolving texture.
        "osc1_table": SPECTRAL, "osc1_position": 0.22, "osc1_wt_mode": WT_SCAN,
        "osc1_unison": UNI_4, "osc1_detune": 0.3, "osc1_width": 0.95,
        "osc2_table": SPECTRAL, "osc2_position": 0.6, "osc2_coarse": -12,
        "osc2_level": 0.6, "osc_mix": 0.45,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 2.5, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 7.0,
        **chorus(rate=0.35, depth=0.55, mix=0.42),
        **reverb(size=0.92, damp=0.28, mix=0.58),
    }),

    ("Retro Strings", "PAD", "STRINGS", {
        # 6-voice SAW string ensemble. OSC2 octave up adds sheen.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_6, "osc1_detune": 0.28, "osc1_width": 0.9,
        "osc2_table": SAW, "osc2_coarse": 12, "osc2_level": 0.4,
        "osc_mix": 0.48,
        "filter_type": LP24, "filter_cutoff": 6000.0, "filter_res": 0.05,
        "env1_depth": 0.0,
        "env2_attack": 0.3, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 2.5,
        **chorus(rate=0.7, depth=0.55, mix=0.48),
        **reverb(size=0.7, damp=0.45, mix=0.42),
    }),

    ("Acid Rain", "PAD", "ACID", {
        # Detuned SAWs + resonant filter + slow LFO2 wobble.
        # lfo2 depth=0.12 → ±480Hz wobble around 1800Hz base.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.3, "osc1_width": 0.8,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.42,
        "filter_drive": 0.12,
        "env1_attack": 1.8, "env1_decay": 3.5, "env1_sustain": 0.55,
        "env1_release": 3.5, "env1_depth": 0.48,
        "env2_attack": 2.0, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 5.0,
        **lfo2(rate=0.12, shape=SINE_LFO, depth=0.12),
        **chorus(rate=0.4, depth=0.45, mix=0.35),
        **reverb(size=0.82, damp=0.38, mix=0.5),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # PLUCK (9)
    # ══════════════════════════════════════════════════════════════════════════

    ("Karplus Steel", "PLUCK", "METAL", {
        # SAW body + NOISE attack burst. No sustain = dies like a real string.
        "osc1_table": SAW, "osc1_coarse": 0, "osc1_level": 0.9,
        "osc2_table": NOISE, "osc2_position": 0.5, "osc2_level": 0.6,
        "osc_mix": 0.3,
        "filter_type": LP24, "filter_cutoff": 4500.0, "filter_res": 0.35,
        "filter_drive": 0.12,
        "env1_attack": 0.001, "env1_decay": 0.2, "env1_sustain": 0.0,
        "env1_release": 0.3,  "env1_depth": 0.55,
        "env2_attack": 0.001, "env2_decay": 0.65, "env2_sustain": 0.0,
        "env2_release": 0.5,
        **delay(ms=280.0, feedback=0.22, mix=0.18),
        **reverb(size=0.45, damp=0.6, mix=0.22),
    }),

    ("Bell Drop", "PLUCK", "BELL", {
        # Two pure sines an octave apart. Long 2.5s decay. Pitch +7st on strike.
        "osc1_table": SINE, "osc1_position": 0.75, "osc1_coarse": 0,
        "osc1_level": 0.85,
        "osc2_table": SINE, "osc2_coarse": 12, "osc2_level": 0.7,
        "osc_mix": 0.48,
        "filter_type": LP12, "filter_cutoff": 14000.0, "filter_res": 0.05,
        "env1_depth": 0.0,
        "env2_attack": 0.001, "env2_decay": 2.5, "env2_sustain": 0.0,
        "env2_release": 3.0,
        "pitchenv_attack": 0.002, "pitchenv_amount": 7.0,
        **reverb(size=0.65, damp=0.45, mix=0.4),
    }),

    ("Clav Machine", "PLUCK", "FUNK", {
        # Two SQUARE oscillators 7 cents apart. BP filter gives nasal snap.
        # Ultra-fast filter decay (65ms) IS the sound.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.9,
        "osc2_table": SQUARE, "osc2_coarse": 0, "osc2_fine": 7.0,
        "osc2_level": 0.8, "osc_mix": 0.4,
        "filter_type": BP, "filter_cutoff": 2800.0, "filter_res": 0.55,
        "filter_drive": 0.18,
        "env1_attack": 0.001, "env1_decay": 0.065, "env1_sustain": 0.0,
        "env1_release": 0.08, "env1_depth": -0.5,  # filter CLOSES = snap
        "env2_attack": 0.001, "env2_decay": 0.28,  "env2_sustain": 0.0,
        "env2_release": 0.18,
        **dist(drive=0.18, tone=4000.0, mix=0.32),
        **delay(ms=240.0, feedback=0.18, mix=0.15),
    }),

    ("Electric Piano", "PLUCK", "KEYS", {
        # SINE harmonics. pitchenv +4st = classic e-piano "thump".
        "osc1_table": SINE, "osc1_position": 0.5, "osc1_coarse": 0,
        "osc2_table": SINE, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.35,
        "filter_type": LP12, "filter_cutoff": 8000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.003, "env2_decay": 1.2, "env2_sustain": 0.0,
        "env2_release": 1.5,
        "pitchenv_attack": 0.002, "pitchenv_amount": 4.0,
        **dist(drive=0.08, tone=6000.0, mix=0.18),
        **reverb(size=0.5, damp=0.55, mix=0.28),
    }),

    ("Glass Harp", "PLUCK", "GLASS", {
        # Resonant SINE + BP filter. Singing harmonic quality.
        "osc1_table": SINE, "osc1_position": 0.6, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.1, "osc1_width": 0.7,
        "filter_type": BP, "filter_cutoff": 6000.0, "filter_res": 0.45,
        "env1_depth": 0.0,
        "env2_attack": 0.001, "env2_decay": 1.0, "env2_sustain": 0.0,
        "env2_release": 1.2,
        "pitchenv_attack": 0.001, "pitchenv_amount": 5.0,
        **reverb(size=0.65, damp=0.45, mix=0.38),
    }),

    ("Techno Blip", "PLUCK", "BLIP", {
        # Pitch drops 8st on attack = percussive techno hit.
        # Notch filter at 3500Hz scoops the mid for a hollow, industrial sound.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.9,
        "filter_type": NOTCH, "filter_cutoff": 3500.0, "filter_res": 0.5,
        "env1_depth": 0.0,
        "env2_attack": 0.001, "env2_decay": 0.1, "env2_sustain": 0.0,
        "env2_release": 0.12,
        "pitchenv_attack": 0.001, "pitchenv_amount": -8.0,
        **reverb(size=0.28, damp=0.65, mix=0.14),
    }),

    ("Acid Ping", "PLUCK", "ACID", {
        # SPECTRAL table at high position = inharmonic overtones.
        # Long resonant decay, pitch +9st on strike.
        "osc1_table": SPECTRAL, "osc1_position": 0.65, "osc1_coarse": 0,
        "filter_type": LP24, "filter_cutoff": 7000.0, "filter_res": 0.55,
        "env1_attack": 0.001, "env1_decay": 0.8, "env1_sustain": 0.0,
        "env1_release": 1.0,  "env1_depth": 0.3,
        "env2_attack": 0.001, "env2_decay": 1.2, "env2_sustain": 0.0,
        "env2_release": 1.5,
        "pitchenv_attack": 0.002, "pitchenv_amount": 9.0,
        **reverb(size=0.6, damp=0.45, mix=0.35),
    }),

    ("Gamelan", "PLUCK", "WORLD", {
        # SPECTRAL + SINE fifth. Gamelan inharmonics from the spectral table.
        "osc1_table": SPECTRAL, "osc1_position": 0.5, "osc1_coarse": 0,
        "osc2_table": SINE, "osc2_coarse": 7, "osc2_level": 0.55,
        "osc_mix": 0.45,
        "filter_type": LP12, "filter_cutoff": 12000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.001, "env2_decay": 1.5, "env2_sustain": 0.0,
        "env2_release": 2.0,
        **reverb(size=0.6, damp=0.45, mix=0.35),
        **delay(ms=320.0, feedback=0.18, mix=0.12),
    }),

    ("Chiptune", "PLUCK", "8BIT", {
        # Pure SQUARE. No filter. No reverb. Old-school chip purity.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.9,
        "filter_type": LP12, "filter_cutoff": 18000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.001, "env2_decay": 0.18, "env2_sustain": 0.3,
        "env2_release": 0.3,
        **delay(ms=200.0, feedback=0.22, mix=0.2),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # STAB (8) — short punchy chord hits
    # ══════════════════════════════════════════════════════════════════════════

    ("House Stab", "STAB", "HOUSE", {
        # 3-voice SAW. baseCutoff=3500, depth=0.6 → peaks at 5597Hz.
        # Chorus gives it the classic house "open piano" feel.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_3, "osc1_detune": 0.25, "osc1_width": 0.8,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.18,
        "env1_attack": 0.002, "env1_decay": 0.15, "env1_sustain": 0.0,
        "env1_release": 0.2,  "env1_depth": 0.6,
        "env2_attack": 0.002, "env2_decay": 0.25, "env2_sustain": 0.0,
        "env2_release": 0.25,
        **chorus(rate=0.7, depth=0.35, mix=0.3),
        **reverb(size=0.5, damp=0.5, mix=0.28),
    }),

    ("Rave Brass Stab", "STAB", "RAVE", {
        # 4-voice SAW. Open filter (5500Hz). Pitch blip +10st on every hit.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_4, "osc1_detune": 0.3, "osc1_width": 0.9,
        "filter_type": LP24, "filter_cutoff": 5500.0, "filter_res": 0.15,
        "env1_attack": 0.003, "env1_decay": 0.2,  "env1_sustain": 0.0,
        "env1_release": 0.25, "env1_depth": 0.3,
        "env2_attack": 0.003, "env2_decay": 0.3, "env2_sustain": 0.0,
        "env2_release": 0.3,
        "pitchenv_attack": 0.004, "pitchenv_amount": 10.0,
        **reverb(size=0.55, damp=0.45, mix=0.32),
    }),

    ("Techno Hit", "STAB", "TECHNO", {
        # Hard clip distortion + 2-voice SAW. Sharp and metallic.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.2, "osc1_width": 0.7,
        "filter_type": LP24, "filter_cutoff": 2800.0, "filter_res": 0.38,
        "filter_drive": 0.25,
        "env1_attack": 0.001, "env1_decay": 0.1, "env1_sustain": 0.0,
        "env1_release": 0.1,  "env1_depth": 0.65,
        "env2_attack": 0.001, "env2_decay": 0.18, "env2_sustain": 0.0,
        "env2_release": 0.15,
        "fx1_type": 1,  # Hard Clip
        **dist(drive=0.35, tone=4500.0, mix=0.45),
    }),

    ("Electro Chord", "STAB", "ELECTRO", {
        # Cold SQUARE wave + SQUARE octave up. Minimal, precise.
        "osc1_table": SQUARE, "osc1_coarse": 0,
        "osc1_unison": UNI_2, "osc1_detune": 0.18,
        "osc2_table": SQUARE, "osc2_coarse": 12, "osc2_level": 0.5,
        "osc_mix": 0.38,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.28,
        "env1_attack": 0.002, "env1_decay": 0.1, "env1_sustain": 0.0,
        "env1_release": 0.1,  "env1_depth": 0.5,
        "env2_attack": 0.002, "env2_decay": 0.2, "env2_sustain": 0.0,
        "env2_release": 0.2,
        **reverb(size=0.38, damp=0.58, mix=0.2),
    }),

    ("Jungle Hit", "STAB", "JUNGLE", {
        # DnB stab. 3-voice SAW + pitch blip +8st. Fast and aggressive.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_3, "osc1_detune": 0.25, "osc1_width": 0.8,
        "filter_type": LP24, "filter_cutoff": 4500.0, "filter_res": 0.28,
        "env1_attack": 0.002, "env1_decay": 0.12, "env1_sustain": 0.0,
        "env1_release": 0.12, "env1_depth": 0.55,
        "env2_attack": 0.002, "env2_decay": 0.18, "env2_sustain": 0.0,
        "env2_release": 0.16,
        "pitchenv_attack": 0.003, "pitchenv_amount": 8.0,
        **dist(drive=0.18, tone=5000.0, mix=0.25),
        **reverb(size=0.38, damp=0.58, mix=0.18),
    }),

    ("Pad Stab", "STAB", "SOFT", {
        # 4-voice SAW with slower decay — softer stab for drops and breaks.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_4, "osc1_detune": 0.2, "osc1_width": 0.85,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.08,
        "env1_attack": 0.005, "env1_decay": 0.3, "env1_sustain": 0.0,
        "env1_release": 0.35, "env1_depth": 0.42,
        "env2_attack": 0.008, "env2_decay": 0.5, "env2_sustain": 0.0,
        "env2_release": 0.6,
        **chorus(rate=0.65, depth=0.4, mix=0.35),
        **reverb(size=0.6, damp=0.45, mix=0.38),
    }),

    ("Organ Hit", "STAB", "ORGAN", {
        # SQUARE root + SQUARE octave. Hard gate: sustain=1, release=0.05.
        # Sounds like Hammond chord stab — no attack, instant off.
        "osc1_table": SQUARE, "osc1_coarse": 0, "osc1_level": 0.85,
        "osc2_table": SQUARE, "osc2_coarse": 12, "osc2_level": 0.6,
        "osc_mix": 0.45,
        "filter_type": LP12, "filter_cutoff": 10000.0, "filter_res": 0.0,
        "env1_depth": 0.0,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0,
        "env2_release": 0.05,
        **dist(drive=0.12, tone=6000.0, mix=0.2),
        **reverb(size=0.4, damp=0.55, mix=0.22),
    }),

    ("EBM Stomp", "STAB", "EBM", {
        # Hard clip SAW 3-voice. Cold and brutal.
        "osc1_table": SAW, "osc1_coarse": 0,
        "osc1_unison": UNI_3, "osc1_detune": 0.3,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.42,
        "filter_drive": 0.35,
        "env1_attack": 0.003, "env1_decay": 0.12, "env1_sustain": 0.0,
        "env1_release": 0.15, "env1_depth": 0.55,
        "env2_attack": 0.003, "env2_decay": 0.18, "env2_sustain": 0.0,
        "env2_release": 0.18,
        "fx1_type": 1,  # Hard Clip
        **dist(drive=0.6, tone=3000.0, mix=0.65),
        **reverb(size=0.28, damp=0.68, mix=0.14),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # BASS — user list
    # ══════════════════════════════════════════════════════════════════════════

    ("Armada Bass", "BASS", "ELECTRO", {
        # Hard detuned saw, Unison 4. LP24 cutoff 1200Hz/res 0.7 → sweeps to 2037Hz.
        "osc1_table": SAW, "osc1_unison": UNI_4, "osc1_detune": 0.35, "osc1_width": 0.8,
        "filter_type": LP24, "filter_cutoff": 1200.0, "filter_res": 0.70, "filter_drive": 0.2,
        "env1_attack": 0.01, "env1_decay": 0.25, "env1_sustain": 0.2, "env1_release": 0.2, "env1_depth": 0.65,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.3,
        **mono(glide=0.06), **dist(drive=0.55, tone=3000.0, mix=0.6), **chorus(rate=0.5, depth=0.3, mix=0.2),
    }),

    ("Blast Bass", "BASS", "ELECTRO", {
        # Square wave, downward pitch blip on hit. No sustain — pure transient.
        "osc1_table": SQUARE,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.45,
        "env1_attack": 0.002, "env1_decay": 0.08, "env1_sustain": 0.0, "env1_release": 0.06, "env1_depth": 0.75,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        "pitchenv_attack": 0.003, "pitchenv_amount": -6.0,
        **mono(),
    }),

    ("Hydraulic Space", "BASS", "SCAN", {
        # SPECTRAL SCAN — mechanical/cosmic pressure. Slow filter swell.
        "osc1_table": SPECTRAL, "osc1_position": 0.08, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 800.0, "filter_res": 0.3,
        "env1_attack": 0.8, "env1_decay": 2.0, "env1_sustain": 0.6, "env1_release": 1.5, "env1_depth": 0.35,
        "env2_attack": 0.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 2.0,
        **lfo1(rate=0.05, depth=0.15), **mono(),
        **reverb(size=0.45, damp=0.6, mix=0.2), **delay(ms=300.0, feedback=0.2, mix=0.12),
    }),

    ("Panic Bass", "BASS", "TECHNO", {
        # SAW near self-oscillation resonance. LFO2 adds filter tremor at 4Hz.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.88, "filter_drive": 0.3,
        "env1_attack": 0.003, "env1_decay": 0.12, "env1_sustain": 0.0, "env1_release": 0.1, "env1_depth": 0.8,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.15,
        **lfo2(rate=4.0, depth=0.1), **mono(), **dist(drive=0.55, tone=3000.0, mix=0.6),
    }),

    ("Electrobase 1", "BASS", "ELECTRO", {
        # Clean punchy electro SAW — the utility foundation.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1200.0, "filter_res": 0.4,
        "env1_attack": 0.005, "env1_decay": 0.18, "env1_sustain": 0.15, "env1_release": 0.12, "env1_depth": 0.55,
        "env2_attack": 0.004, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.18,
        **mono(), **dist(drive=0.15, tone=5000.0, mix=0.2),
    }),

    ("Electrobase 2", "BASS", "ELECTRO", {
        # Dual SAW with 8-cent detune. Fatter and wider than Electrobase 1.
        "osc1_table": SAW, "osc2_table": SAW, "osc2_fine": 8.0, "osc_mix": 0.45,
        "filter_type": LP24, "filter_cutoff": 1300.0, "filter_res": 0.35,
        "env1_attack": 0.005, "env1_decay": 0.2, "env1_sustain": 0.2, "env1_release": 0.15, "env1_depth": 0.5,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        **mono(glide=0.025), **chorus(rate=0.6, depth=0.25, mix=0.2),
    }),

    ("Electrobase 3", "BASS", "ELECTRO", {
        # Unison 2 SAW — dirtiest of the three. Sharp filter snap + drive.
        "osc1_table": SAW, "osc1_unison": UNI_2, "osc1_detune": 0.15,
        "filter_type": LP24, "filter_cutoff": 1400.0, "filter_res": 0.65,
        "env1_attack": 0.003, "env1_decay": 0.12, "env1_sustain": 0.0, "env1_release": 0.09, "env1_depth": 0.75,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.18,
        **mono(), **dist(drive=0.35, tone=4000.0, mix=0.45),
    }),

    ("Start Bass", "BASS", "SUB", {
        # Pure SINE sub. No filter movement, no unison. Massive low-end foundation.
        "osc1_table": SINE,
        "filter_type": LP24, "filter_cutoff": 90.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 0.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 1.5,
        **mono(), **reverb(size=0.4, damp=0.7, mix=0.15),
    }),

    ("Reiss 5000", "BASS", "TECHNO", {
        # BASS01 wavetable, downward pitch blip. Roland-inspired mechanical weight.
        "osc1_table": BASS01,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.6,
        "env1_attack": 0.003, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.15, "env1_depth": 0.75,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        "pitchenv_attack": 0.003, "pitchenv_amount": -5.0,
        **mono(glide=0.025), **dist(drive=0.5, tone=3500.0, mix=0.55),
    }),

    ("Boeheim Bass", "BASS", "ACID", {
        # SAW near-max resonance. Acid squelch with modern edge.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2000.0, "filter_res": 0.88, "filter_drive": 0.2,
        "env1_attack": 0.002, "env1_decay": 0.09, "env1_sustain": 0.0, "env1_release": 0.06, "env1_depth": 0.88,
        "env2_attack": 0.002, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.12,
        **mono(), **dist(drive=0.25, tone=5000.0, mix=0.3),
    }),

    ("Wavetable Flux", "BASS", "SCAN", {
        # SPECTRAL SCAN with LFO1. Slow-morphing flux texture over the phrase.
        "osc1_table": SPECTRAL, "osc1_position": 0.15, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 800.0, "filter_res": 0.25,
        "env1_attack": 0.5, "env1_decay": 2.0, "env1_sustain": 0.6, "env1_release": 1.5, "env1_depth": 0.3,
        "env2_attack": 0.3, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 1.5,
        **lfo1(rate=0.12, depth=0.2), **mono(),
        **chorus(rate=0.4, depth=0.35, mix=0.25), **reverb(size=0.45, damp=0.55, mix=0.22),
    }),

    ("Wobble Bass", "BASS", "WOBBLE", {
        # SAW + LFO2 at 5Hz → ±1400Hz filter swing around 1800Hz.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.65,
        "env1_attack": 0.005, "env1_decay": 0.3, "env1_sustain": 0.2, "env1_release": 0.2, "env1_depth": 0.3,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        **lfo2(rate=5.0, depth=0.35), **mono(), **dist(drive=0.3, tone=3500.0, mix=0.4),
    }),

    ("Wob Wob Bass", "BASS", "WOBBLE", {
        # Slower deeper wobble (2.5Hz), more pronounced than Wobble Bass.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1600.0, "filter_res": 0.78,
        "env1_attack": 0.005, "env1_decay": 0.3, "env1_sustain": 0.2, "env1_release": 0.2, "env1_depth": 0.25,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.25,
        **lfo2(rate=2.5, depth=0.45), **mono(glide=0.03), **dist(drive=0.5, tone=3000.0, mix=0.55),
    }),

    ("Funk Bass", "BASS", "FUNK", {
        # BASS02 with snap-pluck filter ENV (65ms decay, no sustain). Grooves.
        "osc1_table": BASS02,
        "filter_type": LP24, "filter_cutoff": 2000.0, "filter_res": 0.45, "filter_drive": 0.15,
        "env1_attack": 0.001, "env1_decay": 0.06, "env1_sustain": 0.0, "env1_release": 0.05, "env1_depth": 0.75,
        "env2_attack": 0.001, "env2_decay": 0.45, "env2_sustain": 0.0, "env2_release": 0.3,
        **mono(), **dist(drive=0.12, tone=4500.0, mix=0.2),
    }),

    ("Electro Funk Bass", "BASS", "FUNK", {
        # SAW + upward pitch blip + slight chorus. Machine-funk crossover.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.55,
        "env1_attack": 0.002, "env1_decay": 0.07, "env1_sustain": 0.0, "env1_release": 0.06, "env1_depth": 0.72,
        "env2_attack": 0.002, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.18,
        "pitchenv_attack": 0.003, "pitchenv_amount": 5.0,
        **mono(glide=0.015), **chorus(rate=0.6, depth=0.25, mix=0.18), **dist(drive=0.15, tone=4500.0, mix=0.2),
    }),

    ("Boom Bass", "BASS", "SUB", {
        # SINE + BASS01 sub layer. Large +16st pitch blip fires the boom thud.
        "osc1_table": SINE, "osc2_table": BASS01, "osc2_level": 0.7, "osc_mix": 0.3,
        "filter_type": LP24, "filter_cutoff": 80.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 0.05, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 2.0,
        "pitchenv_attack": 0.003, "pitchenv_amount": 16.0,
        **mono(), **reverb(size=0.3, damp=0.8, mix=0.08),
    }),

    ("Retox Bass", "BASS", "ACID", {
        # Maximum resonance acid + heavy distortion + subtle LFO2 flutter.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2200.0, "filter_res": 0.92, "filter_drive": 0.3,
        "env1_attack": 0.002, "env1_decay": 0.09, "env1_sustain": 0.0, "env1_release": 0.07, "env1_depth": 0.9,
        "env2_attack": 0.002, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.12,
        **lfo2(rate=3.0, depth=0.08), **mono(), **dist(drive=0.6, tone=3000.0, mix=0.65),
    }),

    ("Itsy Bitsy Bass", "BASS", "ELECTRO", {
        # SQUARE pitched +3st. Small but buzzy — surprising punch in a mix.
        "osc1_table": SQUARE, "osc1_coarse": 3,
        "filter_type": LP24, "filter_cutoff": 2000.0, "filter_res": 0.4,
        "env1_attack": 0.002, "env1_decay": 0.08, "env1_sustain": 0.0, "env1_release": 0.06, "env1_depth": 0.65,
        "env2_attack": 0.002, "env2_decay": 0.25, "env2_sustain": 0.0, "env2_release": 0.2,
        **mono(),
    }),

    ("Hyperbase", "BASS", "TECHNO", {
        # Near-instant filter ENV, rapid downward pitch blip. Ultra-fast tempos.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.72,
        "env1_attack": 0.001, "env1_decay": 0.05, "env1_sustain": 0.0, "env1_release": 0.04, "env1_depth": 0.85,
        "env2_attack": 0.001, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.1,
        "pitchenv_attack": 0.001, "pitchenv_amount": -5.0,
        **mono(), **dist(drive=0.55, tone=3000.0, mix=0.6),
    }),

    ("Block Out Bass", "BASS", "TECHNO", {
        # SQUARE Unison 2. Dense concrete weight — medium-attack gate shape.
        "osc1_table": SQUARE, "osc1_unison": UNI_2, "osc1_detune": 0.15,
        "filter_type": LP24, "filter_cutoff": 1400.0, "filter_res": 0.65, "filter_drive": 0.25,
        "env1_attack": 0.02, "env1_decay": 0.25, "env1_sustain": 0.3, "env1_release": 0.2, "env1_depth": 0.65,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        **mono(glide=0.07), **dist(drive=0.55, tone=3000.0, mix=0.6),
    }),

    ("Terminal Bass", "BASS", "DARK", {
        # BASS02 SCAN mode, very slow movement. Ominous, end-of-the-line energy.
        "osc1_table": BASS02, "osc1_position": 0.06, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP24, "filter_cutoff": 600.0, "filter_res": 0.55,
        "env1_attack": 2.0, "env1_decay": 4.0, "env1_sustain": 0.5, "env1_release": 3.0, "env1_depth": 0.4,
        "env2_attack": 2.0, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 4.0,
        **lfo1(rate=0.03, depth=0.15), **mono(), **reverb(size=0.65, damp=0.4, mix=0.35),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # STAB — user list
    # ══════════════════════════════════════════════════════════════════════════

    ("Acid Stab", "STAB", "ACID", {
        # Classic 303-squelch stab. Max resonance, instant filter spike.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2200.0, "filter_res": 0.90,
        "env1_attack": 0.001, "env1_decay": 0.07, "env1_sustain": 0.0, "env1_release": 0.05, "env1_depth": 0.90,
        "env2_attack": 0.001, "env2_decay": 0.1, "env2_sustain": 0.0, "env2_release": 0.08,
        **mono(), **dist(drive=0.2, tone=4500.0, mix=0.25),
    }),

    ("Daft Stab", "STAB", "ELECTRO", {
        # French house SAW + 10-cent layer. Chorus gives it the compressed width.
        "osc1_table": SAW, "osc2_table": SAW, "osc2_fine": 10.0, "osc_mix": 0.4,
        "filter_type": LP24, "filter_cutoff": 2000.0, "filter_res": 0.35,
        "env1_attack": 0.003, "env1_decay": 0.14, "env1_sustain": 0.0, "env1_release": 0.12, "env1_depth": 0.6,
        "env2_attack": 0.003, "env2_decay": 0.22, "env2_sustain": 0.0, "env2_release": 0.2,
        **mono(), **chorus(rate=0.7, depth=0.3, mix=0.22),
    }),

    ("Frankenstein Stab", "STAB", "DARK", {
        # SPECTRAL wavetable, lurching pitch blip, heavy drive. Stitched-together horror.
        "osc1_table": SPECTRAL, "osc1_position": 0.5,
        "filter_type": LP24, "filter_cutoff": 1800.0, "filter_res": 0.6, "filter_drive": 0.35,
        "env1_attack": 0.003, "env1_decay": 0.25, "env1_sustain": 0.0, "env1_release": 0.2, "env1_depth": 0.65,
        "env2_attack": 0.003, "env2_decay": 0.3, "env2_sustain": 0.0, "env2_release": 0.25,
        "pitchenv_attack": 0.004, "pitchenv_amount": 8.0,
        **mono(), **dist(drive=0.5, tone=3500.0, mix=0.55), **reverb(size=0.35, damp=0.65, mix=0.18),
    }),

    ("Hyperactive Stab", "STAB", "TECHNO", {
        # SAW, ultra-short decay, heavy drive. Machine-gun burst energy.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.7,
        "env1_attack": 0.001, "env1_decay": 0.06, "env1_sustain": 0.0, "env1_release": 0.05, "env1_depth": 0.75,
        "env2_attack": 0.001, "env2_decay": 0.08, "env2_sustain": 0.0, "env2_release": 0.06,
        **mono(), **dist(drive=0.5, tone=3500.0, mix=0.55),
    }),

    ("Storm Stab", "STAB", "TECHNO", {
        # SAW Unison 4 + delay + reverb. Sweeping breakdown impact.
        "osc1_table": SAW, "osc1_unison": UNI_4, "osc1_detune": 0.3, "osc1_width": 0.85,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.35,
        "env1_attack": 0.005, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.18, "env1_depth": 0.55,
        "env2_attack": 0.005, "env2_decay": 0.3, "env2_sustain": 0.0, "env2_release": 0.28,
        **mono(glide=0.02), **delay(ms=280.0, feedback=0.18, mix=0.18), **reverb(size=0.5, damp=0.5, mix=0.28),
    }),

    ("Search Stab", "STAB", "SCAN", {
        # SPECTRAL SCAN, LFO1 scanning. Radar-like — each hit probes a new timbre.
        "osc1_table": SPECTRAL, "osc1_position": 0.25, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 2000.0, "filter_res": 0.3,
        "env1_attack": 0.003, "env1_decay": 0.1, "env1_sustain": 0.0, "env1_release": 0.08, "env1_depth": 0.2,
        "env2_attack": 0.003, "env2_decay": 0.15, "env2_sustain": 0.0, "env2_release": 0.12,
        **lfo1(rate=2.0, depth=0.25), **mono(), **delay(ms=250.0, feedback=0.2, mix=0.18),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # PAD — user list
    # ══════════════════════════════════════════════════════════════════════════

    ("Jungle Pad", "PAD", "ORGANIC", {
        # FORMANT_A + FORMANT_E blend. Warm vowel-to-vowel movement with LFO1 scan.
        "osc1_table": FORMANT_A, "osc2_table": FORMANT_E, "osc2_level": 0.8, "osc_mix": 0.5,
        "filter_type": LP12, "filter_cutoff": 6000.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 2.0, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 4.5,
        **lfo1(rate=0.06, depth=0.2),
        **chorus(rate=0.4, depth=0.45, mix=0.38), **reverb(size=0.8, damp=0.35, mix=0.52),
    }),

    ("Synth King Pad", "PAD", "WIDE", {
        # SAW Unison 6 — throne-room width. Huge with heavy chorus.
        "osc1_table": SAW, "osc1_unison": UNI_6, "osc1_detune": 0.25, "osc1_width": 0.95,
        "filter_type": LP12, "filter_cutoff": 6000.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 2.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 5.0,
        **chorus(rate=0.55, depth=0.5, mix=0.45), **reverb(size=0.85, damp=0.35, mix=0.52),
    }),

    ("Smooth Pad", "PAD", "SOFT", {
        # SINE + SAW blend at 0.3 mix. Buttery bed under melody.
        "osc1_table": SINE, "osc2_table": SAW, "osc2_level": 0.8, "osc_mix": 0.3,
        "filter_type": LP12, "filter_cutoff": 8000.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 2.0, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 4.0,
        **chorus(rate=0.5, depth=0.4, mix=0.3), **reverb(size=0.7, damp=0.45, mix=0.45),
    }),

    ("String Pad", "PAD", "STRING", {
        # SAW Unison 4 + slight LFO2 filter sway. Classic synthetic strings.
        "osc1_table": SAW, "osc1_unison": UNI_4, "osc1_detune": 0.18, "osc1_width": 0.85,
        "filter_type": LP12, "filter_cutoff": 6500.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 0.4, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 2.5,
        **lfo2(rate=0.08, depth=0.06),
        **chorus(rate=0.65, depth=0.45, mix=0.38), **reverb(size=0.7, damp=0.45, mix=0.42),
    }),

    ("Crossbones Pad", "PAD", "DARK", {
        # SPECTRAL SCAN + very slow filter swell. Foreboding skeletal dread.
        "osc1_table": SPECTRAL, "osc1_position": 0.08, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP24, "filter_cutoff": 1500.0, "filter_res": 0.2, "filter_drive": 0.1,
        "env1_attack": 3.0, "env1_decay": 5.0, "env1_sustain": 0.4, "env1_release": 4.0, "env1_depth": 0.4,
        "env2_attack": 3.0, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 6.0,
        **lfo1(rate=0.03, depth=0.15),
        **dist(drive=0.1, tone=3000.0, mix=0.15), **reverb(size=0.9, damp=0.3, mix=0.58),
    }),

    ("Tooth Dust Pad", "PAD", "NOISE", {
        # NOISE + SPECTRAL blend. Crumbling dusty grain texture.
        "osc1_table": NOISE, "osc2_table": SPECTRAL, "osc2_position": 0.4, "osc2_level": 0.7, "osc_mix": 0.6,
        "filter_type": LP12, "filter_cutoff": 3000.0, "filter_res": 0.0, "env1_depth": 0.0,
        "env2_attack": 2.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 5.0,
        **lfo2(rate=0.08, depth=0.12),
        **chorus(rate=0.35, depth=0.4, mix=0.3), **reverb(size=0.88, damp=0.3, mix=0.55),
    }),

    ("Daylight Robbery Pad", "PAD", "BRIGHT", {
        # SAW Unison 4 through HP12 — cuts the mud, leaves pure high-end boldness.
        "osc1_table": SAW, "osc1_unison": UNI_4, "osc1_detune": 0.22, "osc1_width": 0.9,
        "filter_type": HP12, "filter_cutoff": 300.0, "filter_res": 0.2, "env1_depth": 0.0,
        "env2_attack": 0.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 2.0,
        **lfo2(rate=0.15, depth=0.1),
        **chorus(rate=0.7, depth=0.4, mix=0.35), **delay(ms=340.0, feedback=0.3, mix=0.22),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # PLUCK — user list
    # ══════════════════════════════════════════════════════════════════════════

    ("Beam Force Pluck", "PLUCK", "ELECTRO", {
        # SAW, fast deep filter decay. Laser-precise beam transient.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 4000.0, "filter_res": 0.55,
        "env1_attack": 0.001, "env1_decay": 0.18, "env1_sustain": 0.0, "env1_release": 0.22, "env1_depth": 0.65,
        "env2_attack": 0.001, "env2_decay": 0.35, "env2_sustain": 0.0, "env2_release": 0.28,
        **mono(), **dist(drive=0.15, tone=5000.0, mix=0.22),
    }),

    ("Straight Pluck", "PLUCK", "CLEAN", {
        # SQUARE, completely dry. The no-nonsense utility workhorse.
        "osc1_table": SQUARE,
        "filter_type": LP24, "filter_cutoff": 4000.0, "filter_res": 0.4,
        "env1_attack": 0.001, "env1_decay": 0.15, "env1_sustain": 0.0, "env1_release": 0.12, "env1_depth": 0.65,
        "env2_attack": 0.001, "env2_decay": 0.4, "env2_sustain": 0.0, "env2_release": 0.3,
        **mono(),
    }),

    ("Electro Pluck", "PLUCK", "ELECTRO", {
        # SAW + 8-cent layer. Chorus + delay for classic electro sequence width.
        "osc1_table": SAW, "osc2_table": SAW, "osc2_fine": 8.0, "osc_mix": 0.35,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.48,
        "env1_attack": 0.001, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.15, "env1_depth": 0.7,
        "env2_attack": 0.001, "env2_decay": 0.5, "env2_sustain": 0.0, "env2_release": 0.4,
        **mono(), **chorus(rate=0.6, depth=0.3, mix=0.2), **delay(ms=250.0, feedback=0.18, mix=0.12),
    }),

    ("Donk Pluck", "PLUCK", "DONK", {
        # BASS01 + +12st pitch blip = the characteristic UK donk bounce.
        "osc1_table": BASS01,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.65, "filter_drive": 0.15,
        "env1_attack": 0.001, "env1_decay": 0.12, "env1_sustain": 0.0, "env1_release": 0.1, "env1_depth": 0.72,
        "env2_attack": 0.001, "env2_decay": 0.4, "env2_sustain": 0.0, "env2_release": 0.3,
        "pitchenv_attack": 0.003, "pitchenv_amount": 12.0,
        **mono(), **dist(drive=0.25, tone=4500.0, mix=0.3),
    }),

    ("Laser Pluck", "PLUCK", "SCI-FI", {
        # SPECTRAL high cutoff + pitch fires high then drops = zapping laser sweep.
        "osc1_table": SPECTRAL, "osc1_position": 0.6,
        "filter_type": LP24, "filter_cutoff": 6000.0, "filter_res": 0.45,
        "env1_attack": 0.001, "env1_decay": 0.35, "env1_sustain": 0.0, "env1_release": 0.3, "env1_depth": 0.55,
        "env2_attack": 0.001, "env2_decay": 0.45, "env2_sustain": 0.0, "env2_release": 0.35,
        "pitchenv_attack": 0.001, "pitchenv_amount": 10.0,
        **mono(), **delay(ms=220.0, feedback=0.15, mix=0.15),
    }),

    # ══════════════════════════════════════════════════════════════════════════
    # LEAD — user list
    # ══════════════════════════════════════════════════════════════════════════

    ("Acid Lead", "LEAD", "ACID", {
        # SAW near-max resonance, pitch push +8st. Classic squelching acid lead.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.88,
        "env1_attack": 0.003, "env1_decay": 0.18, "env1_sustain": 0.0, "env1_release": 0.12, "env1_depth": 0.8,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        "pitchenv_attack": 0.004, "pitchenv_amount": 8.0,
        **mono(glide=0.03), **dist(drive=0.22, tone=5000.0, mix=0.3),
    }),

    ("Super Sore Lead", "LEAD", "AGGRESSIVE", {
        # SAW Unison 2 + max drive. Abrasive, painful in the best way.
        "osc1_table": SAW, "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "filter_type": LP24, "filter_cutoff": 2800.0, "filter_res": 0.7, "filter_drive": 0.35,
        "env1_attack": 0.003, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.15, "env1_depth": 0.72,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.25,
        "pitchenv_attack": 0.004, "pitchenv_amount": 10.0,
        **mono(glide=0.03), **dist(drive=0.6, tone=3500.0, mix=0.65),
    }),

    ("Juno Lead", "LEAD", "VINTAGE", {
        # SAW + heavy chorus (emulating Juno BBD). Warm 80s analogue sweetness.
        "osc1_table": SAW,
        "filter_type": LP12, "filter_cutoff": 4000.0, "filter_res": 0.1,
        "env1_attack": 0.01, "env1_decay": 0.4, "env1_sustain": 0.7, "env1_release": 0.35, "env1_depth": 0.2,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.4,
        **mono(), **chorus(rate=0.8, depth=0.5, mix=0.45),
    }),

    ("Oberheim Lead", "LEAD", "VINTAGE", {
        # Two SAW 7-cents apart + glide. Thick creamy American synth character.
        "osc1_table": SAW, "osc2_table": SAW, "osc2_fine": 7.0, "osc_mix": 0.5,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.25,
        "env1_attack": 0.015, "env1_decay": 0.5, "env1_sustain": 0.7, "env1_release": 0.5, "env1_depth": 0.35,
        "env2_attack": 0.015, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.6,
        **mono(glide=0.07), **reverb(size=0.35, damp=0.6, mix=0.2),
    }),

    ("Exoplanet Lead", "LEAD", "SCI-FI", {
        # SPECTRAL SCAN + long glide + huge reverb. Alien, floating through deep space.
        "osc1_table": SPECTRAL, "osc1_position": 0.12, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 4000.0, "filter_res": 0.05, "env1_depth": 0.0,
        "env2_attack": 1.5, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 3.5,
        **lfo1(rate=0.06, depth=0.15), **mono(glide=0.2),
        **reverb(size=0.85, damp=0.3, mix=0.55), **delay(ms=400.0, feedback=0.4, mix=0.3),
    }),

    ("Vert Lead", "LEAD", "TECHNO", {
        # SAW, sharp upward pitch stab +12st. Cuts through a mix like a blade.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2500.0, "filter_res": 0.72,
        "env1_attack": 0.003, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.15, "env1_depth": 0.75,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.25,
        "pitchenv_attack": 0.004, "pitchenv_amount": 12.0,
        **mono(), **dist(drive=0.3, tone=4500.0, mix=0.38),
    }),

    ("Wavetable Synth 1", "LEAD", "SCAN", {
        # SPECTRAL SCAN at 0.5Hz — slow, smooth, evolving melodic character.
        "osc1_table": SPECTRAL, "osc1_position": 0.06, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 5000.0, "filter_res": 0.08,
        "env1_attack": 0.01, "env1_decay": 0.4, "env1_sustain": 0.8, "env1_release": 0.35, "env1_depth": 0.2,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.4,
        **lfo1(rate=0.5, depth=0.2), **mono(),
        **chorus(rate=0.5, depth=0.35, mix=0.25), **reverb(size=0.5, damp=0.5, mix=0.25),
    }),

    ("Wavetable Synth 2", "LEAD", "SCAN", {
        # SPECTRAL SCAN at 2Hz — rhythmic scan pulse, more animated than Synth 1.
        "osc1_table": SPECTRAL, "osc1_position": 0.15, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP12, "filter_cutoff": 4000.0, "filter_res": 0.2,
        "env1_attack": 0.008, "env1_decay": 0.35, "env1_sustain": 0.6, "env1_release": 0.3, "env1_depth": 0.3,
        "env2_attack": 0.008, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.35,
        **lfo1(rate=2.0, depth=0.25), **mono(),
        **dist(drive=0.15, tone=5000.0, mix=0.2), **delay(ms=320.0, feedback=0.25, mix=0.18),
    }),

    ("Wavetable Synth 3", "LEAD", "SCAN", {
        # SPECTRAL SCAN at 6Hz — FM-like buzz, restless, most aggressive scan lead.
        "osc1_table": SPECTRAL, "osc1_position": 0.22, "osc1_wt_mode": WT_SCAN,
        "filter_type": LP24, "filter_cutoff": 3500.0, "filter_res": 0.45,
        "env1_attack": 0.005, "env1_decay": 0.35, "env1_sustain": 0.4, "env1_release": 0.3, "env1_depth": 0.45,
        "env2_attack": 0.005, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.35,
        **lfo1(rate=6.0, depth=0.3), **mono(),
        **dist(drive=0.25, tone=4500.0, mix=0.3),
    }),

    ("Spectrum Lead", "LEAD", "BRIGHT", {
        # SPECTRAL Unison 3 + LFO2 shimmer. Wide, luminous, full-spectrum.
        "osc1_table": SPECTRAL, "osc1_position": 0.5, "osc1_unison": UNI_3, "osc1_detune": 0.15,
        "filter_type": LP12, "filter_cutoff": 7000.0, "filter_res": 0.05, "env1_depth": 0.0,
        "env2_attack": 0.01, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.5,
        **lfo2(rate=0.06, depth=0.08), **mono(),
        **chorus(rate=0.55, depth=0.35, mix=0.22), **reverb(size=0.55, damp=0.45, mix=0.28),
    }),

    ("Retox Lead", "LEAD", "ACID", {
        # SAW max resonance + maximum drive + pitch venom snap. Corrosive acid.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 2800.0, "filter_res": 0.92, "filter_drive": 0.3,
        "env1_attack": 0.002, "env1_decay": 0.15, "env1_sustain": 0.0, "env1_release": 0.12, "env1_depth": 0.85,
        "env2_attack": 0.002, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        "pitchenv_attack": 0.004, "pitchenv_amount": 10.0,
        **mono(), **dist(drive=0.65, tone=3000.0, mix=0.7),
    }),

    ("Hyperactive Lead", "LEAD", "TECHNO", {
        # SAW instant filter spike + pitch micro-blip. Machine-gun, no glide.
        "osc1_table": SAW,
        "filter_type": LP24, "filter_cutoff": 3000.0, "filter_res": 0.72,
        "env1_attack": 0.001, "env1_decay": 0.07, "env1_sustain": 0.0, "env1_release": 0.06, "env1_depth": 0.78,
        "env2_attack": 0.001, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.2,
        "pitchenv_attack": 0.001, "pitchenv_amount": 8.0,
        **mono(), **dist(drive=0.45, tone=3500.0, mix=0.5),
    }),

    ("Brock Out Lead", "LEAD", "AGGRESSIVE", {
        # SAW Unison 2 + LFO2 filter aggression + delay. Raw breaking-point energy.
        "osc1_table": SAW, "osc1_unison": UNI_2, "osc1_detune": 0.2,
        "filter_type": LP24, "filter_cutoff": 2800.0, "filter_res": 0.68, "filter_drive": 0.3,
        "env1_attack": 0.003, "env1_decay": 0.2, "env1_sustain": 0.0, "env1_release": 0.15, "env1_depth": 0.7,
        "env2_attack": 0.003, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 0.25,
        **lfo2(rate=4.0, depth=0.22), **mono(glide=0.03),
        **dist(drive=0.55, tone=3500.0, mix=0.6), **delay(ms=300.0, feedback=0.25, mix=0.18),
    }),
]

# ─── XML serialisation ────────────────────────────────────────────────────────

def fmt(key, val):
    if key in INT_PARAMS:
        return str(int(round(float(val))))
    s = f"{float(val):.10g}"
    if "." not in s and "e" not in s:
        s += ".0"
    return s

def write_preset(out_dir, name, category, sub, overrides, dry_run=False):
    params = dict(DEFAULTS)
    for k, v in overrides.items():
        if k not in DEFAULTS:
            print(f"  [WARN] unknown param '{k}' in '{name}'")
            continue
        params[k] = v

    if dry_run:
        print(f"\n  ── {name}  [{category}/{sub}]")
        for k, v in sorted(overrides.items()):
            if k in DEFAULTS:
                print(f"    {k:30s} = {v}")
        return

    root = ET.Element("Parameters")
    root.set("_name",     name)
    root.set("_category", category.upper())
    root.set("_sub",      sub.upper())
    for k in DEFAULTS:
        root.set(k, fmt(k, params[k]))

    cat_dir = out_dir / category.upper()
    cat_dir.mkdir(parents=True, exist_ok=True)
    fn = name.replace(" ", "_").replace("/", "-") + ".vwpreset"
    ET.ElementTree(root).write(str(cat_dir / fn), xml_declaration=True, encoding="UTF-8")
    print(f"  {category}/{fn}")

# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    script_dir = Path(__file__).parent
    out_dir    = script_dir / "Resources" / "Presets" / "factory"
    dry_run    = False

    for arg in sys.argv[1:]:
        if arg.startswith("--outdir="):
            out_dir = Path(arg.split("=", 1)[1])
        elif arg == "--dry-run":
            dry_run = True

    cats = {}
    for name, cat, *_ in RECIPES:
        cats.setdefault(cat, 0)
        cats[cat] += 1
    summary = "  ".join(f"{c}:{n}" for c, n in sorted(cats.items()))

    if dry_run:
        print(f"DRY RUN — {len(RECIPES)} recipes  {summary}\n")
    else:
        print(f"Writing {len(RECIPES)} recipes → {out_dir}\n{summary}\n")

    for name, category, sub, overrides in RECIPES:
        write_preset(out_dir, name, category, sub, overrides, dry_run)

    if not dry_run:
        print(f"\n✓  {len(RECIPES)} presets written")

if __name__ == "__main__":
    main()
