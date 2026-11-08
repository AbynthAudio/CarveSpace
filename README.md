<p align="center">
  <img src="assets/abynth_logo.png" alt="Abynth Audio" width="460">
</p>

<h1 align="center">CarveSpace</h1>

<p align="center">
  <b>32-band spectral sidechain carver — native VST3 for Linux, Windows and macOS&nbsp;(Apple&nbsp;Silicon)</b><br>
  Zero latency. Dark, Light and pastel-Pink skins. Free and open source, no authorization.
</p>

<p align="center">
  <a href="https://github.com/AbynthAudio/CarveSpace/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/AbynthAudio/CarveSpace?label=download&style=for-the-badge"></a>
  <a href="LICENSE"><img alt="License: GPLv3" src="https://img.shields.io/badge/license-GPLv3-8a2be2?style=for-the-badge"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platforms-Linux%20%7C%20Windows%20%7C%20macOS%20arm64-3ddc84?style=for-the-badge">
</p>

---

## What is CarveSpace?

CarveSpace is a **native, from-scratch VST3 implementation of the spectral-sidechain-carving workflow**: it analyses the spectrum of a trigger signal (the *sidechain*) and automatically applies the inverse EQ curve to the track it is inserted on, so two elements can occupy the same frequency range without fighting.

Insert it on a bass, a pad, a vocal or a whole bus, feed it a sidechain, and turn up **Amount**. CarveSpace does the dynamic, 32-band EQ work in real time — no manual notch-fishing, no compressor pumping.

> **Inspiration, not a copy.** CarveSpace is an independent reimplementation inspired by the workflow of Wavesfactory **Trackspacer 2.5**. All source code, UI drawing, assets and DSP in this repository were written from scratch for this project. This project is **not affiliated with, endorsed by, sponsored by or connected to Wavesfactory**, and contains **no Wavesfactory code, binaries, images or fonts**. *Trackspacer* is a trademark of its respective owner. The original parameter ranges and measured behaviour were studied only to provide compatible controls and interoperability.

## Highlights

- **32-band dynamic EQ** driven by an external stereo sidechain.
- **Zero added latency** — measured sample-exact against the reference workflow.
- **Amount** (depth), **Low-cut / High-cut** filters, **Attack / Release** ballistics.
- **L/R and M/S** processing with a continuous **Pan** of the effect.
- **Freeze** (hold the current reduction curve) and **Sidechain Audition**.
- **Spectrum display**: sidechain spectrum and applied reduction curve, with hover read-out.
- **Resizable** 650×650 editor, persisted size and theme.
- **Three skins**: Dark, Light and pastel **Pink** — persisted per project and in the user config.
- **Animated Abynth Audio swirl** (clockwise black-hole rotation, ~9.5 s/rev); click it to pause/resume.
- **No licensing, serial, iLok or authorization** — GPLv3 open source.

## Screenshots & animations

**The three skins.** The faceplate, spectrum display and knobs are redrawn for each skin; the Abynth Audio swirl keeps its original colours.

<p align="center">
  <img src="docs/media/skins.gif" alt="CarveSpace skins: Dark, Light and Pink" width="650">
</p>

<p align="center">
  <img src="docs/screenshots/dark-650.png" width="31%" alt="Dark skin">
  <img src="docs/screenshots/light-650.png" width="31%" alt="Light skin">
  <img src="docs/screenshots/pink-650.png" width="31%" alt="Pink skin">
</p>

**Advanced panel in use.** Pan, L/R–M/S, Attack, Release and Sidechain audition.

<p align="center">
  <img src="docs/media/advanced-panel.gif" alt="CarveSpace advanced panel" width="650">
</p>

The Abynth Audio swirl rotates slowly in the background; **click it to pause or resume** the animation.

More stills in [`docs/screenshots/`](docs/screenshots).

## Download

Grab the latest release:

