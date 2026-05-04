#!/usr/bin/env python3
"""
generate_awesome_presets.py  ·  VOID WAVE ultra-complex presets
─────────────────────────────────────────────────────────────────
Showcases the absolute limits of the synth using the modulation matrix,
FM, extreme unisons, and macro control mapping.
"""
import sys
from pathlib import Path
from xml.etree import ElementTree as ET

# ─── Symbolic constants ───────────────────────────────────────────────────────
SINE, SAW, SQUARE, SPECTRAL, FORMANT_A, FORMANT_E, BASS01, BASS02, NOISE = range(9)
LP12, LP24, HP12, HP24, BP, NOTCH = 0, 1, 2, 3, 4, 5
SINE_LFO, TRI_LFO, SAW_LFO, RAMP_LFO, SQ_LFO = 0, 1, 2, 3, 4
UNI_1, UNI_2, UNI_3, UNI_4, UNI_6, UNI_8 = 0, 1, 2, 3, 4, 5
WT_SINGLE, WT_MORPH, WT_SCAN = 0, 1, 2

# Mod Sources
M_ENV1, M_ENV2, M_ENV3, M_LFO1, M_LFO2 = 0, 1, 2, 3, 4
M_VEL, M_NOTE, M_AT, M_MW, M_BREATH = 5, 6, 7, 8, 9
M_MAC1, M_MAC2, M_MAC3, M_MAC4 = 10, 11, 12, 13

# Mod Destinations
D_OSC1_PITCH, D_OSC2_PITCH = 0, 1
D_OSC1_WT_POS, D_OSC2_WT_POS = 2, 3
D_OSC1_LEVEL, D_OSC2_LEVEL = 4, 5
D_OSC1_PAN, D_OSC2_PAN = 6, 7
D_OSC1_DETUNE, D_OSC2_DETUNE = 8, 9
D_FILT_CUTOFF, D_FILT_RES, D_FILT_DRIVE = 10, 11, 12
D_LFO1_RATE, D_LFO2_RATE = 13, 14
D_LFO1_DEPTH, D_LFO2_DEPTH = 15, 16
D_ENV1_DEPTH = 17
D_AMP = 18

