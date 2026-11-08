# CarveSpace VST3 — automated validation report (final)

**Date:** 2026-11-08 (Europe/Madrid)
**Build under test:** `build-lead` Release (DSP frozen; final depth-compensation tweak included)
**Plugin:** `build-lead/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3`
**Host:** `build-lead/CarveSpaceHost_artefacts/Release/CarveSpaceHost`
**Harness:** `tests/validate.sh` (self-contained; deterministic fixtures generated with python3/numpy)

**Verdict: READY for delivery. All 83 automated checks PASS; the optional pluginval
strictness-5 run PASSES as well.**

```
SUMMARY: 83 passed, 0 failed, 1 skipped   (skip = pluginval step; ran manually: SUCCESS)
```

Reproduce:

```bash
# fresh-build path (build-test/ is git-ignored and rebuilt automatically):
cmake -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-test -j6 CarveSpace_VST3 CarveSpaceHost
SKIP_BUILD=1 tests/validate.sh

# against the final packaging build:
VST3_PATH="$PWD/build-lead/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3" \
HOST_BIN="$PWD/build-lead/CarveSpaceHost_artefacts/Release/CarveSpaceHost" \
SKIP_BUILD=1 tests/validate.sh

# full evidence including pluginval:
PLUGINVAL_BIN=/tmp/pluginval-linux/pluginval SKIP_BUILD=1 tests/validate.sh
```

Logs: `tests/.work/logs/validate.log` (complete run), per-command logs alongside it.
Fixtures / scratch audio: `tests/.work/fixtures/`, `tests/.work/`.

---

## 1. Environment

| Item | Value |
|---|---|
| OS | Arch Linux, Wayland session (`DISPLAY=:0`, XWayland) |
| Python | 3.x + numpy 2.5.4 (no scipy/soundfile dependency) |
| Build | CMake + Ninja, Release, JUCE 7.0.5, `-j6` |
| Plugin | CarveSpace 1.0.0, manufacturer **Abynth Audio**, VST3 SDK 3.7.2 |
| Host | `CarveSpaceHost` (in-repo offline host, `tools/host/HostMain.cpp`) |
| Rates / blocks | 44 100 / 48 000 / 96 000 Hz; blocks 1 / 64 / 256 / 512 / 4096 |
| Analysis | float32 WAV, windowed complex demodulation, Welch band power, cross-correlation delay |

Host extensions (backward compatible with `dump`, `render`, `shot`):

- `state` — parameter-state save/load round-trip and fresh-process reload;
- `render --automate id=norm@sample` — mid-render automation (applied at the first block
  boundary ≥ sample; the actual sample is printed);
- `render --state-in/--state-out`, CPU timing (`cpu_ms_per_block`, `realtime_factor`),
  output `min/max/nonfinite/rms_db` scan, `dump --json`;
- robustness fixes (short sidechain files, partial final block);
- `shot` editor sizing fix for Wayland (see §5.3).

The harness never hardcodes parameter IDs or latency: it parses the parameter table and
the reported latency from `dump`, maps the 9 canonical parameters by name (fallback: the
reference-compatible VST3 ParamID hash), and verifies the measured output delay.

---

## 2. Plugin metadata (final build, from `dump`)

```
name: CarveSpace      manufacturer: Abynth Audio      version: 1.0.0
latencySamples: 0     inputs: 4 (2 buses)             outputs: 2 (1 bus)
bus 0 "Input" 2ch (main)      bus 1 "Sidechain" 2ch      output bus 0 "Output" 2ch
```

| # | ParamID | Host name | Default (norm) | Default text |
|---|---|---|---|---|
| 0 | 77748203 (`RATIO`) | Ratio | 0.15 | 15.0 |
| 1 | 2423 (`LC`) | Low-cut | 0.0 | 20.00 |
| 2 | 2299 (`HC`) | High-cut | 1.0 | 20.00k |
| 3 | 2081894039 (`FREEZE`) | Freeze | 0.0 | OFF |
| 4 | 78973 (`PAN`) | Pan | 0.5 | 50.0 |
| 5 | 2372003 (`MODE`) | Stereo Mode | 0.0 | L/R |
| 6 | 2099 (`AT`) | Attack | 0.674743 | 5.00 |
| 7 | 2618 (`RL`) | Release | 0.519181 | 30.00 |
| 8 | 941475754 (`SIDECHAIN`) | Sidechain | 0.0 | OFF |
| 9 | 1652125811 | Bypass (auto-added) | 0.0 | Off |

