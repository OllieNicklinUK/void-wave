# VOID WAVE — UI Design System
Generated from UI/UX Pro Max skill. Style: Retro-Futurism + Dark Mode OLED.

## Color Tokens
```
--bg-void:       #000000   // pure black (OLED base)
--bg-deep:       #050508   // deep background
--bg-panel:      #0a0b10   // panel surface (elevation 1)
--bg-raised:     #111318   // raised controls (elevation 2)
--bg-control:    #1a1d26   // knob/slider track (elevation 3)
--bg-elevated:   #1e2235   // active/highlighted (elevation 4)

--neon-amber:    #f5a623   // primary accent (warm, main brand)
--neon-cyan:     #00d4ff   // secondary accent (cool, OSC2/LFO)
--neon-violet:   #9b5fe0   // tertiary (mod/LFO visual)
--neon-green:    #00ff88   // success/envelope accent
--neon-red:      #ff3355   // danger/filter drive

--border-subtle: #1c2030   // hairline separators
--border-glow:   #2a3050   // visible borders with ambient
--text-bright:   #e8edf8   // primary text
--text-mid:      #7a8099   // secondary labels
--text-dim:      #3a3f55   // disabled / placeholder

--glow-amber:    rgba(245,166,35,  0.35)
--glow-cyan:     rgba(0,  212,255, 0.30)
--glow-violet:   rgba(155, 95,224, 0.28)
--glow-green:    rgba(0,  255,136, 0.30)
```

## Typography
- **Display / Section headers**: Orbitron Bold 700, 8–9px, 3–4px letter-spacing, UPPERCASE
- **Values / readouts**: JetBrains Mono 400, 9–11px (monospaced for stable width)
- **Labels**: JetBrains Mono 400, 8px, 1–2px letter-spacing, uppercase
- Fonts are embedded via BinaryData (load at startup)

## Elevation System (Shadow → Glow)
```
Level 0 (flat):   no shadow
Level 1 (panel):  inset top 1px rgba(255,255,255,0.04) — simulates rim light
Level 2 (knob):   outer drop 0 4px 12px rgba(0,0,0,0.6)
                  inner rim  0 1px 0   rgba(255,255,255,0.08)
Level 3 (active): outer glow 0 0 8px  accent-glow-color
                  outer glow 0 0 20px accent-glow-color × 0.5
Level 4 (focused): full neon halo — 0 0 0 2px bg, 0 0 0 3px accent
```

## Knob Specification (38px standard)
```
Track ring:      Colour(0xff1a1d26), strokeWidth 3.5px
                 arc from 225° to 315° (270° sweep)
Value arc:       section neon colour, strokeWidth 3.0px, glow layer × 0.4
                 same arc painted twice: wide+transparent (glow), thin+opaque (crisp)
Centre body:     radial gradient: centre Colour(0xff252a38) → edge Colour(0xff0d0f16)
                 rim highlight: arc at 12 o'clock, 120° wide, rgba(255,255,255,0.12)
Indicator dot:   section accent colour, 4px diameter at 85% radius
                 glow halo: 8px diameter, accent × 0.5
On hover:        value arc glow × 1.5, body brightens +8%
```

## Panel / Section Specification
```
Background:      Colour(0xff0a0b10) — not pure black, slight blue-grey shift
Top edge line:   2px, section neon colour (full opacity)
Top glow:        8px height gradient below top line, neon × 0.15 → transparent
Inner border:    1px solid Colour(0xff1c2030) all sides
Corner radius:   0px (hard edge — industrial/hardware aesthetic)
Header text:     section neon, 8px, Orbitron-style spacing
```

## Background Ambient
```
Main bg:         #050508 base
Ambient blobs:   2 radial gradients, very subtle, opacity 0.04–0.06
  Blob 1:        neon-cyan at (10%, 30%), radius 35% of width
  Blob 2:        neon-violet at (80%, 70%), radius 28% of width
Scanline overlay: horizontal lines every 2px, rgba(0,0,0,0.12) — very subtle
```

## Combo Box
```
Background:      Colour(0xff111318)
Border:          1px Colour(0xff2a3050)
Text:            text-bright, JetBrains Mono
Arrow:           section accent colour, 8px triangle
Hover:           border brightens to section accent × 0.5
```

## Button (Toggle / Tab)
```
Off:             bg Colour(0xff111318), border Colour(0xff1c2030), text text-mid
On:              bg accent × 0.15, border accent × 0.7, text accent
                 top edge 1px accent (full)
Hover off:       border Colour(0xff2a3050)
```