# ─── Parameter defaults ───────────────────────────────────────────────────────
DEFAULTS = {
    "osc1_table": SINE,      "osc1_position": 0.0,   "osc1_coarse": 0,
    "osc1_fine":  0.0,       "osc1_level":    0.8,   "osc1_pan":    0.0,
    "osc1_unison": UNI_1,    "osc1_detune":   0.0,   "osc1_phase_mode": 0,
    "osc1_wt_mode": WT_SINGLE, "osc1_width":  1.0,
    "osc2_table": SINE,      "osc2_position": 0.0,   "osc2_coarse": 0,
    "osc2_fine":  0.0,       "osc2_level":    0.8,   "osc2_pan":    0.0,
    "osc2_unison": UNI_1,    "osc2_detune":   0.0,   "osc2_phase_mode": 0,
    "osc2_wt_mode": WT_SINGLE, "osc2_width":  1.0,
    "osc_mix": 0.5,          "osc_mix_mode":  0,     "osc_fm_ratio": 1.0,
    "pitchenv_attack": 0.01, "pitchenv_amount": 0.0,
    "filter_type": LP24,     "filter_cutoff": 5000.0, "filter_res":  0.0,
    "filter_drive": 0.0,     "filter_keytrack": 0.0,  "filter_veltrack": 0.0,
    "env1_attack": 0.01,  "env1_decay":  0.2,  "env1_sustain": 0.5,
    "env1_hold":   0.0,   "env1_release": 0.3, "env1_curve":   1,
    "env1_depth":  0.5,   "env1_loop":   0,    "env1_vel_sens": 0.5,
    "env2_attack": 0.01,  "env2_decay":  0.2,  "env2_sustain": 0.8,
    "env2_hold":   0.0,   "env2_release": 0.4, "env2_curve":   1,
    "env2_loop":   0,     "env2_vel_sens": 0.5,
    "env3_attack": 0.01,  "env3_decay":  0.1,  "env3_sustain": 0.7,
    "env3_hold":   0.0,   "env3_release": 0.3, "env3_curve":   1,
    "env3_loop":   0,     "env3_vel_sens": 0.5,
    "lfo1_shape": SINE_LFO, "lfo1_rate": 1.0,  "lfo1_tempo_sync": 0,
    "lfo1_sync_div": 0.5,   "lfo1_phase": 0.0, "lfo1_trigger": 0,
    "lfo1_fade": 0.0,        "lfo1_depth": 0.5,
    "lfo2_shape": SINE_LFO, "lfo2_rate": 1.0,  "lfo2_tempo_sync": 0,
    "lfo2_sync_div": 0.5,   "lfo2_phase": 0.0, "lfo2_trigger": 0,
    "lfo2_fade": 0.0,        "lfo2_depth": 0.5,
    **{f"mod{i}_source":  0   for i in range(1, 13)},
    **{f"mod{i}_dest":    0   for i in range(1, 13)},
    **{f"mod{i}_depth":   0.0 for i in range(1, 13)},
    **{f"mod{i}_enabled": 0   for i in range(1, 13)},
    "macro1": 0.0, "macro2": 0.0, "macro3": 0.0, "macro4": 0.0,
    "voice_poly": 8,    "voice_glide": 0.0, "voice_glide_mode": 1,
    "voice_pitch_bend": 2, "master_tune": 0.0, "master_volume": 0.8,
    "fx1_type": 0,  "fx1_param1": 0.0,   "fx1_param2": 8000.0, "fx1_param3": 0.0,
    "fx2_type": 0,  "fx2_param1": 1.0,   "fx2_param2": 0.5,    "fx2_param3": 0.0,
    "fx3_type": 1,  "fx3_param1": 375.0, "fx3_param2": 0.4,    "fx3_param3": 0.0,
    "fx3_sync": 0,
    "fx4_param1": 0.6, "fx4_param2": 0.5, "fx4_param3": 0.0, "fx4_param4": 0.0,
}
INT_PARAMS = {k for k, v in DEFAULTS.items() if isinstance(v, int)}

# ─── Helpers ──────────────────────────────────────────────────────────────────
def mod(slot, source, dest, depth):
    return {
        f"mod{slot}_source": source,
        f"mod{slot}_dest": dest,
        f"mod{slot}_depth": depth,
        f"mod{slot}_enabled": 1
    }

def fm_mode(ratio=2.0, mix=0.5):
    # osc_mix_mode: 3 is usually FM based on generate_recipes.py
    return {"osc_mix_mode": 3, "osc_fm_ratio": ratio, "osc_mix": mix}

def ring_mode(mix=0.5):
    # osc_mix_mode: 1 or 2 is usually Ring/Sync. Let's assume 1 is Ring.
    return {"osc_mix_mode": 1, "osc_mix": mix}

