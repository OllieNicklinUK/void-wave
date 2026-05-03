#!/usr/bin/env python3
"""
Generate 420 dark-electro presets (60 per category × 7 categories).
Each sub-category has a distinct synthesis recipe to guarantee timbral variety.
Sub-categories are stored as the 'sub' field and written as _sub XML attribute.
"""
import json, math
from pathlib import Path

# ── helpers ──────────────────────────────────────────────────────────────────
def lerp(a, b, t): return a + (b - a) * t
def cycle(lst, n): return lst[n % len(lst)]
def spread(lo, hi, n, steps): return lo + (hi - lo) * n / max(steps - 1, 1)

# ── shared distortion palette ────────────────────────────────────────────────
DIST_SOFT   = {"fx1_param1": 0.0, "fx1_param3": 0.0}
DIST_GRIT   = {"fx1_param1": 0.3, "fx1_param3": 0.6}
DIST_HEAVY  = {"fx1_param1": 0.6, "fx1_param3": 0.9}
DIST_CRUNCH = {"fx1_param1": 0.45,"fx1_param3": 0.75}
REVERB_DRY  = {"fx4_param4": 0.0}
REVERB_ROOM = {"fx4_param1": 0.3, "fx4_param4": 0.25}
REVERB_HALL = {"fx4_param1": 0.7, "fx4_param4": 0.45}
REVERB_VAST = {"fx4_param1": 0.85,"fx4_param4": 0.65}
CHORUS_ON   = {"fx2_param1": 0.8, "fx2_param2": 0.5, "fx2_param3": 0.35}

# ── BASS presets (60) ─────────────────────────────────────────────────────────
# 7 sub-categories, ~8-9 each

BASS = []

# SUB (pure sine/tri low-end — no distortion, max mass)
for i in range(9):
    fc  = spread(60, 160, i, 9)
    atk = [0.08, 0.12, 0.05, 0.15, 0.2, 0.1, 0.06, 0.18, 0.09][i]
    BASS.append({"name": ["VOID SUB","IRON WEIGHT","CARBON FLOOR","DARK MASS","VOID CORE",
                           "DEEP GRAVITY","STEEL DEPTH","OBSIDIAN SUB","TUNGSTEN WEIGHT"][i],
                 "sub": "SUB",
                 "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":-12,"osc_mix":0.4,
                             "filter_cutoff":round(fc,1),"filter_res":0.0,
                             "env2_attack":atk,"env2_decay":0.8,"env2_sustain":1.0,"env2_release":0.5,
                             **DIST_SOFT, **REVERB_DRY}})

# REESE (detuned dual saw, filter motion)
REESE_CUTOFFS = [500,700,900,400,650,300,800,1100]
REESE_DETUNES = [0.35,0.45,0.25,0.5,0.3,0.4,0.28,0.42]
for i in range(8):
    BASS.append({"name": ["CORRODED REESE","CHROME VECTOR","OXIDE REESE","FLUX BASS",
                           "RAW REESE","WORN CHROME","RUST VECTOR","IRON FLUX"][i],
                 "sub": "REESE",
                 "params": {"osc1_table":1,"osc1_unison":5,"osc1_detune":REESE_DETUNES[i],
                             "osc2_table":1,"osc2_unison":4,"osc2_detune":round(REESE_DETUNES[i]+0.05,2),
                             "osc2_coarse":-12,"filter_cutoff":REESE_CUTOFFS[i],
                             "filter_drive":0.5,"filter_res":0.2,
                             "env2_attack":0.005,"env2_sustain":0.8,"env2_release":0.4,
                             "lfo2_rate":0.4,"lfo2_depth":0.2,**DIST_GRIT}})

# ACID (TB-style, high resonance, LFO2→cutoff)
ACID_RES   = [0.75,0.85,0.9,0.7,0.8,0.88,0.72,0.82,0.78]
ACID_RATE  = [2.0,4.0,1.5,3.0,6.0,2.5,4.5,3.5,1.0]
ACID_DEPTH = [0.7,0.8,0.6,0.75,0.9,0.65,0.85,0.7,0.55]
for i in range(9):
    BASS.append({"name": ["ACID PIPELINE","CORROSIVE LOOP","CHLORINE BASS","TOXIC ACID",
                           "CHEMICAL WASH","ACID DRAIN","NEON ACID","RAW ACID","DISSOLVE"][i],
                 "sub": "ACID",
                 "params": {"osc1_table":1,"filter_type":1,
                             "filter_cutoff":1200,"filter_res":ACID_RES[i],"filter_drive":0.25,
                             "lfo2_rate":ACID_RATE[i],"lfo2_depth":ACID_DEPTH[i],
                             "env1_attack":0.002,"env1_decay":0.2,"env1_depth":0.85,
                             "env2_attack":0.002,"env2_decay":0.25,"env2_sustain":0.4,"env2_release":0.1,
                             **DIST_SOFT}})