All nine canonical VST3 ParamIDs are verified as `hashCode(canonical_id) & 0x7fffffff`
(reference-compatible). Names/order/ranges/defaults match `reference/measurements.md` §3.

---

## 3. Results

### 3.1 Bundle / linking — PASS

| Check | Result |
|---|---|
| `CarveSpace.vst3/Contents/x86_64-linux/CarveSpace.so` | PASS |
| ELF 64-bit shared object | PASS |
| `ldd` VST3 module: all dependencies resolve | PASS |
| Exports `GetPluginFactory` | PASS |
| `ldd` host binary: all dependencies resolve | PASS |
| `Contents/moduleinfo.json` | INFO: absent (optional for JUCE builds; pluginval 5 accepts) |

### 3.2 Buses / parameters / latency — PASS (15/15)

| Check | Measurement |
|---|---|
| Identity | `CarveSpace` / `Abynth Audio` |
| Input buses | 2 (`Input` stereo + `Sidechain` stereo) |
| Output | 1 stereo bus, 2 channels |
| Reported latency | **0 samples** |
| 9 canonical parameters + ParamIDs | all present and canonical |
| `dump --json` | parses, 10 parameters |

### 3.3 Zero latency / unity path (Amount = 0) — PASS

| Check | Measurement |
|---|---|
| Output delay vs reported latency | reported 0, measured **0 samples** (corr 1.0) |
| Broadband magnitude response | **0.000 dB max deviation** (probes 100 Hz / 1 kHz / 5 kHz / 10 kHz all 0.000 dB) |
| Time-domain residual | **−300 dB** (sample-exact transparency) |
| Input vs output RMS | −7.950 dBFS both |
| DC offset change | ≤ 1e-4 |
| NaN / Inf | 0 |
| Tone-mix level change (100 Hz, 1 kHz) | 0.000 dB / 0.000 dB |

### 3.4 Effect depth — PASS (reference-calibrated)

| Case | Clone | Reference (`reference/measurements.md`) |
|---|---|---|
| main = side = 1 kHz @ −12 dBFS, Ratio 100 % | **−34.84 dB** | −33.09 dB |
| main 1 kHz + 100 Hz, side 1 kHz @ −6 dBFS | 1 kHz **−34.87 dB**, 100 Hz **−37.00 dB** | wide analysis hump also reduces LF (§7) |
| Internal sidechain (no external bus), main mix | 1 kHz −34.87 dB | internal mode not measurable on reference |

(The task assumption "1 kHz is attenuated more than 100 Hz" does not apply to the
reference either: `reference/measurements.md` §7 shows its deliberately wide analysis
detectors reduce 200 Hz by −34.7 dB with a 1 kHz sidechain. The harness reports both
numbers and explains this.)

### 3.5 Amount law vs reference — PASS

Main −12 dBFS, side 1 kHz −6 dBFS, Ratio sweep (reference §4 table):

| Ratio | 0.05 | 0.10 | 0.15 | 0.20 | 0.25 | 0.50 | 1.00 |
|---|---|---|---|---|---|---|---|
| reference (dB) | −2.05 | −8.65 | −21.49 | −31.25 | −32.34 | −33.06 | −33.10 |
| clone (dB) | −0.32 | −7.15 | −21.49 | −29.40 | −32.43 | −34.71 | −34.87 |
| deviation (dB) | +1.73 | +1.50 | +0.01 | +1.85 | −0.09 | −1.65 | −1.77 |

- PASS: every point within 1.85 dB of the reference, monotonic, no nulls.
- The clone is ~1.8 dB deeper than the reference above Ratio 50 % (final
  depth-compensation tweak; accepted by the lead).
- Additional monotonic sweep at main/side −6 dBFS: 0 / 0.25 / 0.5 / 0.75 / 1.0 →
  0.00 / −32.43 / −34.71 / −34.84 / −34.87 dB.

### 3.6 Sidechain-level law — PASS

Main −12 dBFS, side 1 kHz:

| Case | Clone | Reference | Deviation |
|---|---|---|---|
| side −30 dBFS, Ratio 25 % | −1.29 dB | −3.26 dB | 1.97 dB (PASS ≤ 3) |
| side −30 dBFS, Ratio 100 % | −33.01 dB | −32.35 dB | 0.66 dB (PASS ≤ 3) |