# ─── AWESOME RECIPES ──────────────────────────────────────────────────────────
RECIPES = [
    ("Hypernova", "AWESOME", "LEAD", {
        # Insane unison lead with FM and macro modulation
        "osc1_table": SAW, "osc1_unison": UNI_8, "osc1_detune": 0.35, "osc1_width": 1.0,
        "osc2_table": SPECTRAL, "osc2_coarse": 12, "osc2_unison": UNI_4, "osc2_detune": 0.2,
        **fm_mode(ratio=3.0, mix=0.3),
        "filter_type": BP, "filter_cutoff": 2000.0, "filter_res": 0.5, "filter_drive": 0.4,
        "env1_attack": 0.05, "env1_decay": 0.4, "env1_sustain": 0.3, "env1_depth": 0.8,
        "env2_attack": 0.01, "env2_sustain": 1.0, "env2_release": 0.6,
        "lfo1_rate": 2.5, "lfo1_depth": 0.5,
        "lfo2_rate": 0.1, "lfo2_depth": 0.8,
        **mod(1, M_LFO1, D_OSC1_WT_POS, 0.4),
        **mod(2, M_LFO2, D_FILT_CUTOFF, 0.6),
        **mod(3, M_MW, D_LFO1_RATE, 0.8),
        **mod(4, M_AT, D_FILT_RES, 0.5),
        "fx4_param1": 0.8, "fx4_param4": 0.4, # Huge reverb
        "fx2_param1": 0.5, "fx2_param2": 0.7, "fx2_param3": 0.5 # Heavy Chorus
    }),
    
    ("Mind Bender", "AWESOME", "SEQ", {
        # Self-modulating patch where LFOs modulate each other's rates
        "osc1_table": FORMANT_A, "osc1_wt_mode": WT_SCAN, "osc1_position": 0.2,
        "osc2_table": FORMANT_E, "osc2_wt_mode": WT_SCAN, "osc2_position": 0.8,
        "osc_mix": 0.5,
        "filter_type": HP12, "filter_cutoff": 400.0, "filter_res": 0.8,
        "lfo1_rate": 1.5, "lfo2_rate": 0.5,
        **mod(1, M_LFO1, D_OSC1_WT_POS, 0.7),
        **mod(2, M_LFO2, D_OSC2_WT_POS, -0.7),
        **mod(3, M_LFO1, D_LFO2_RATE, 0.5), # LFO cross-modulation!
        **mod(4, M_LFO2, D_LFO1_RATE, -0.5),
        **mod(5, M_ENV3, D_FILT_CUTOFF, 0.8),
        "env3_attack": 0.1, "env3_decay": 0.2, "env3_sustain": 0.0, "env3_loop": 1,
        "fx3_param1": 400.0, "fx3_param2": 0.6, "fx3_param3": 0.4 # Delay
    }),

    ("Abyssal Drone", "AWESOME", "PAD", {
        # Evolving cinematic drone with extreme reverb and slowly drifting parameters
        "osc1_table": SINE, "osc1_coarse": -24, "osc1_unison": UNI_6, "osc1_detune": 0.4,
        "osc2_table": NOISE, "osc2_level": 0.2,
        "osc_mix": 0.2,
        "filter_type": LP24, "filter_cutoff": 800.0, "filter_res": 0.3,
        "env2_attack": 4.0, "env2_sustain": 1.0, "env2_release": 8.0,
        "lfo1_rate": 0.05, "lfo2_rate": 0.03,
        **mod(1, M_LFO1, D_OSC1_DETUNE, 0.3),
        **mod(2, M_LFO2, D_FILT_CUTOFF, 0.4),
        **mod(3, M_MW, D_OSC2_LEVEL, 0.5), # Mod wheel brings in noise
        **mod(4, M_AT, D_FILT_DRIVE, 0.4), # Pressure adds grit
        "fx4_param1": 0.95, "fx4_param2": 0.2, "fx4_param4": 0.8, # Infinite verb
        "fx1_param1": 0.3, "fx1_param3": 0.2 # Slight distortion
    }),

    ("Cybernetic Bass", "AWESOME", "BASS", {
        # Brutal neuro bass utilizing FM, distortion, and fast envelope modulation
        "osc1_table": BASS01, "osc1_coarse": -12, "osc1_unison": UNI_4, "osc1_detune": 0.15,
        "osc2_table": SAW, "osc2_coarse": -12,
        **fm_mode(ratio=1.5, mix=0.4),
        "filter_type": LP24, "filter_cutoff": 300.0, "filter_res": 0.6, "filter_drive": 0.7,
        "env1_attack": 0.01, "env1_decay": 0.3, "env1_sustain": 0.1, "env1_depth": 0.9,
        "env2_attack": 0.01, "env2_sustain": 0.8, "env2_release": 0.4,
        "env3_attack": 0.05, "env3_decay": 0.1, "env3_sustain": 0.0, # Pluck envelope
        **mod(1, M_ENV3, D_OSC1_WT_POS, 0.8),
        **mod(2, M_LFO1, D_FILT_CUTOFF, 0.3),
        **mod(3, M_VEL, D_ENV1_DEPTH, 0.5), # Velocity to filter depth
        **mod(4, M_MW, D_LFO1_RATE, 0.9), # Mod wheel accelerates LFO wobble
        "lfo1_rate": 2.0, "lfo1_shape": SAW_LFO,
        "fx1_param1": 0.6, "fx1_param3": 0.7, # Heavy Dist
        "voice_poly": 1, "voice_glide": 0.05
    }),

    ("Expressive Pluck", "AWESOME", "PLUCK", {
        # Highly responsive acoustic-ish pluck, velocity maps to multiple parameters
        "osc1_table": SINE, "osc1_level": 0.9,
        "osc2_table": SQUARE, "osc2_coarse": 12, "osc2_level": 0.3,
        **ring_mode(mix=0.3),
        "filter_type": LP12, "filter_cutoff": 1000.0, "filter_res": 0.2,
        "env1_attack": 0.001, "env1_decay": 0.15, "env1_sustain": 0.0, "env1_depth": 0.8,
        "env2_attack": 0.001, "env2_decay": 0.8, "env2_sustain": 0.0,
        **mod(1, M_VEL, D_FILT_CUTOFF, 0.6),
        **mod(2, M_VEL, D_OSC2_LEVEL, 0.4),
        **mod(3, M_VEL, D_ENV1_DEPTH, 0.5),
        **mod(4, M_AT, D_OSC1_PITCH, 0.05), # Slight pitch bend on pressure
        "fx3_param1": 350.0, "fx3_param2": 0.4, "fx3_param3": 0.25, # Delay
        "fx4_param1": 0.6, "fx4_param4": 0.3
    }),

    ("Wormhole", "AWESOME", "SFX", {
        # Extreme pitch/filter modulation creating a sucking sound
        "osc1_table": NOISE, "osc1_level": 0.8,
        "osc2_table": SAW, "osc2_coarse": -24,
        "filter_type": BP, "filter_cutoff": 8000.0, "filter_res": 0.9, "filter_drive": 0.5,
        "env2_attack": 5.0, "env2_decay": 0.0, "env2_sustain": 1.0, "env2_release": 2.0,
        "env3_attack": 5.0, "env3_sustain": 1.0, # Slow rise
        **mod(1, M_ENV3, D_FILT_CUTOFF, -0.9), # Filter sweeps downwards
        **mod(2, M_ENV3, D_OSC2_PITCH, -0.8), # Pitch drops
        **mod(3, M_LFO1, D_FILT_RES, 0.6),
        **mod(4, M_LFO2, D_OSC1_PAN, 0.8), # Stereo swirling
        "lfo1_rate": 8.0, "lfo2_rate": 4.0,
        "fx4_param1": 0.9, "fx4_param4": 0.6
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

def write_preset(out_dir, name, category, sub, overrides):
    params = dict(DEFAULTS)
    for k, v in overrides.items():
        if k not in DEFAULTS:
            print(f"  [WARN] unknown param '{k}' in '{name}'")
            continue
        params[k] = v

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

def main():
    script_dir = Path(__file__).parent
    out_dir = script_dir / "Resources" / "Presets" / "factory"
    print(f"Writing AWESOME presets to {out_dir}\n")
    for name, category, sub, overrides in RECIPES:
        write_preset(out_dir, name, category, sub, overrides)
    print(f"\n✓ {len(RECIPES)} presets generated! Enjoy the awesomeness.")

if __name__ == "__main__":
    main()
