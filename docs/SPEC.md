# Trackspacer Clone — project spec and working agreement

Goal: a **native Linux VST3** that reproduces the behaviour of
Wavesfactory **Trackspacer 2.5.10** (32-band sidechain-driven dynamic EQ)
closely enough to be used interchangeably in a mix.

Reference artifacts (extracted from the official free installer, for
behavioural reference only — do **not** ship copied artwork/fonts/binaries):

- Windows original: `/tmp/ts_install/cf64/VST3/Trackspacer25.vst3`
- Original manual: `/tmp/trackspacer-manual.pdf`
- Extracted original UI assets (reference only):
  - `/tmp/tsimg_2.png` — full background at 2x (1300x1300) = 650x650 UI
  - `/tmp/tsimg_1.png` — low-cut knob filmstrip, 128 frames of 275x279
  - `/tmp/tsimg_3.png` — high-cut knob filmstrip, 128 frames of 291x301
  - `/tmp/tsimg_4.png` — amount knob filmstrip, 128 frames of 407x405
  - `/tmp/tsimg_0.png` — freeze button sprite 146x328
  - previews: `/tmp/ref_bg.png`, `/tmp/ref_amount.png`, `/tmp/ref_lowcut.png`,
    `/tmp/ref_highcut.png`, `/tmp/ref_freeze_full.png`
- Original screenshots (manual pages): `/tmp/man/page-07.png`, `/tmp/man/page-08.png`

The original is built with JUCE 7.0.5 + VST3 SDK 3.7.2. This project uses the
same JUCE version (local `JUCE/` checkout).

## Plugin identity

- Company: `OpenClone Audio`
- Product: `Trackspacer Clone`
- Bundle id: `com.openclones.trackspacerclone`
- Formats: VST3 + Standalone (Linux)
- UI: 650x650 design size, resizable 0.5x–2x, fixed square aspect.

## Parameters (provisional; calibrated against the reference)

| id | name | range | default | notes |
|----|------|-------|---------|-------|
| amount | Amount | 0..100 % | 0 | depth of reduction |
| lowcut | Low-cut | 20..20000 Hz (log) | 20 | analysis high-pass |
| highcut | High-cut | 20..20000 Hz (log) | 20000 | analysis low-pass |
| attack | Attack | 0.1..100 ms (log) | 10 | reduction onset |
| release | Release | 1..1000 ms (log) | 100 | reduction recovery |
| pan | Pan | -100..100 | 0 | pan of the effect: L↔R or M↔S |
| mode | Stereo Mode | choice: L/R, M/S | L/R | processing domain |
| freeze | Freeze | bool | off | hold current reduction curve |
| scaudition | Sidechain | bool | off | output the filtered sidechain |

Buses: main input stereo (mono also accepted), main output stereo, **sidechain
stereo input bus** (disabled by default, activated by the host). If no external
sidechain is active, the plugin analyses its own input (internal sidechain,
as in 2.5.8+).

## Audio behaviour (current implementation)

`src/dsp/CarverEngine`: STFT overlap-add engine.
- FFT 2048, hop 256 (8x overlap), Hann analysis+synthesis.
- Latency reported: **2048 samples** (provisional; matches or will be tuned to
  the reference measurement).
- Sidechain is 2nd-order Butterworth HPF (low-cut) and LPF (high-cut) filtered,
  then analysed into 32 logarithmically spaced bands (20 Hz..20 kHz).
- Band level -> reduction weight (`-54 dB` floor .. 0 dBFS) -> target reduction
  in dB, depth = `-30 dB * amount/100` (provisional).
- Attack/release one-pole smoothing per band (hop rate).
- Gain curve interpolated across FFT bins in log-frequency, applied to main.
- L/R or M/S domain processing; pan crossfades the effect between channels
  (`wA = 1-pan`, `wB = 1+pan`).
- Freeze holds the reduction gains.
- SC Audition outputs the filtered sidechain, delayed by the reported latency.
- Display data: sidechain spectrum (blue), reduction curve (white), 32 bands.

## UI layout (design coordinates, 650x650)

- Faceplate: rounded light-grey metal plate with dark border.
- Spectrum display: (50, 57, 548, 196); dark panel, log-frequency grid, blue
  sidechain curve, white reduction curve, filter-range shading, hover readout,
  small blue dot bottom-right that opens the advanced panel.
- FREEZE label around (70,276,80,16); freeze button centred (110,335) 36x36.
- `— TRACKSPACER —` around (430,276,165,16).
- Amount knob: 204x204 at (223,258); scale dots at (241,473) and (397,473);
  "0"/"100" labels at y=486; "AMOUNT" at (265,470,120,16).
- Low-cut knob 137x137 at (51,437); caption "20 HZ" (58,556), "20 K"
  (142,556), "LOW-CUT" (70,570).
- High-cut knob 145x145 at (457,433); labels mirrored.
- Wavesfactory badge around (303,548,44,44); wordmark at (215,598,220,16).
- Advanced panel occupies the display rectangle (50,57,548,196):
  title "Advanced Panel", Pan knob, L/R + M/S radio dots, Attack knob, Release
  knob, Sidechain button, blue close dot bottom-right.

## Build

```bash
cmake -B build-lead -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-lead TrackspacerClone TrackspacerHost
```

Outputs:
- `build-lead/TrackspacerClone_artefacts/Release/VST3/Trackspacer Clone.vst3`
- `build-lead/TrackspacerHost_artefacts/Release/TrackspacerHost`

Each teammate must use a separate build directory (`build-gui`, `build-test`,
...) to avoid racing ninja jobs.

## Offline host

`TrackspacerHost` (tools/host) commands:
- `dump <plugin.vst3>` — buses, latency, parameter table with texts.
- `render <plugin.vst3> --main in.wav [--side sc.wav] --out out.wav --set id=norm`
- `shot <plugin.vst3> --out png [--width 650]`

The host allocates a 4-channel process buffer: ch 0–1 main, ch 2–3 sidechain.

## Team ownership

- **lead**: CMake, `src/PluginProcessor.*`, `src/dsp/*`, `src/Parameters.h`,
  integration, final build + packaging.
- **gui-dev**: `src/gui/*`, `src/PluginEditor.*` (only those files).
- **test-dev**: `tools/host/*`, `tests/*`, validation/reporting (do not edit
  `src/`; report bugs to lead instead).
- **reference-dev**: `reference/*` only; Wine/yabridge lives under `/tmp`.
