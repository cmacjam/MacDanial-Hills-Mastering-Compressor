# Basemental OD-2 Mastering Compressor

A dual-channel, two-stage mastering compressor plug-in (AU / VST3 / Standalone) built with JUCE 8.
Signal path per channel, as on the hardware it is modelled on:

```
Input line amp -> OPTICAL compressor -> DISCRETE (VCA) compressor -> Output transformer (Nickel / Iron / Steel)
```

## Controls

| Section | Control | Values |
|---|---|---|
| Optical | Threshold | 1–24 (1 = least compression, 0 dBFS; each step −1.25 dB) |
| | Gain | 0–24, **12 = unity**, 1 dB per step |
| | Bypass toggle | IN / OUT (red LED lit = IN) |
| | Behaviour | Fixed 2:1, soft knee, ~10 ms attack, two-stage release: 80 % fast (~60 ms), 20 % slow (0.8–1.8 s, deeper = slower) |
| Discrete | Threshold | 1–24 |
| | Ratio | 1.2, 2, 3, 4, 6, FLOOD (20:1) |
| | Attack | .1, .5, 1, 5, 10, 30 ms |
| | Recover | .1, .25, .5, .8, 1.2 s, DUAL (two-stage, like the optical) |
| | Gain | 0–24, 12 = unity |
| | Bypass toggle | IN / OUT |
| Sidechain filter | Toggle | 90 Hz high-pass on both detectors |
| Transformer | 3-way toggle | Nickel (clean, air), Iron (low-mid weight), Steel (40 Hz bump, most saturation) |
| Meter select | Mini knob | Optical GR / Discrete GR / Output (0 VU = −14 dBFS RMS) |
| Centre | Dual Mono / Stereo | In Stereo, the **left** controls run both channels and detectors are linked (right controls grey out). Meter select stays independent. |
| | Hardwire bypass | Clickless, latency-matched. Also mapped to the DAW's bypass button. |
| | Magic eye | Follows output peaks |

Click-free switching: every stage IN/OUT, gain step, transformer change and bypass is crossfaded.
Processing runs at 4× oversampling (reported latency is 6 samples at 48 kHz).

## Build on GitHub (no local compiler needed)

The repo-level workflow (`.github/workflows/build.yml` at the repo root) builds this plug-in on every push to `main`:

- **macOS** (Apple Silicon + Intel): AU `.component` and `.vst3`, ad-hoc signed and checked with `auval`
- **Windows**: `.vst3`

Download them from the repo's **Actions** tab → latest run → **Artifacts**.
Push a tag such as `v1.0.0` (`git tag v1.0.0 && git push --tags`) and the builds are also attached to a GitHub **Release**.
Install on a Mac by unzipping into `~/Library/Audio/Plug-Ins/Components/` (AU) or `~/Library/Audio/Plug-Ins/VST3/`.

## Build locally — macOS (Logic Pro, AU + VST3)

Requirements: Xcode (from the App Store, open it once to accept the licence) and CMake 3.22+ (`brew install cmake`).

```bash
cd BasementalOD2
cmake -B build -G Xcode
cmake --build build --config Release
```

The first configure downloads JUCE 8.0.4 (needs internet). When the build finishes, the plug-ins are copied automatically to:

- `~/Library/Audio/Plug-Ins/Components/Basemental OD-2.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/Basemental OD-2.vst3`

Validate the AU so Logic will load it:

```bash
auval -v aufx Bod2 Bsmt
```

If Logic doesn't list it, quit Logic, run `killall -9 AudioComponentRegistrar`, and reopen
(or use Logic › Settings › Plug-in Manager › Reset & Rescan Selection).

For a quick local signature on Apple Silicon (if macOS refuses to load an unsigned bundle):

```bash
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/Basemental\ OD-2.component
```

Selling it: sign with your Developer ID and notarize before distributing (`codesign` + `xcrun notarytool`).

## Build locally — Windows (VST3)

Requirements: Visual Studio 2022 with "Desktop development with C++", CMake 3.22+.

```bat
cd BasementalOD2
cmake -B build -A x64
cmake --build build --config Release
```

The VST3 is copied to `C:\Program Files\Common Files\VST3\` (run the terminal as Administrator, or set
`COPY_PLUGIN_AFTER_BUILD FALSE` in CMakeLists.txt and copy it from `build\BasementalOD2_artefacts\Release\VST3`).

## Project layout

```
CMakeLists.txt
Source/
  ParamIDs.h                parameter IDs
  PluginProcessor.h/.cpp    parameters, state, host bypass, latency
  PluginEditor.h/.cpp       faceplate layout (1600×720 design units, resizable, fixed aspect)
  DSP/OD2Engine.h/.cpp      oversampled optical + discrete + transformer engine, lock-free meters
  UI/PanelLookAndFeel.*     bakelite knobs, bat toggles, LEDs, screws, curved legends
  UI/PanelComponents.*      VU meter, magic eye, LED selectors
```

## Tuning points

- Threshold / gain calibration: `thresholdDb()` and `makeupDb()` in `DSP/OD2Engine.h`
- Transformer voicings (EQ, drive, LF core saturation): `Transformer::configure()` in `DSP/OD2Engine.cpp`
- Output meter reference: `kVuRefDbfs` in `PluginEditor.cpp`
- Rename the product, or change the 4-letter plug-in and manufacturer codes, in `CMakeLists.txt` (keep them unique to each product)
