#!/usr/bin/env python3
"""
generate_presets.py — Generate JUCE APVTS preset XML files for VOID WAVE

Output format matches juce::ValueTree::createXml() exactly:
  <Parameters param_id="value" ... />

Usage:
  python generate_presets.py                          # uses presets.json in same dir
  python generate_presets.py my_presets.json          # custom JSON
  python generate_presets.py presets.csv              # CSV input
  python generate_presets.py --outdir /custom/path    # override output directory

Output: Resources/Presets/factory/{CATEGORY}/{NAME}.vwpreset
"""

import csv
import json
import os
import sys
from pathlib import Path
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Full parameter default table — mirrors C++ DEFAULTS in PresetManager.cpp.
# Types matter: int params must stay int so JUCE parses them correctly.
# ---------------------------------------------------------------------------
DEFAULTS = {
    # OSC 1
    "osc1_table": 0,        "osc1_position": 0.0,  "osc1_coarse": 0,
    "osc1_fine": 0.0,       "osc1_level": 0.8,     "osc1_pan": 0.0,
    "osc1_unison": 0,       "osc1_detune": 0.0,    "osc1_phase_mode": 0,
    "osc1_wt_mode": 0,      "osc1_width": 1.0,
    # OSC 2
    "osc2_table": 0,        "osc2_position": 0.0,  "osc2_coarse": 0,
    "osc2_fine": 0.0,       "osc2_level": 0.8,     "osc2_pan": 0.0,
    "osc2_unison": 0,       "osc2_detune": 0.0,    "osc2_phase_mode": 0,
    "osc2_wt_mode": 0,      "osc2_width": 1.0,
    # OSC Mix
    "osc_mix": 0.5,         "osc_mix_mode": 0,     "osc_fm_ratio": 1.0,
    # Pitch envelope
    "pitchenv_attack": 0.01, "pitchenv_amount": 0.0,
    # Filter
    "filter_type": 1,       "filter_cutoff": 5000.0, "filter_res": 0.0,
    "filter_drive": 0.0,    "filter_keytrack": 0.0,  "filter_veltrack": 0.0,
    # ENV 1 (→ filter)
    "env1_attack": 0.01,    "env1_decay": 0.1,     "env1_sustain": 0.7,
    "env1_hold": 0.0,       "env1_release": 0.3,   "env1_curve": 1,
    "env1_depth": 0.5,      "env1_loop": 0,        "env1_vel_sens": 0.5,
    # ENV 2 (→ VCA)
    "env2_attack": 0.01,    "env2_decay": 0.1,     "env2_sustain": 0.7,
    "env2_hold": 0.0,       "env2_release": 0.3,   "env2_curve": 1,
    "env2_loop": 0,         "env2_vel_sens": 0.5,
    # ENV 3 (free)
    "env3_attack": 0.01,    "env3_decay": 0.1,     "env3_sustain": 0.7,
    "env3_hold": 0.0,       "env3_release": 0.3,   "env3_curve": 1,
    "env3_loop": 0,         "env3_vel_sens": 0.5,
    # LFO 1
    "lfo1_shape": 0,        "lfo1_rate": 1.0,      "lfo1_tempo_sync": 0,
    "lfo1_sync_div": 0.5,   "lfo1_phase": 0.0,     "lfo1_trigger": 0,
    "lfo1_fade": 0.0,       "lfo1_depth": 0.5,
    # LFO 2
    "lfo2_shape": 0,        "lfo2_rate": 1.0,      "lfo2_tempo_sync": 0,
    "lfo2_sync_div": 0.5,   "lfo2_phase": 0.0,     "lfo2_trigger": 0,
    "lfo2_fade": 0.0,       "lfo2_depth": 0.5,
    # Mod matrix (12 slots, all disabled)
    **{f"mod{i}_source": 0 for i in range(1, 13)},
    **{f"mod{i}_dest":   0 for i in range(1, 13)},
    **{f"mod{i}_depth":  0.0 for i in range(1, 13)},
    **{f"mod{i}_enabled": 0 for i in range(1, 13)},
    # Macros
    "macro1": 0.0, "macro2": 0.0, "macro3": 0.0, "macro4": 0.0,
    # Voice / global
    "voice_poly": 8,        "voice_glide": 0.0,    "voice_glide_mode": 1,
    "voice_pitch_bend": 2,  "master_tune": 0.0,    "master_volume": 0.8,
    # FX 1 Distortion
    "fx1_type": 0,   "fx1_param1": 0.0,    "fx1_param2": 8000.0, "fx1_param3": 0.0,
    # FX 2 Modulation
    "fx2_type": 0,   "fx2_param1": 1.0,    "fx2_param2": 0.5,    "fx2_param3": 0.0,
    # FX 3 Delay
    "fx3_type": 1,   "fx3_param1": 375.0,  "fx3_param2": 0.4,    "fx3_param3": 0.0,
    "fx3_sync": 0,
    # FX 4 Reverb
    "fx4_param1": 0.5, "fx4_param2": 0.5,  "fx4_param3": 0.0,   "fx4_param4": 0.0,
}

