# MacDanial Hills Mastering Compressor

JUCE 8 mastering-compressor plug-ins by Basemental (AU / VST3 / Standalone).

| Folder | Plug-in | What it is |
|---|---|---|
| [`Combined/`](Combined) | **MacDanial Hills Mastering Compressor** | **Main product.** Model A stacked above Model B in one plug-in, wired in series (input → A → B → output). Each unit has its own controls, meters and hardwire bypass; the DAW bypass button bypasses both. |
| [`A/`](A) | MacDanial Hills A | Model A on its own: Optical → Discrete VCA → Nickel / Iron / Steel transformers. Black faceplate. |
| [`B/`](B) | MacDanial Hills B | Model B on its own: Class-A edition with input transformer, smoother/punchier Class-A VCA. Copper faceplate, green LEDs. |

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
