# MacDanial Hills Mastering Compressor — User Manual

*by Basemental · version 1.0.0*

One plug-in, three rack units wired in series:

```
Input → MODEL A (standard) → MODEL B (Class-A) → BUS COMPRESSOR (C) → Output
```

A fresh instance is set up so nothing double-compresses: Model A's optical stage is OUT, and both channels of the bus compressor are OUT. Switch in what you want to use.

---

## 1. Installing

**Mac (Logic Pro, Ableton Live and others):** double-click `MacDanial Hills Mastering Compressor … macOS Installer.pkg`. Choose *Customize* to install only the AU or only the VST3. Restart your DAW. In Logic it appears under **Audio Units › Basemental**.
If macOS warns the installer is from an unidentified developer, right-click it → **Open** → **Open**. (This goes away once the installer is signed with an Apple Developer ID.)

**Windows:** run `MacDanial Hills Mastering Compressor … Windows Installer.exe`, then rescan plug-ins in your DAW. The VST3 goes to `C:\Program Files\Common Files\VST3`.

---

## 2. The toolbar

| Control | What it does |
|---|---|
| **Presets** menu | Factory presets and your own. Factory presets also appear in your DAW's preset menu. |
| **< >** | Previous / next preset |
| **Save** | Saves the current settings as a user preset |
| **Delete** | Deletes the selected user preset (factory presets can't be deleted) |
| **Zoom** | 60 % to 200 %. You can also drag the bottom-right corner to any size. |
| **Stretch** | Off: the window keeps its proportions. On: width and height resize freely and the units stretch to fill. |
| **Pop out A / B / C** | Opens that unit in its own floating window. Close the window (or press *Dock*) to put it back. |

User presets are stored in `~/Library/Audio/Presets/Basemental/MacDanial Hills` (Mac) or `Documents\Basemental\MacDanial Hills\Presets` (Windows), so you can back them up or share them.

The DAW's own bypass button bypasses all three units.

---

## 3. Models A and B

Both units have the same controls, per channel (left and right):

### Optical section (first compressor)

| Control | Values | Notes |
|---|---|---|
| **Optical Threshold** | 1–24 | 1 = least compression. Fixed 2:1 ratio, soft knee. |
| **Optical Gain** | 0–24 | **Position 7 = unity gain.** Fine 0.5 dB steps near unity, coarser further out (−5.75 dB to +18.25 dB). |
| **Optical IN / OUT** toggle | | OUT removes the optical stage and its gain. The red/green LED shows IN. |

The optical cell has a slow attack and a two-stage release: about 80 % of the reduction lets go quickly, the last 20 % takes over a second, and longer the harder it has been working.

### Discrete section (second compressor)

| Control | Values |
|---|---|
| **Discrete Threshold** | 1–24 |
| **Discrete Ratio** | 1.2, 2, 3, 4, 6, FLOOD (20:1) |
| **Discrete Attack** | .1, .5, 1, 5, 10, 30 ms |
| **Discrete Recover** | .1, .25, .5, .8, 1.2 s, DUAL (two-stage, like the optical) |
| **Discrete Gain** | 0–24, position 7 = unity |
| **Discrete IN / OUT** toggle | Switches the discrete section in or out |
| **Sidechain Filter IN / OUT** | 90 Hz high-pass on both detectors, so bass doesn't drive the compression |

### Output and metering

| Control | What it does |
|---|---|
| **Transformer** (3-way switch) | **Nickel**: cleanest, lift around 10 kHz. **Iron**: even-order harmonics, weight around 110 Hz. **Steel**: most colour, 40 Hz lift. Active even with both compressors OUT, so you can use it purely for tone. |
| **Meter Select** (small knob) | What the VU shows: Optical gain reduction, Discrete gain reduction, or Output level (0 VU = −14 dBFS RMS). The O / D / VU LEDs under each meter show the same thing, and you can click them. |
| **Hardwire Bypass** | OUT = the unit is a straight wire. |
| **Dual Mono / Stereo** | Stereo: the two channels are linked and the left and right controls move together. Dual Mono: each channel is independent (useful for M/S or two different sources). |
| **Magic eye** | Follows output peaks. |

**Clicking LEDs:** the stage LEDs switch their section in or out, and the O / D / VU LEDs choose the meter source.

### How A and B differ

| | Model A: standard | Model B: Class-A |
|---|---|---|
| Character | Denser, heavier, more saturated glue | More open, punchy and smooth, with more depth |
| Input | Op-amp line amp (2nd + 3rd harmonics) | Lundahl-style input transformer and discrete Class-A line amp (clean, even-order only) |
| Discrete VCA | Odd-order density grows as it compresses | Low-distortion Class-A VCA, softer knee |
| Sidechain | Standard | Faster Class-A sidechain |
| Output | Unity | About 1.5 dB hotter at the same settings |

---

## 4. Bus compressor (unit C)

| Control | Values |
|---|---|
| **POWER** | Turns the whole unit on or off (off = untouched audio) |
| **IN** (each channel) | Switches that channel's compression in |
| **STEREO** | Links the two channels; left and right controls move together |
| **Threshold** | −15 to +15 (0 = −10 dBFS) |
| **Ratio** | 2, 4, 10 |
| **Attack** | .1, .3, 1, 3, 10, 30 ms |
| **Release** | .1, .3, .6, 1.2 s, AUTO (program-dependent) |
| **Make-up** | 0 to +15 dB |
| **dB Compression meters** | Gain reduction per channel, 0–20 dB |

---

## 5. Factory presets

| Preset | What it's for |
|---|---|
| Default | Starting point |
| Mastering Glue | Model B, about 1 dB optical + 2 dB discrete, 1.2:1, 30 ms attack, .1 s recover, sidechain filter in, Steel |
| Transparent Master (Nickel) | Light discrete compression with Dual recover, Nickel |
| Gentle Opto Leveler | Optical only, for smoothing |
| Vocal Smoother | Model A optical + 2:1 discrete, Dual recover, Iron |
| Punchy Drum Bus | Model A 4:1 with 10 ms attack, Steel, plus the bus compressor |
| Class-A Polish | Model B discrete only, Iron |
| Classic Bus Comp | Bus compressor only, 4:1, Auto release |
| Steel Colour Only | No compression, just the Steel transformers |
| Flood Limiter | Model B Flood (20:1) with the fastest attack |
| Stack Everything | All three units working lightly together |

---

## 6. Tips

- **Mastering:** start from *Mastering Glue*. Aim for 1–2 dB on each meter, then level-match with the Gain knobs before judging.
- **Colour without compression:** switch both sections OUT and pick a transformer.
- **A/B the models:** Hardwire-bypass one unit, then the other. B is about 1.5 dB louder, so trim its gain to compare fairly.
- **Latency:** the plug-in reports its small latency (oversampling) to your DAW, which compensates automatically.