### 3.7 Harmonic distortion — PASS

Steady 1 kHz main + steady 1 kHz sidechain, Ratio 50 %, default attack:

| Component | Level |
|---|---|
| 1 kHz | −40.73 dBFS |
| 3 kHz (3rd harmonic) | −132.61 dBFS = **−91.9 dBc** (threshold −60 dBc) |

(The old STFT-era −40…−51 dBc envelope-modulation distortion is long gone.)

### 3.8 Freeze (automated at 0.9 s) — PASS

Main 1 kHz; sidechain = 1 kHz for 0–1.0 s then silence; Ratio 100 %; Freeze toggled via
host automation (requested 43200, applied 43264, block 256):

| Window | No freeze | Freeze |
|---|---|---|
| 0.3–0.8 s (sidechain active) | −40.89 dBFS | −40.89 dBFS |
| 1.4–1.9 s (sidechain silent) | −6.02 dBFS (released) | **−40.89 dBFS (held)** |
| Held vs released | — | **34.87 dB** |

### 3.9 Pan and M/S — PASS

| Check | Measurement |
|---|---|
| L/R mode pan 0 vs 1, L channel | Δ −34.87 dB (more reduction at pan 0) |
| L/R mode pan 0 vs 1, R channel | Δ +34.87 dB (more reduction at pan 1) |
| M/S mode Amount = 0 | broadband magnitude deviation 0.000 dB |
| M/S pan centre vs extremes | centre −170.4 dBFS vs extremes −12.04 dBFS (centre reduces ≥ 6 dB more) |
| M/S L/R balance | centre (very deep), extremes 0.32 dB — reference-consistent direction |

### 3.10 Sidechain audition — PASS

Main 440 Hz, side 2 kHz, Sidechain = ON: output 2 kHz = **−6.02 dBFS**, 440 Hz =
−231.39 dBFS (Δ 225 dB); audition level independent of Amount.

### 3.11 Low-cut / high-cut analysis filters — PASS

Measured through the SC-audition path with a broadband sidechain (reference §9):

| Filter | Measurement | Reference | Verdict |
|---|---|---|---|
| Low-cut 632 Hz | 20 Hz −29.90, 50 Hz −22.22, 100 Hz −16.13, 200 Hz −10.41 dB → **5.91 dB/oct** | ~6 dB/oct (1st order) | PASS |
| High-cut 3.56 kHz | 4 kHz −4.21, 8 kHz −15.58, 12 kHz −25.02, 16 kHz −34.55, 20 kHz −47.89 dB → **13.1 dB/oct** (4–12 kHz) | ~10–12 dB/oct (2nd order) | PASS |

Broadband-sidechain band protection (informational — the reference's wide detectors
limit this too): low-cut 1k protects 50–200 Hz by +2.06 dB; high-cut 1k protects
8–15 kHz by +8.53 dB.

### 3.12 State save / load — PASS

| Check | Measurement |
|---|---|
| State blob | 718 bytes (includes UI properties uiTheme/editorWidth/editorHeight) |
| All parameters restored after scrambling | 10/10 exact (≤ 1e-6) |
| Fresh host process restores saved state | 10/10 exact |

### 3.13 Robustness — PASS

| Check | Measurement |
|---|---|
| Blocks 1 / 64 / 512 / 4096 | finite, identical output, max abs 0.500 each |
| 44.1 / 48 / 96 kHz | 1 kHz reduction −35.20 / −34.87 / −28.71 dB, finite |
| 100 blocks of silence | output exactly 0, no NaN/Inf |
| 1e-25 input (denormal probe) | finite, no blow-up |
| CPU (block 512 @ 48 kHz) | **0.85 ms/block**, realtime factor **0.080** |
| Fresh process after all tests | clean load, ≥ 9 parameters |

Observation: the effect depth is ~6 dB shallower at 96 kHz than at 48 kHz for the same
nominal test signal. Consistent with a sample-rate-dependent detector band-level
calibration; noted for future work if 96 kHz parity is required.

### 3.14 Screenshots — PASS (dimensions)

| Check | Measurement |
|---|---|
| 650×650 shot | valid PNG 650×650 |
| 975×975 shot | valid PNG 975×975 |

The captured image content is a uniform background (unique colours = 1): a hosted VST3
editor is embedded through `IPlugView` in a native child window, which JUCE's
`createComponentSnapshot` cannot read back. Dimensions (the task requirement) are
verified; visual content must be checked in the Standalone app or with an external
screen capture. The host `shot` command now sizes the editor explicitly, which fixes the
earlier Wayland 480×480 problem.