# NEURO (ring/FM bass, complex harmonics)
FM_RATIOS = [2.0,3.0,4.0,2.5,3.5,1.5,5.0,2.0]
for i in range(8):
    mm = [3,1,3,1,3,1,3,3][i]  # FM or Ring
    BASS.append({"name": ["NEURAL SIGNAL","BIOMECH BASS","CIRCUIT FLESH","NEURO GRID",
                           "SYNAPSE BASS","CORTEX LOW","NERVE CABLE","DATA TISSUE"][i],
                 "sub": "NEURO",
                 "params": {"osc1_table":[1,2,1,2,1,0,1,2][i],
                             "osc2_table":[2,1,0,1,2,1,2,0][i],
                             "osc_mix_mode":mm,"osc_fm_ratio":FM_RATIOS[i],"osc_mix":0.5,
                             "filter_cutoff":600,"filter_res":0.3,"filter_drive":0.4,
                             "env2_attack":0.002,"env2_sustain":0.6,"env2_release":0.3,
                             **DIST_GRIT}})

# DUB (slow, spacious, reggae-tech)
for i in range(8):
    fc = spread(100, 400, i, 8)
    BASS.append({"name": ["DEEP DUB","PRESSURE WAVE","DUB SIGNAL","DARK DUB",
                           "SUBSONIC DUB","PLATE DUB","CHANNEL DUB","STEP DUB"][i],
                 "sub": "DUB",
                 "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":-12,
                             "filter_cutoff":round(fc,1),"filter_res":0.05,
                             "env2_attack":0.1,"env2_decay":0.5,"env2_sustain":0.7,"env2_release":1.5,
                             "lfo2_rate":0.3,"lfo2_depth":0.15,**REVERB_ROOM}})

# INDUSTRIAL (heavy distortion, hard sync, metallic)
for i in range(9):
    co = [0,-12,12,0,-12,7,0,12,-7][i]
    mm = [2,0,2,1,0,2,0,1,2][i]
    BASS.append({"name": ["MACHINE BASS","FACTORY FLOOR","IRON LUNG","PISTON BASS",
                           "MOTOR BASS","GEAR BASS","TURBINE LOW","STEAM BASS","RIVET BASS"][i],
                 "sub": "INDUSTRIAL",
                 "params": {"osc1_table":1,"osc2_table":2,"osc2_coarse":co,
                             "osc_mix_mode":mm,"filter_cutoff":800,"filter_res":0.2,"filter_drive":0.6,
                             "env2_attack":0.001,"env2_sustain":0.7,"env2_release":0.25,
                             **DIST_HEAVY}})

# STAB-BASS (short, punchy, for sequences)
for i in range(9):
    fc = spread(800, 3000, i, 9)
    BASS.append({"name": ["VOLT STAB","CURRENT PUNCH","AMP STAB","OHMIC BASS",
                           "RESISTOR STAB","DIODE PUNCH","CAPACITOR HIT","RELAY STAB","FUSE BASS"][i],
                 "sub": "STAB",
                 "params": {"osc1_table":[1,2,1,1,2,1,2,1,1][i],"osc1_unison":[2,0,3,1,0,2,0,1,3][i],
                             "filter_cutoff":round(fc,1),"filter_res":0.3,
                             "env2_attack":0.001,"env2_decay":0.15,"env2_sustain":0.0,"env2_release":0.1,
                             **DIST_CRUNCH}})

# ── LEAD presets (60) ─────────────────────────────────────────────────────────

LEAD = []

# SUPERSAW (dense detuned saw leads)
for i in range(9):
    det = spread(0.25, 0.5, i, 9)
    fc  = spread(4000, 9000, i, 9)
    LEAD.append({"name": ["CHROME BLADE","STEEL NERVE","IRON SCREAM","CARBON EDGE",
                           "TITANIUM CUT","ALLOY SHRIEK","TUNGSTEN BLADE","FERRO SCREAM","OXIDE EDGE"][i],
                 "sub": "SUPERSAW",
                 "params": {"osc1_table":1,"osc1_unison":5,"osc1_detune":round(det,3),
                             "osc2_table":1,"osc2_unison":4,"osc2_detune":round(det+0.03,3),
                             "filter_cutoff":round(fc,1),"filter_res":0.15,
                             "env2_attack":0.008,"env2_sustain":1.0,"env2_release":0.35,
                             **REVERB_ROOM}})

# SYNC (hard sync sweep)
SYNC_FC = [3000,5000,2000,7000,4000,6000,1500,8000,2500]
for i in range(9):
    LEAD.append({"name": ["SYNC EDGE","HARD SYNC","SYNC SIGNAL","PHASE SYNC",
                           "SYNC NERVE","LOCK SYNC","SYNC WIRE","SYNC SLASH","RESET SYNC"][i],
                 "sub": "SYNC",
                 "params": {"osc1_table":1,"osc2_table":1,"osc_mix_mode":2,
                             "filter_cutoff":SYNC_FC[i],"filter_res":0.3,
                             "env2_attack":0.004,"env2_sustain":1.0,"env2_release":0.3,
                             "lfo1_rate":0.5,"lfo1_depth":0.4,**DIST_SOFT}})

# FM (frequency modulation leads, complex timbres)
for i in range(8):
    ratio = [2.0,3.0,4.5,1.5,7.0,2.5,3.5,5.0][i]
    fc    = [4000,6000,3000,8000,2000,7000,5000,4500][i]
    LEAD.append({"name": ["FM NERVE","CARRIER BLADE","SIDEBAND EDGE","FM SIGNAL",
                           "HARMONIC BLADE","FM WIRE","SPECTRUM CUT","FM SCREAM"][i],
                 "sub": "FM",
                 "params": {"osc1_table":0,"osc2_table":[1,0,2,1,0,2,1,0][i],
                             "osc_mix_mode":3,"osc_fm_ratio":ratio,
                             "filter_cutoff":fc,"filter_res":0.25,
                             "env2_attack":0.005,"env2_sustain":1.0,"env2_release":0.3,**DIST_SOFT}})