# Params that JUCE registered as int/choice — must be serialised without decimal point.
INT_PARAMS = {k for k, v in DEFAULTS.items() if isinstance(v, int)}


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

def format_value(key: str, val) -> str:
    """Serialise a value the same way JUCE does in ValueTree XML."""
    if key in INT_PARAMS:
        return str(int(round(float(val))))
    f = float(val)
    # JUCE uses up to 17 significant digits for doubles, but 7 is fine for floats.
    # Use repr-style for exact round-trips on common values.
    s = f"{f:.10g}"
    # Ensure there's a decimal point so JUCE parses it as float, not int.
    if "." not in s and "e" not in s:
        s += ".0"
    return s


def safe_filename(name: str) -> str:
    return name.replace(" ", "_").replace("/", "-").replace("\\", "-")


# ---------------------------------------------------------------------------
# XML generation
# ---------------------------------------------------------------------------

def build_xml(name: str, category: str, overrides: dict, sub: str = "") -> ET.ElementTree:
    """Merge defaults + overrides, return an ElementTree ready to write."""
    params = dict(DEFAULTS)
    for k, v in overrides.items():
        if k not in DEFAULTS:
            print(f"  [WARN] Unknown param '{k}' in preset '{name}' — skipped")
            continue
        params[k] = v

    root = ET.Element("Parameters")
    # Metadata attributes (prefixed _ so parsePresetFile skips them as params)
    root.set("_name",     name)
    root.set("_category", category)
    root.set("_sub",      sub)
    # All param values
    for k in DEFAULTS:
        root.set(k, format_value(k, params[k]))

    return ET.ElementTree(root)


# ---------------------------------------------------------------------------
# Input parsers
# ---------------------------------------------------------------------------

def load_json(path: Path) -> list:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, list):
        raise ValueError("JSON root must be an array of preset objects")
    return data


def load_csv(path: Path) -> list:
    """
    CSV format:
      name, category, param1=value1, param2=value2, ...

    First two columns are name and category.
    Remaining columns are key=value pairs (one param per column).
    """
    presets = []
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        headers = None
        for row in reader:
            row = [c.strip() for c in row]
            if not row or row[0].startswith("#"):
                continue
            if headers is None:
                headers = row
                continue
            if len(row) < 2:
                continue
            preset = {"name": row[0], "category": row[1], "params": {}}
            for i, cell in enumerate(row[2:], start=2):
                if "=" in cell:
                    k, _, v = cell.partition("=")
                    preset["params"][k.strip()] = float(v.strip())
                elif i < len(headers) and headers[i] in DEFAULTS:
                    if cell:
                        preset["params"][headers[i]] = float(cell)
            presets.append(preset)
    return presets


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    script_dir = Path(__file__).parent
    input_file = None
    out_dir    = script_dir / "Resources" / "Presets" / "factory"

    for arg in sys.argv[1:]:
        if arg.startswith("--outdir="):
            out_dir = Path(arg.split("=", 1)[1])
        elif not arg.startswith("-"):
            input_file = Path(arg)

    # Default input: presets.json next to this script
    if input_file is None:
        input_file = script_dir / "presets.json"

    if not input_file.exists():
        print(f"Error: input file not found: {input_file}")
        sys.exit(1)

    suffix = input_file.suffix.lower()
    if suffix == ".json":
        presets = load_json(input_file)
    elif suffix == ".csv":
        presets = load_csv(input_file)
    else:
        print(f"Error: unsupported file type '{suffix}' (use .json or .csv)")
        sys.exit(1)

    print(f"Generating {len(presets)} presets → {out_dir}\n")

    written, skipped = 0, 0
    for p in presets:
        name     = p.get("name", "Untitled").strip()
        category = p.get("category", "USER").strip().upper()
        overrides = p.get("params", {})

        if not name:
            skipped += 1
            continue

        cat_dir = out_dir / category
        cat_dir.mkdir(parents=True, exist_ok=True)

        sub = p.get("sub", "").strip().upper()
        out_path = cat_dir / (safe_filename(name) + ".vwpreset")
        tree = build_xml(name, category, overrides, sub)

        # Write with XML declaration
        tree.write(str(out_path), xml_declaration=True, encoding="UTF-8")
        print(f"  {category}/{safe_filename(name)}.vwpreset")
        written += 1

    print(f"\n✓  {written} written" + (f", {skipped} skipped" if skipped else ""))

    # Hint
    user_dir = Path.home() / "Library/Application Support/Void Audio/Void Wave/Presets"
    print(f"\nTo load as user presets during dev, copy to:\n  {user_dir}")


if __name__ == "__main__":
    main()
