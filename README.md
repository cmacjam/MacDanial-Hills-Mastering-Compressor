# MacDanial Hills Mastering Compressor

**User manual: [MANUAL.md](MANUAL.md)**

JUCE 8 mastering-compressor plug-ins by Basemental (AU / VST3 / Standalone).

| Folder | Plug-in | What it is |
|---|---|---|
| [`Combined/`](Combined) | **MacDanial Hills Mastering Compressor** | **Main product.** Model A, Model B and a 1U bus compressor (C) stacked in one plug-in, wired in series (input → A → B → C → output). Each unit has its own controls and meters; the DAW bypass button bypasses all three. |
| [`A/`](A) | MacDanial Hills A | Model A on its own: Optical → Discrete VCA → Nickel / Iron / Steel transformers. Black faceplate. |
| [`B/`](B) | MacDanial Hills B | Model B on its own: Class-A edition with input transformer, smoother/punchier Class-A VCA. Copper faceplate, green LEDs. |

### Combined plug-in features

- **Toolbar**: factory + user presets (also in your DAW's preset menu), previous/next, Save, Delete; Zoom 60–200 %; **Stretch** (resize width and height freely); **Pop out A / B / C** (open a unit in its own floating window, close it to dock it back).
- User presets are saved to `~/Library/Audio/Presets/Basemental/MacDanial Hills` (Windows: `Documents\Basemental\MacDanial Hills\Presets`).
- **Unit C** (bus compressor): POWER, IN per channel, STEREO link, Threshold (−15…+15), Ratio 2/4/10, Attack .1–30 ms, Release .1–1.2 s / AUTO, Make-up 0–15 dB, needle gain-reduction meters. Starts with both channels OUT.
- In Stereo mode, left and right controls move together (either side can be used).
- Every LED is clickable: O / D / VU select the meter source, stage LEDs switch their section IN/OUT.

### Sound (from the hardware documentation)

- Optical: fixed 2:1, soft knee, slow attack, two-stage release (80 % fast, 20 % > 1 s, program-dependent).
- Discrete: 1.2 / 2 / 3 / 4 / 6 / Flood (20:1); .1–30 ms attack; .1–1.2 s or Dual recover; 90 Hz sidechain filter.
- Gain knobs: 24 positions, **unity at position 7**, 0.5 dB steps near unity, coarser further out (−5.75 … +18.25 dB).
- Nickel: lowest distortion, lift around 10 kHz. Iron: Class-A output, even-order harmonics, ~110 Hz lift. Steel: most (still mild) distortion, ~40 Hz lift.

To change A or B, edit `A/` or `B/` and run `python3 tools/sync_combined.py` to regenerate the combined plug-in's copies.

In the combined plug-in, Model A's optical stage starts switched OUT so a fresh instance doesn't double-compress; switch it IN when you want both units working.

## Builds and installer

GitHub Actions (`.github/workflows/build.yml`) builds everything on each push to `main`. Download from **Actions → latest run → Artifacts**:

| Artifact | Contents |
|---|---|
| **MacDanialHills-Installer-macOS** | `MacDanial Hills Mastering Compressor Installer.pkg`: double-click installer for the combined plug-in (AU + VST3) |
| MacDanialHills-AB-macOS / -Windows | Combined plug-in as raw bundles |
| A-MacDanialHills-macOS / -Windows | Model A on its own |
| B-MacDanialHills-macOS / -Windows | Model B on its own |

Push a tag (`git tag v1.0.0 && git push --tags`) to publish everything as a GitHub **Release**.

### About the installer

The `.pkg` is built by `installer/macos/build_pkg.sh`. It installs to `/Library/Audio/Plug-Ins/Components` and `/Library/Audio/Plug-Ins/VST3`, and the user can untick AU or VST3 under *Customize*.
It is **unsigned** until you add an Apple Developer ID, so macOS will warn on first open: right-click the `.pkg` → **Open** → **Open**.
To sell it, sign it with a *Developer ID Installer* certificate (`INSTALLER_SIGN_ID`) and notarize it with `xcrun notarytool`.