# MONO (mono portamento, expressive)
PORT = [0.04,0.08,0.02,0.12,0.06,0.1,0.015,0.09]
for i in range(8):
    LEAD.append({"name": ["SOLO NERVE","MONO BLADE","GLIDE EDGE","PORTAL LEAD",
                           "SLIDE SIGNAL","SWEEP BLADE","LEGATO EDGE","DRIFT LEAD"][i],
                 "sub": "MONO",
                 "params": {"osc1_table":[1,1,2,1,0,1,2,1][i],"osc2_table":[0,2,1,2,1,0,1,2][i],
                             "osc_mix":0.35,"filter_cutoff":5000,"filter_res":0.2,
                             "voice_poly":1,"voice_glide":PORT[i],"voice_glide_mode":0,
                             "env2_attack":0.005,"env2_sustain":1.0,"env2_release":0.3}})

# ACID LEAD
for i in range(8):
    res = spread(0.65, 0.9, i, 8)
    LEAD.append({"name": ["ACID NERVE","ACID SIGNAL","ACID BLADE","ACID EDGE",
                           "ACID WIRE","ACID SLASH","ACID SCREAM","ACID CUT"][i],
                 "sub": "ACID",
                 "params": {"osc1_table":1,"filter_type":1,
                             "filter_cutoff":2500,"filter_res":round(res,2),"filter_drive":0.25,
                             "env1_attack":0.002,"env1_decay":0.25,"env1_depth":0.9,
                             "env2_attack":0.002,"env2_sustain":0.8,"env2_release":0.2,
                             "lfo2_rate":0.5,"lfo2_depth":0.3}})

# SPECTRAL/FORMANT (vowel sweeps)
for i in range(9):
    rate = spread(0.2, 1.5, i, 9)
    LEAD.append({"name": ["VOWEL BLADE","FORMANT NERVE","SPEAK EDGE","VOICE SIGNAL",
                           "VOCAL BLADE","THROAT EDGE","FORMANT WIRE","PHARYNX LEAD","GLOTTAL EDGE"][i],
                 "sub": "SPECTRAL",
                 "params": {"osc1_table":1,"osc1_unison":[2,3,1,4,2,3,1,2,3][i],
                             "filter_type":7,"filter_cutoff":2000,"filter_res":0.4,
                             "lfo1_rate":rate,"lfo1_depth":0.7,
                             "env2_attack":0.01,"env2_sustain":1.0,"env2_release":0.4,
                             **REVERB_ROOM}})

# SOFT (melodic, warm leads)
for i in range(9):
    fc = spread(1500, 4000, i, 9)
    LEAD.append({"name": ["WARM SIGNAL","SOFT BLADE","MELLOW EDGE","VELVET LEAD",
                           "SILK SIGNAL","SMOOTH BLADE","GENTLE EDGE","WARM NERVE","SOFT CUT"][i],
                 "sub": "SOFT",
                 "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":12,"osc_mix":0.3,
                             "filter_cutoff":round(fc,1),"filter_res":0.1,
                             "env2_attack":0.04,"env2_sustain":0.9,"env2_release":0.6,
                             "lfo1_rate":5.0,"lfo1_depth":0.15,**REVERB_ROOM}})

# ── PAD presets (60) ──────────────────────────────────────────────────────────

PAD = []

# LUSH (wide detuned, warm)
for i in range(10):
    det = spread(0.18, 0.42, i, 10)
    atk = spread(0.8, 2.5, i, 10)
    PAD.append({"name": ["VOID DRIFT","CARBON HAZE","STEEL ETHER","NEON MIST",
                          "CHROME CLOUD","IRON VEIL","OXIDE DRIFT","COPPER HAZE","ALLOY MIST","ZINC VEIL"][i],
                "sub": "LUSH",
                "params": {"osc1_table":1,"osc1_unison":5,"osc1_detune":round(det,3),
                            "osc2_table":1,"osc2_unison":4,"osc2_detune":round(det+0.04,3),
                            "osc2_coarse":12,"filter_cutoff":3500,"filter_res":0.05,
                            "env2_attack":round(atk,2),"env2_sustain":1.0,"env2_release":3.0,
                            **REVERB_HALL,**CHORUS_ON}})

# DARK (slow, heavy, foreboding)
for i in range(10):
    fc  = spread(200, 800, i, 10)
    atk = spread(2.0, 4.5, i, 10)
    PAD.append({"name": ["DARK MATTER","VOID FIELD","BLACK ETHER","SHADOW HAZE",
                          "NIGHT CLOUD","ABYSS DRIFT","DARK SIGNAL","SHADOW FIELD","VOID HAZE","DEAD CLOUD"][i],
                "sub": "DARK",
                "params": {"osc1_table":1,"osc1_unison":4,"osc1_detune":0.32,
                            "osc2_table":1,"osc2_unison":3,"osc2_coarse":-12,
                            "filter_cutoff":round(fc,1),"filter_res":0.08,
                            "env2_attack":round(atk,2),"env2_sustain":1.0,"env2_release":4.0,
                            **REVERB_VAST,**DIST_SOFT}})