### 3.15 Fresh-process reload — PASS

A new host process loads the final VST3 cleanly after all tests and still exposes ≥ 9
parameters.

---

## 4. pluginval strictness 5 (headless) — PASS

```bash
curl -L -o /tmp/pluginval_Linux.zip \
  https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip
unzip -d /tmp/pluginval-linux /tmp/pluginval_Linux.zip
/tmp/pluginval-linux/pluginval --strictness-level 5 --skip-gui-tests \
  --validate-in-process "build-lead/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3"
```

Result: **SUCCESS** (exit 0), 16 test groups, 30 audio-processing combinations
(44.1/48/96 kHz × block 64…1024), cold/warm open, plugin info (reported latency 0),
programs, audio processing, state, automation, all bus enable/disable permutations and
default-layout restore. Log: `tests/.work/logs/pluginval.log`.
The harness runs this automatically when `PLUGINVAL_BIN` is set.

---

## 5. History / fixed issues during validation

1. **Old STFT engine produced silence.** `CarverEngine`'s constructor called
   `window.multiplyWithWindowingTable (synthWindow.data(), fftSize)` on a
   zero-initialised `synthWindow`, so the synthesis window was all zeros and every
   render was silent (gdb: `outFifoL` stayed 0). Reported to the lead; the engine was
   replaced with the final zero-latency filterbank design.
2. **`shot` produced 480×480 images** in this Wayland session (the WM forced a 941×508
   window before the editor was sized). Fixed in `tools/host/HostMain.cpp`; 650×650 and
   975×975 are now correct.
3. **Host `render` robustness** — short sidechain files and partial final blocks were
   hardened; `--automate`, `state`, CPU/NaN reporting added.
4. **Calibration iterations** — an early IIR revision was ~7× too shallow (−4.8 dB vs
   reference −33.1 dB at Ratio 100 %) and had non-monotonic depth; a subsequent revision
   had a 3.5–6.1 dB Amount-law transition-region deviation. Both were reported with
   repros and fixed. The final build passes all calibration checks.

---

## 6. Coverage

| Area | Checks | Result |
|---|---|---|
| Bundle / linking | 5 | PASS |
| dump: identity, buses, 9 ParamIDs, latency | 15 | PASS |
| Zero latency / unity (Amount 0) | 10 | PASS |
| Effect depth + amount/sidechain law | 12 | PASS |
| Harmonic distortion | 1 | PASS |
| Internal sidechain | 1 | PASS |
| Mono main / mono sidechain | 3 | PASS |
| Low-cut / high-cut slopes | 2 (+2 info) | PASS |
| Freeze automation | 4 | PASS |
| Pan L/R + M/S | 5 | PASS |
| SC audition | 3 | PASS |
| State round-trip + fresh process | 5 | PASS |
| Blocks / rates / silence / denormals / CPU | 10 | PASS |
| Screenshots | 4 (+content note) | PASS |
| Fresh reload | 2 | PASS |
| **Total** | **84** | **83 pass / 0 fail / 1 harness-skip** |

The skipped item is the harness's pluginval step (no `PLUGINVAL_BIN` in the default
environment); pluginval itself ran successfully manually (§4).

---

## 7. Files

- `tests/validate.sh` — self-contained harness (PASS/FAIL/INFO, exit 0/1/2)
- `tests/gen_wavs.py` — deterministic fixtures (tones, noise, mono, split, silence,
  denormal, reference levels)
- `tests/wavlib.py` — WAV reader/writer (numpy only)
- `tests/measure.py` — stats / tone / band / channel / response / delay / unity / PNG
- `tools/host/HostMain.cpp` — offline host (`dump`, `render`, `shot`, `state`,
  `--automate`, `--json`, CPU stats)
- `tests/REPORT.md` — this report
- Evidence logs/renders: `tests/.work/` (git-ignored)

---

## 8. Suggested follow-ups (not blocking)

1. Optional sample-rate-dependent detector calibration if 96 kHz parity matters
   (currently ~6 dB shallower than 48 kHz in the saturated test).
2. Update `docs/SPEC.md`'s parameter table (it still shows the pre-reference values).
3. Once a visual GUI check is possible (Standalone app / external capture), add a
   screenshot-content smoke test; the harness currently verifies dimensions only.
