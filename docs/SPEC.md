# CarveSpace — technical spec (final)

CarveSpace is a native Linux/Windows/macOS VST3 spectral sidechain carver: it
analyses a sidechain signal and carves the inverse spectrum into the track it
is inserted on, so two tracks can share the same frequency range. It is an
independent, from-scratch implementation of the Trackspacer 2.5 workflow,
written in C++17 with JUCE 7.0.5. GPLv3, no authorization/licensing of any kind.

## Identity

- Product: **CarveSpace**
- Author/manufacturer: **Abynth Audio**
- Bundle id: `com.abynthaudio.carvespace`
- Manufacturer code: `Abyn`, plugin code `CrvS`
- Formats: VST3 (Linux x86_64, Windows x64, macOS arm64) + Standalone

## Parameters (match the reference plug-in exactly)

Host parameter order, names, JUCE string-hash VST3 IDs, ranges, defaults and
display text all match the reference (verified against a Wine/yabridge dump of
Trackspacer 2.5.10):

| # | name | id | range | default | display |
|---|------|----|-------|---------|---------|
| 0 | Ratio (UI: AMOUNT) | RATIO (77748203) | 0..100 % | 15 % | 1 decimal |
| 1 | Low-cut | LC (2423) | 20..20000 Hz log | 20 Hz | 2 dp / k |
| 2 | High-cut | HC (2299) | 20..20000 Hz log | 20 kHz | 2 dp / k |
| 3 | Freeze | FREEZE | bool | OFF | OFF/ON (>0.5) |
| 4 | Pan | PAN (78973) | 0..100 | 50 | 1 decimal |
| 5 | Stereo Mode | MODE (2372003) | L/R, M/S | L/R | choice |
| 6 | Attack | AT (2099) | 0.01..100 ms log | 5 ms | 2 dp |
| 7 | Release | RL (2618) | 1..700 ms log | 30 ms | 2 dp |
| 8 | Sidechain (SC audition) | SIDECHAIN | bool | OFF | OFF/ON |
| 9 | Bypass | auto (JUCE VST3) | bool | Off | Off/On |

Buses: stereo Input, stereo Sidechain input (aux, disabled by default), stereo
Output. If no sidechain is connected the plug-in analyses its own input.

## DSP

- **Zero latency** (reported latency = 0); all processing is IIR.
- Main path: cascade of 32 peaking filters at 18 Hz × 1.2495^k centres
  (~1/3 octave, 18 Hz..22.5 kHz). A peaking filter is exactly transparent at
  0 dB, so the cascade is bit-transparent at zero reduction.
- Analysis: sidechain is filtered by a 1st-order high-pass (Low-cut) and a
  2nd-order low-pass (High-cut), then 32 *wide* band-pass filters (Q ≈ 0.7,
  centres matching the bands) with peak envelope followers (attack/release).
- Control law fitted to the reference:
  `depth = 33.10 / (1 + exp(-(L - T)/3.75))`, `T = -43.5 - 42·log10(amount)`,
  with L the band level in dBFS. Max reduction 33.1 dB.
- Per-band gain = `-depth · bandScale[band]`, where `bandScale` is computed at
  prepare time by solving the peaking-cascade overlap matrix (and normalised so
  full depth gives ~33 dB), preventing over-attenuation of adjacent bands.
- Gains are updated at control rate (16 samples), smoothed per sample and
  coefficients rebuilt every 4 samples; this keeps THD far below -60 dBc.
- Pan: L/R mode crossfades the effect L↔R; M/S mode applies it to mid/side.
- Freeze holds the current gains; SC audition outputs the filtered sidechain;
  Bypass is bit-transparent.

## UI

650×650 design, resizable 0.5×–2× (square), persisted size and theme. Three
themes: Dark (default), Light, Pink pastel, selectable with the visible skin
button; theme + size are saved in the plugin state and in
`~/.config/AbynthAudio/CarveSpace.conf`.

## Build / test

See README.md and docs/BUILD.md. `CarveSpaceHost` (tools/host) is an offline
test host: dump parameters, render files with a sidechain, take screenshots.