# MOTION (LFO-animated WT position)
LFO_RATES = [0.05,0.08,0.12,0.06,0.15,0.1,0.04,0.2,0.07,0.18]
for i in range(10):
    PAD.append({"name": ["MOTION FIELD","WT DRIFT","SCAN HAZE","MORPH CLOUD",
                          "SWEEP ETHER","SCAN SIGNAL","WAVE DRIFT","MORPH HAZE","EVOLVE CLOUD","SCAN FIELD"][i],
                "sub": "MOTION",
                "params": {"osc1_table":1,"osc1_wt_mode":2,"osc1_unison":3,"osc1_detune":0.2,
                            "filter_cutoff":3000,"filter_res":0.1,
                            "lfo1_rate":LFO_RATES[i],"lfo1_depth":0.9,
                            "env2_attack":1.5,"env2_sustain":1.0,"env2_release":3.0,
                            **REVERB_HALL}})

# CHOIR (formant/vocal)
for i in range(10):
    fc = spread(1500, 4000, i, 10)
    PAD.append({"name": ["MACHINE CHOIR","SYNTHETIC VOICE","CIRCUIT CHOIR","DIGITAL CHOIR",
                          "VOLTAGE CHOIR","NEURAL CHOIR","SYSTEM CHOIR","COLD CHOIR","IRON CHOIR","VOID CHOIR"][i],
                "sub": "CHOIR",
                "params": {"osc1_table":0,"osc1_unison":5,"osc1_detune":0.22,
                            "osc2_table":0,"osc2_unison":4,"osc2_fine":-6,
                            "filter_type":7,"filter_cutoff":round(fc,1),"filter_res":0.35,
                            "env2_attack":1.0,"env2_sustain":1.0,"env2_release":2.5,
                            **REVERB_HALL}})

# CRYSTAL (bright, shimmer)
for i in range(10):
    PAD.append({"name": ["CRYSTAL FIELD","ICE ETHER","GLASS HAZE","PRISM CLOUD",
                          "QUARTZ DRIFT","CRYSTAL SIGNAL","SHARD FIELD","FROST HAZE","DIAMOND CLOUD","CRYSTAL WIRE"][i],
                "sub": "CRYSTAL",
                "params": {"osc1_table":0,"osc1_unison":4,"osc1_detune":0.18,
                            "osc2_table":0,"osc2_coarse":24,
                            "filter_cutoff":7000,"filter_res":0.2,
                            "env2_attack":1.2,"env2_sustain":1.0,"env2_release":2.8,
                            **REVERB_VAST}})

# DRONE (static, evolving, textural)
for i in range(10):
    fc  = spread(300, 2000, i, 10)
    mm  = [0,1,3,0,1,0,3,0,1,0][i]
    PAD.append({"name": ["MACHINE DRONE","VOLTAGE DRONE","CURRENT DRONE","SIGNAL DRONE",
                          "STATIC DRONE","CIRCUIT DRONE","FEEDBACK DRONE","LOOP DRONE","PHASE DRONE","VOID DRONE"][i],
                "sub": "DRONE",
                "params": {"osc1_table":[1,0,2,1,0,1,2,0,1,2][i],
                            "osc2_table":[0,2,1,0,2,2,0,1,0,1][i],"osc_mix_mode":mm,
                            "osc_fm_ratio":[1,1,2,1,3,1,2,1,1,4][i],
                            "filter_cutoff":round(fc,1),"filter_res":[0.2,0.5,0.3,0.1,0.4,0.6,0.2,0.15,0.45,0.3][i],
                            "env2_attack":3.0,"env2_sustain":1.0,"env2_release":5.0,
                            "lfo1_rate":0.06,"lfo1_depth":0.5,**REVERB_VAST}})

# ── PLUCK presets (60) ────────────────────────────────────────────────────────

PLUCK = []

# METALLIC (ring mod, high resonance)
for i in range(12):
    fc  = spread(3000, 9000, i, 12)
    dec = spread(0.05, 0.8, i, 12)
    mm  = [1,1,0,1,1,0,1,1,0,1,1,0][i]
    PLUCK.append({"name": ["IRON PING","STEEL TICK","CHROME SPIKE","ALLOY PING",
                            "METAL TICK","ZINC SPIKE","COPPER PING","FERRO TICK","TIN SPIKE",
                            "OXIDE PING","RUST TICK","BOLT SPIKE"][i],
                  "sub": "METALLIC",
                  "params": {"osc1_table":[2,0,0,2,0,2,0,2,0,2,0,2][i],
                              "osc2_table":[0,2,1,0,2,0,2,0,2,0,2,0][i],
                              "osc_mix_mode":mm,"filter_cutoff":round(fc,1),"filter_res":0.7,
                              "env2_attack":0.001,"env2_decay":round(dec,3),"env2_sustain":0.0,"env2_release":round(dec*1.5,3)}})

# ACOUSTIC (natural pluck character)
for i in range(12):
    dec = spread(0.2, 1.0, i, 12)
    PLUCK.append({"name": ["WIRE PLUCK","CARBON PLUCK","STEEL PLUCK","NYLON WIRE",
                            "COPPER PLUCK","BRASS WIRE","ALLOY PLUCK","IRON WIRE","TIN PLUCK",
                            "ZINC WIRE","OXIDE PLUCK","CHROME WIRE"][i],
                  "sub": "ACOUSTIC",
                  "params": {"osc1_table":0,"osc2_table":1,"osc2_fine":8,"osc_mix":0.35,
                              "filter_cutoff":spread(2500,6000,i,12),"filter_res":0.05,
                              "env2_attack":0.001,"env2_decay":round(dec,3),"env2_sustain":0.0,"env2_release":round(dec*1.2,3)}})