**[⬇ Downloads — GitHub Releases](https://github.com/AbynthAudio/CarveSpace/releases/latest)**

| Platform | File | Notes |
|---|---|---|
| **Linux x86_64** | `CarveSpace-linux-x86_64.vst3.tar.gz` | VST3 only |
| **Windows x64** | `CarveSpace-windows-x64.vst3.tar.gz` | VST3 only — **untested** (cross-built on Linux) |
| **macOS Apple Silicon** | `CarveSpace-macos-arm64.vst3.tar.gz` | VST3 only, unsigned — **untested** (built by CI, not run on macOS hardware) |

Every archive contains only the `CarveSpace.vst3` plug-in bundle (no Standalone).

### Install

**Linux**

```bash
tar -xzf CarveSpace-linux-x86_64.vst3.tar.gz
mkdir -p ~/.vst3
cp -r CarveSpace.vst3 ~/.vst3/
```

**Windows** — extract the `.tar.gz` (e.g. with 7-Zip) and copy the `CarveSpace.vst3` folder to `C:\Program Files\Common Files\VST3\`.
**macOS** — extract the `.tar.gz` and copy `CarveSpace.vst3` to `~/Library/Audio/Plug-Ins/VST3/` (or `/Library/Audio/Plug-Ins/VST3/`); if Gatekeeper complains, right-click → Open.

## Quick start

1. Insert CarveSpace on the track that should make room (bass, pads, bus…).
2. Enable the sidechain input in your DAW and route the priority track (kick, vocal…) into it.
3. Raise **Amount** until the track sits in the mix.
4. Fine-tune with **Low-cut / High-cut**; open the advanced panel (the dot in the display) for **Pan**, **L/R–M/S**, **Attack**, **Release** and **Sidechain Audition**.

## Parameters

| # | Name (UI) | ID | Range | Default |
|---|-----------|----|-------|---------|
| 0 | Amount (`Ratio`) | `RATIO` | 0–100 % | 15 % |
| 1 | Low-cut | `LC` | 20 Hz–20 kHz (log) | 20 Hz |
| 2 | High-cut | `HC` | 20 Hz–20 kHz (log) | 20 kHz |
| 3 | Freeze | `FREEZE` | on/off | off |
| 4 | Pan | `PAN` | 0–100 | 50 |
| 5 | Stereo Mode | `MODE` | L/R, M/S | L/R |
| 6 | Attack | `AT` | 0.01–100 ms (log) | 5 ms |
| 7 | Release | `RL` | 1–700 ms (log) | 30 ms |
| 8 | Sidechain audition | `SIDECHAIN` | on/off | off |
| 9 | Bypass | *(VST3 auto)* | on/off | off |

Stereo input, stereo output and a stereo sidechain input bus. If no sidechain is connected, CarveSpace analyses its own input.

## Build from source

JUCE 7.0.5 is not vendored (it is large). Clone it into `JUCE/` first:

```bash
git clone https://github.com/AbynthAudio/CarveSpace.git
cd CarveSpace
git clone --depth 1 --branch 7.0.5 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build CarveSpace_VST3
```

Output: `build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3`.
Cross-compiling Windows with LLVM-MinGW and packaging notes are in [docs/BUILD.md](docs/BUILD.md) and [docs/BUILD-CROSS.md](docs/BUILD-CROSS.md). The GitHub Actions matrix builds Linux, Windows and macOS arm64 automatically. The Windows and macOS binaries are provided as-is and are **untested on Windows/macOS hardware** (development and testing happen on Linux); please report any issue. (see [`.github/workflows/build.yml`](.github/workflows/build.yml)).

## Validation

`tests/validate.sh` is a self-contained offline harness (parameters, buses, latency, unity path, effect depth, filters, freeze, pan, M/S, SC audition, state, robustness at 1–4096 block sizes and 44.1/48/96 kHz, CPU, screenshots). It is calibrated against measurements of the reference workflow and is run in CI-like fashion from a fresh build.

Last run: **83 passed, 0 failed** (pluginval strictness 5 also passes). See [tests/REPORT.md](tests/REPORT.md).

## Project layout

```
src/            plugin processor, zero-latency 32-band DSP, editor and GUI
assets/         Abynth Audio logo/icon/wordmark + Comfortaa font
docs/           build docs, spec, screenshots and animated GIFs
tests/          offline validation harness and report
tools/host/     offline VST3 host (dump/render/screenshot tooling)
.github/        CI matrix for Linux, Windows and macOS arm64
```

## License

**GNU GPLv3** — see [LICENSE](LICENSE). Built with [JUCE 7.0.5](https://juce.com) (GPL-compatible). Comfortaa font is licensed under the SIL Open Font License.

<p align="center"><b>Made with care by Abynth Audio.</b></p>
