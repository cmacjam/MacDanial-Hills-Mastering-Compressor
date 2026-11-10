# MacDanial Hills Mastering Compressor — A & B

Two JUCE 8 mastering-compressor plug-ins by Basemental (AU / VST3 / Standalone).

| Folder | Plug-in | Character |
|---|---|---|
| [`A/`](A) | **Basemental OD-2** | Standard dual-stage: Optical → Discrete VCA → Nickel / Iron / Steel output transformers. Black faceplate. |
| [`B/`](B) | **MacDanial Hills Mastering Compressor (Class-A Edition)** | Lundahl-style input transformer, smoother/punchier Class-A Discrete VCA, richer low end. Copper faceplate, green LEDs. |

## Builds

GitHub Actions (`.github/workflows/build.yml`) builds both plug-ins on every push to `main`:
macOS AU + VST3 (Apple Silicon + Intel, AU validated with `auval`) and Windows VST3.
Download them from **Actions → latest run → Artifacts**.
Push a tag (`git tag v1.0.0 && git push --tags`) to publish them as a GitHub **Release**.

To build locally, see `A/README.md` or `B/README.md`.