# BELL (long sustain, high harmonics)
for i in range(12):
    co2 = [7,12,14,19,7,5,12,7,19,5,14,10][i]
    dec = spread(0.6, 2.5, i, 12)
    PLUCK.append({"name": ["IRON BELL","STEEL CHIME","CHROME BELL","ALLOY CHIME",
                            "COPPER BELL","OXIDE CHIME","FERRO BELL","TIN CHIME","ZINC BELL",
                            "RUST CHIME","BOLT BELL","RIVET CHIME"][i],
                  "sub": "BELL",
                  "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":co2,"osc_mix_mode":[1,0,1,0,1,0,1,0,1,0,1,0][i],
                              "filter_cutoff":8000,"filter_res":0.15,
                              "env2_attack":0.002,"env2_decay":round(dec,3),"env2_sustain":0.0,"env2_release":round(dec*1.3,3),
                              **REVERB_ROOM}})

# GLITCH (electronic, digital artifacts)
for i in range(12):
    fc = spread(2000, 9000, i, 12)
    PLUCK.append({"name": ["BIT PLUCK","DATA SPIKE","GLITCH PING","PIXEL TICK",
                            "BIT SPIKE","BYTE PING","ERROR PLUCK","CORRUPT TICK","OVERFLOW SPIKE",
                            "NULL PING","NaN PLUCK","SEGFAULT TICK"][i],
                  "sub": "GLITCH",
                  "params": {"osc1_table":2,"osc2_table":2,"osc2_fine":[5,7,3,10,7,5,3,12,7,5,10,3][i],
                              "osc_mix_mode":[1,0,1,0,1,0,1,0,1,0,1,0][i],
                              "filter_type":[6,1,6,4,6,1,6,4,6,1,6,4][i],
                              "filter_cutoff":round(fc,1),"filter_res":[0.6,0.2,0.7,0.3,0.5,0.15,0.65,0.25,0.55,0.1,0.7,0.3][i],
                              "env2_attack":0.001,"env2_decay":spread(0.04,0.3,i,12),"env2_sustain":0.0,"env2_release":0.1}})

# MALLET (warm, percussive)
for i in range(12):
    PLUCK.append({"name": ["IRON MALLET","CARBON MARIMBA","STEEL VIBES","ALLOY XYLOPHONE",
                            "COPPER MALLET","BRASS MARIMBA","OXIDE VIBES","ZINC MALLET","TIN MARIMBA",
                            "FERRO VIBES","CHROME MALLET","RUST MARIMBA"][i],
                  "sub": "MALLET",
                  "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":12,"osc_mix":0.4,
                              "filter_cutoff":spread(3000,7000,i,12),"filter_res":0.1,
                              "env2_attack":0.001,"env2_decay":spread(0.15,0.7,i,12),"env2_sustain":0.0,"env2_release":0.4}})

# ── STAB presets (60) ─────────────────────────────────────────────────────────

STAB = []

# TRANCE (supersaw, full, bright)
for i in range(10):
    fc = spread(3000, 7000, i, 10)
    STAB.append({"name": ["TRANCE BLADE","VOLTAGE STAB","CURRENT PUNCH","ARC STAB",
                           "SPARK PUNCH","FLASH STAB","BOLT PUNCH","SURGE STAB","PLASMA PUNCH","ION STAB"][i],
                 "sub": "TRANCE",
                 "params": {"osc1_table":1,"osc1_unison":4,"osc1_detune":0.28,
                             "osc2_table":1,"osc2_coarse":7,"osc_mix":0.45,
                             "filter_cutoff":round(fc,1),"filter_res":0.2,
                             "env2_attack":0.002,"env2_decay":0.2,"env2_sustain":0.0,"env2_release":0.12,
                             **REVERB_ROOM}})

# HOUSE (warm, chord-y)
for i in range(10):
    STAB.append({"name": ["HOUSE CHORD","DEEP STAB","VINYL PUNCH","FLOOR STAB",
                           "DECK CHORD","FOUR-FOUR STAB","GROOVE PUNCH","ACID HOUSE","CHICAGO STAB","DEEP CHORD"][i],
                 "sub": "HOUSE",
                 "params": {"osc1_table":1,"osc1_unison":3,"osc1_detune":0.2,
                             "osc2_table":0,"osc2_coarse":7,"osc_mix":0.45,
                             "filter_cutoff":spread(2500,6000,i,10),"filter_res":0.15,
                             "env2_attack":0.003,"env2_decay":0.25,"env2_sustain":0.0,"env2_release":0.15}})

# TECH (minimal, percussive, hard)
for i in range(10):
    STAB.append({"name": ["TECH PUNCH","MINIMAL STAB","GRID PUNCH","CELL STAB",
                           "MATRIX PUNCH","NODE STAB","SECTOR PUNCH","GRID STAB","DATA PUNCH","BYTE STAB"][i],
                 "sub": "TECH",
                 "params": {"osc1_table":[2,1,2,1,2,1,2,1,2,1][i],
                             "osc2_table":[1,2,1,2,1,2,1,2,1,2][i],
                             "osc_mix_mode":[0,1,0,0,0,1,0,0,0,1][i],
                             "filter_cutoff":spread(2000,6000,i,10),"filter_res":0.3,"filter_drive":0.3,
                             "env2_attack":0.001,"env2_decay":0.12,"env2_sustain":0.0,"env2_release":0.08,
                             **DIST_GRIT}})

# INDUSTRIAL (distorted, harsh)
for i in range(10):
    STAB.append({"name": ["MACHINE PUNCH","FACTORY STAB","RIVET PUNCH","BOLT STAB",
                           "PISTON PUNCH","GEAR STAB","TURBINE PUNCH","GRIND STAB","LATHE PUNCH","DRILL STAB"][i],
                 "sub": "INDUSTRIAL",
                 "params": {"osc1_table":[1,2,1,2,1,2,1,2,1,2][i],"osc1_unison":[2,0,3,0,2,1,3,0,2,1][i],
                             "filter_cutoff":spread(1500,5000,i,10),"filter_drive":0.5,"filter_res":0.25,
                             "env2_attack":0.001,"env2_decay":0.18,"env2_sustain":0.0,"env2_release":0.1,
                             **DIST_HEAVY}})

# FUNK (slap, groove-oriented)
for i in range(10):
    STAB.append({"name": ["FUNK STAB","GROOVE PUNCH","SLAP STAB","CLAV PUNCH",
                           "WAH STAB","MOOG PUNCH","BASS STAB","TIGHT PUNCH","FAT STAB","GROOVE CHORD"][i],
                 "sub": "FUNK",
                 "params": {"osc1_table":1,"osc1_unison":[2,1,3,1,2,0,2,1,3,2][i],
                             "filter_cutoff":spread(1500,5000,i,10),"filter_res":0.5,
                             "env1_attack":0.001,"env1_decay":0.1,"env1_depth":0.7,
                             "env2_attack":0.001,"env2_decay":0.15,"env2_sustain":0.0,"env2_release":0.1}})

# BRASS (fat, slightly detuned)
for i in range(10):
    STAB.append({"name": ["BRASS STAB","HORN PUNCH","BRASS CHORD","TRUMPET STAB",
                           "SECTION PUNCH","BRASS BLAST","HORN STAB","SAX PUNCH","TROMBONE STAB","BRASS HIT"][i],
                 "sub": "BRASS",
                 "params": {"osc1_table":1,"osc1_unison":3,"osc1_detune":0.18,
                             "osc2_table":2,"osc_mix":0.4,
                             "filter_cutoff":spread(2500,5000,i,10),"filter_res":0.2,"filter_drive":0.15,
                             "env2_attack":0.005,"env2_decay":0.2,"env2_sustain":0.0,"env2_release":0.15}})

# ── ELECTRO presets (60) ──────────────────────────────────────────────────────

ELECTRO = []

# ACID (classic 303-ish, all variations)
for i in range(10):
    res  = spread(0.65, 0.92, i, 10)
    rate = [2.0,4.0,1.0,6.0,3.0,8.0,2.5,4.5,1.5,3.5][i]
    ELECTRO.append({"name": ["ACID PROTOCOL","CHLORINE LOOP","ACID MACHINE","MURIATIC ACID",
                              "SULPHURIC LOOP","NITRIC ACID","ACID CIRCUIT","PHOSPHORIC LOOP",
                              "ACID NODE","CARBOLIC LOOP"][i],
                    "sub": "ACID",
                    "params": {"osc1_table":1,"filter_cutoff":1500,"filter_res":round(res,2),
                                "filter_drive":0.2,"lfo2_rate":rate,"lfo2_depth":0.75,
                                "env1_attack":0.002,"env1_decay":0.2,"env1_depth":0.9,
                                "env2_attack":0.002,"env2_decay":0.3,"env2_sustain":0.4,"env2_release":0.1}})

# TECHNO (driving, functional)
for i in range(10):
    ELECTRO.append({"name": ["TECHNO PROTOCOL","DETROIT SIGNAL","MOTOR CITY LOOP","TECHNO MACHINE",
                              "DETROIT PROTOCOL","MOTOR LOOP","TECHNO SIGNAL","CITY MACHINE",
                              "DETROIT LOOP","TECHNO NODE"][i],
                    "sub": "TECHNO",
                    "params": {"osc1_table":[2,1,2,1,2,1,2,1,2,1][i],
                                "osc2_table":[1,2,1,2,1,2,1,2,1,2][i],
                                "osc2_coarse":[-12,0,-12,0,-12,0,-12,0,-12,0][i],
                                "filter_cutoff":spread(400,1200,i,10),"filter_res":0.4,"filter_drive":0.35,
                                "env2_attack":0.002,"env2_sustain":0.7,"env2_release":0.2,
                                "lfo2_rate":[0,2,0,4,0,1,0,3,0,2][i],"lfo2_depth":[0,0.5,0,0.6,0,0.4,0,0.55,0,0.45][i],
                                **DIST_GRIT}})

# INDUSTRIAL (harsh, metallic electro)
for i in range(10):
    ELECTRO.append({"name": ["MACHINE PROTOCOL","FACTORY SIGNAL","INDUSTRIAL LOOP","STEEL MACHINE",
                              "IRON PROTOCOL","CHROME SIGNAL","ALLOY LOOP","OXIDE MACHINE",
                              "RUST PROTOCOL","RIVET SIGNAL"][i],
                    "sub": "INDUSTRIAL",
                    "params": {"osc1_table":[2,1,2,2,1,2,1,2,2,1][i],
                                "osc2_table":[1,2,0,1,2,0,2,1,0,2][i],
                                "osc_mix_mode":[1,0,1,3,0,1,0,1,3,0][i],
                                "osc_fm_ratio":[1,1,1,2,1,1,1,1,3,1][i],
                                "filter_cutoff":spread(600,2000,i,10),"filter_drive":0.65,
                                "env2_attack":0.002,"env2_sustain":0.6,"env2_release":0.25,
                                **DIST_HEAVY}})

# MODULAR (patch-style, unusual)
for i in range(10):
    ftype = [6,4,7,5,6,4,7,6,4,5][i]
    ELECTRO.append({"name": ["MODULAR PATCH","PATCH SIGNAL","EURORACK LOOP","MODULAR MACHINE",
                              "CV PATCH","GATE SIGNAL","TRIGGER LOOP","PATCH MACHINE",
                              "MODULE PROTOCOL","RACK SIGNAL"][i],
                    "sub": "MODULAR",
                    "params": {"osc1_table":[0,2,1,0,2,1,0,2,1,0][i],
                                "osc2_table":[2,0,2,2,0,2,2,0,2,2][i],
                                "osc_mix_mode":[3,1,3,0,3,1,3,0,3,1][i],
                                "osc_fm_ratio":[4.0,1.0,3.0,1.0,5.0,1.0,2.5,1.0,7.0,1.0][i],
                                "filter_type":ftype,"filter_cutoff":spread(300,3000,i,10),"filter_res":0.55,
                                "env2_attack":0.01,"env2_sustain":0.8,"env2_release":0.5,
                                "lfo1_shape":[5,0,5,1,5,0,5,2,5,0][i],"lfo1_rate":spread(0.1,8.0,i,10),"lfo1_depth":0.6}})

# RAVE (rave anthem energy)
for i in range(10):
    ELECTRO.append({"name": ["RAVE SIGNAL","RAVE MACHINE","RAVE PROTOCOL","RAVE LOOP",
                              "RAVE NODE","RAVE CIRCUIT","RAVE WIRE","RAVE CORE",
                              "RAVE PULSE","RAVE CURRENT"][i],
                    "sub": "RAVE",
                    "params": {"osc1_table":1,"osc1_unison":5,"osc1_detune":spread(0.3,0.5,i,10),
                                "osc2_table":2,"osc2_coarse":[0,-12,0,12,0,-12,0,12,0,-12][i],
                                "filter_cutoff":spread(2000,8000,i,10),"filter_res":0.25,"filter_drive":0.4,
                                "env2_attack":0.003,"env2_sustain":1.0,"env2_release":0.3,
                                **DIST_GRIT}})

# EBM (body music pulse)
for i in range(10):
    ELECTRO.append({"name": ["EBM PROTOCOL","BODY SIGNAL","EBM MACHINE","MARCHING LOOP",
                              "EBM NODE","STOMP SIGNAL","EBM WIRE","MARCH MACHINE",
                              "EBM CIRCUIT","BODY PROTOCOL"][i],
                    "sub": "EBM",
                    "params": {"osc1_table":[2,1,2,1,2,1,2,1,2,1][i],
                                "osc2_table":[1,2,1,2,1,2,1,2,1,2][i],
                                "osc2_coarse":-12,"osc_mix":0.4,
                                "filter_cutoff":spread(500,1500,i,10),"filter_res":0.35,"filter_drive":0.5,
                                "env2_attack":0.002,"env2_decay":0.3,"env2_sustain":0.5,"env2_release":0.2,
                                "lfo2_rate":[0,1,0,2,0,0.5,0,1.5,0,3][i],"lfo2_depth":[0,0.4,0,0.5,0,0.3,0,0.45,0,0.6][i],
                                **DIST_HEAVY}})

# ── MISC presets (60) ─────────────────────────────────────────────────────────

MISC = []

# AMBIENT
for i in range(10):
    MISC.append({"name": ["MACHINE HYMN","DARK RITUAL","CIRCUIT PRAYER","STEEL MANTRA",
                           "IRON HYMN","VOLTAGE PRAYER","CHROME RITUAL","OXIDE MANTRA",
                           "RUST HYMN","SYSTEM PRAYER"][i],
                 "sub": "AMBIENT",
                 "params": {"osc1_table":0,"osc1_unison":5,"osc1_detune":spread(0.2,0.45,i,10),
                             "osc2_table":0,"osc2_coarse":[12,0,12,0,12,0,12,0,12,0][i],"osc_mix":0.4,
                             "filter_cutoff":spread(800,5000,i,10),"filter_res":0.1,
                             "env2_attack":3.0,"env2_sustain":1.0,"env2_release":5.0,
                             "lfo1_rate":0.04,"lfo1_depth":0.4,"lfo1_shape":6,
                             **REVERB_VAST}})

# NOISE
for i in range(10):
    ftype=[1,4,5,6,1,4,5,6,1,4][i]
    MISC.append({"name": ["STATIC MASS","NOISE CLOUD","WHITE HISS","NOISE FIELD",
                           "GRAIN CLOUD","NOISE PULSE","STATIC FIELD","GRAIN MASS","NOISE SIGNAL","WHITE CLOUD"][i],
                 "sub": "NOISE",
                 "params": {"osc1_table":2,"osc1_unison":5,"osc1_detune":0.5,
                             "osc2_table":2,"osc2_unison":5,"osc2_detune":0.48,
                             "osc_mix_mode":[0,1,0,0,1,0,0,1,0,0][i],
                             "filter_type":ftype,"filter_cutoff":spread(300,5000,i,10),"filter_res":0.3,
                             "env2_attack":1.0,"env2_sustain":1.0,"env2_release":3.0,**REVERB_HALL}})

# BELL
for i in range(10):
    co2=[7,14,19,12,5,10,7,14,12,5][i]
    MISC.append({"name": ["FM BELL","IRON BELL","TOWER BELL","CARILLON","STEEL BELL",
                           "CHROME BELL","ALLOY BELL","OXIDE BELL","RUST BELL","CARBON BELL"][i],
                 "sub": "BELL",
                 "params": {"osc1_table":0,"osc2_table":0,"osc2_coarse":co2,
                             "osc_mix_mode":[3,1,3,1,3,1,3,1,3,1][i],
                             "osc_fm_ratio":[7.0,1.0,5.0,1.0,3.0,1.0,9.0,1.0,4.0,1.0][i],
                             "filter_cutoff":9000,"filter_res":0.1,
                             "env2_attack":0.003,"env2_decay":spread(0.8,3.0,i,10),"env2_sustain":0.0,"env2_release":spread(1.0,4.0,i,10),
                             **REVERB_HALL}})

# DRONE
for i in range(10):
    MISC.append({"name": ["MACHINE DRONE","SYSTEM DRONE","CIRCUIT DRONE","VOLTAGE DRONE",
                           "IRON DRONE","STEEL DRONE","CHROME DRONE","SIGNAL DRONE","STATIC DRONE","DARK DRONE"][i],
                 "sub": "DRONE",
                 "params": {"osc1_table":[1,0,2,1,0,1,2,0,1,2][i],
                             "osc2_table":[0,2,0,2,2,2,0,2,0,0][i],"osc2_coarse":[-12,0,-12,12,-12,0,12,-12,7,-7][i],
                             "filter_type":[1,6,1,4,1,6,1,4,1,6][i],
                             "filter_cutoff":spread(200,2000,i,10),"filter_res":[0.1,0.6,0.2,0.4,0.15,0.7,0.25,0.35,0.1,0.5][i],
                             "env2_attack":4.0,"env2_sustain":1.0,"env2_release":6.0,
                             "lfo1_rate":0.03,"lfo1_depth":0.5,**REVERB_VAST}})

# GLITCH
for i in range(10):
    MISC.append({"name": ["GLITCH TONE","BIT CRUSH","DATA CORRUPT","PIXEL FAIL",
                           "BUFFER OVER","NULL POINTER","SEGFAULT","OVERFLOW","UNDERRUN","CHECKSUM ERR"][i],
                 "sub": "GLITCH",
                 "params": {"osc1_table":2,"osc2_table":2,"osc2_fine":[7,3,12,5,10,3,7,14,5,3][i],
                             "osc_mix_mode":[1,0,1,0,1,0,1,0,1,0][i],
                             "filter_type":[6,4,6,5,6,4,6,4,6,5][i],
                             "filter_cutoff":spread(400,4000,i,10),"filter_res":spread(0.5,0.9,i,10),
                             "env2_attack":0.001,"env2_decay":spread(0.05,0.5,i,10),"env2_sustain":0.0,"env2_release":0.1,
                             "lfo1_shape":5,"lfo1_rate":16.0,"lfo1_depth":0.9}})

# TEXTURE
for i in range(10):
    MISC.append({"name": ["MACHINE TEXTURE","STEEL TEXTURE","CARBON TEXTURE","IRON TEXTURE",
                           "OXIDE TEXTURE","CHROME TEXTURE","ALLOY TEXTURE","RUST TEXTURE",
                           "ZINC TEXTURE","COPPER TEXTURE"][i],
                 "sub": "TEXTURE",
                 "params": {"osc1_table":[0,1,2,0,1,2,0,1,2,0][i],"osc1_unison":[4,5,4,3,5,4,3,5,4,3][i],"osc1_detune":0.38,
                             "osc2_table":[2,0,1,2,0,1,2,0,1,2][i],"osc_mix_mode":[1,0,3,1,0,3,1,0,3,1][i],
                             "osc_fm_ratio":[1,1,3,1,1,2,1,1,4,1][i],
                             "filter_cutoff":spread(600,4000,i,10),"filter_res":spread(0.2,0.7,i,10),
                             "env2_attack":spread(1.0,5.0,i,10),"env2_sustain":1.0,"env2_release":spread(2.0,6.0,i,10),
                             "lfo1_rate":0.06,"lfo1_depth":0.55,**REVERB_VAST}})

# ── assemble ──────────────────────────────────────────────────────────────────

all_presets = []
for cat_name, cat_data in [("BASS",BASS),("LEAD",LEAD),("PAD",PAD),
                             ("PLUCK",PLUCK),("STAB",STAB),("ELECTRO",ELECTRO),("MISC",MISC)]:
    for p in cat_data:
        all_presets.append({"name": p["name"], "category": cat_name,
                             "sub": p.get("sub",""), "params": p["params"]})

out = Path(__file__).parent / "presets.json"
out.write_text(json.dumps(all_presets, indent=2))
print(f"Written {len(all_presets)} presets ({len(set(p['category'] for p in all_presets))} cats, "
      f"{len(set(p['sub'] for p in all_presets))} sub-cats)")
