# Reference measurements — Wavesfactory Trackspacer 2.5.10 (original Windows VST3)

Status: measurements made 2026-11-08 by `reference-dev`. All numbers below are **measured**, not
guessed, unless marked *inferred*. The original was run under Wine and bridged with yabridge;
the deliverable remains the native Linux plugin.

Files in this directory:

- `metadata.txt` — raw `TrackspacerHost dump` output of the bridged original.
- `metadata.json` — machine-readable identity / buses / parameter table with exact ranges.
- `measurements.md` — this document.
- `audio/` — test inputs (48 kHz 32-bit float WAVs).
- `audio/measurements/` — the actual **rendered outputs** used for the numbers below.
- `screenshots/original-650.png`, `original-1300.png` — UI screenshots (limitation: editor renders
  blank/grey through Wine+yabridge; see §10).

---

## 1. Measurement setup (and why the external sidechain data is valid)

```
WINEPREFIX=/tmp/refopt/wineprefix          HOME=/tmp/refhome
Wine 11.21 portable (Kron4ek build)         yabridge 5.1.1
Original: /tmp/ts_install/cf64/VST3/Trackspacer25.vst3  (single-file PE32+ DLL, JUCE 7.0.5)
Bridged bundle: /tmp/refhome/.vst3/yabridge/Trackspacer25.vst3
Host: build-ref/TrackspacerHost dump|render|shot   (JUCE 7.0.5 VST3 hosting, 4-ch buffer:
      ch0-1 = main, ch2-3 = sidechain)
Reproduce: source /tmp/refenv_refdev.sh   (unique env file; another agent overwrote /tmp/refenv.sh)
```

**Sidechain bus control test (disproves the "sidechain bus is not really connected" worry):**

| render | main | side | Ratio | measured steady gain |
|---|---|---|---|---|
| `M3a_mix_ctl_silent.wav` | noise+1k mix | **silence** | 1.0 | **0.000 dB**, corr = 1.000000, max sample diff 0.0013 |
| `B_control_silent.wav` | white noise −18 dBFS | **silence** | 1.0 | **0.000 dB** |
| `A_self_sc.wav` | 1 kHz −12 dBFS | 1 kHz −12 dBFS | 1.0 | **−33.09 dB** |
| `B_shape.wav` | white noise −18 dBFS | 1 kHz −6 dBFS | 1.0 | −8.0 dB overall / −33 dB in most bands |

Conclusion: the external sidechain bus **is active and used**. A silent external sidechain produces
bit-transparent output (no internal-sidechain fallback happens when the bus is active but silent);
a non-silent external sidechain changes the output. All A–J measurements below are therefore valid
external-sidechain measurements.

**Cross-check of the spectral method (answers the "−6.6 dB RMS vs −33 dB spectrum" concern):**
for `M3b_noise_self.wav`, Parseval gives `sum(P_out)/sum(P_in) = −8.76 dB`, which equals the
time-domain RMS change (−8.76 dB) exactly. The per-bin attenuations really are ≈−33 dB below
~12 kHz and only ≈−2…−8 dB above 15 kHz; the total RMS is dominated by the nearly-unattenuated
HF content, so it under-reports the reduction. There is no normalisation or file-mismatch bug:
every render used the exact same `--main` file that is also the analysis reference.

**Known limitation:** a render *without* `--side` (sidechain bus deactivated, 2-in layout) makes
`libyabridge-vst3.so` segfault inside `processAudio` (yabridge 5.1.1 / 2-input-bus deactivated-bus
bug). Internal-sidechain mode therefore cannot be rendered directly; it was emulated by feeding
`--side <same file as --main>`, which is equivalent if internal mode analyses the main input.
(`dump` works, and all external-sidechain renders work.)

---

## 2. Identity, latency, buses

| item | value |
|---|---|
| name / descriptive | `Trackspacer 2.5` |
| manufacturer | `Wavesfactory` |
| version | `2.5.10` |
| **latencySamples** | **0** |
| tailSamples | 0 |
| inputs / outputs | **4 / 2** |
| input bus 0 | `Input`, 2 ch, Stereo |
| input bus 1 | `Sidechain`, 2 ch, Stereo (VST3 aux) |
| output bus 0 | `Output`, 2 ch, Stereo |

**Latency is really 0** — measured sample-exactly: an impulse at sample 24000 (main) appears at
output sample 24000, both with silent sidechain (dry path) and with the reduction active.
There is no STFT/overlap-add delay. This is an **IIR / filterbank design**, so the clone's
2048-sample STFT latency is not compatible; the clone must report and deliver 0 samples.

---

## 3. Parameters — exact VST3 table

`getNumSteps = 2147483647` for continuous params; `2` for booleans.

| # | id | name | label | default norm | default text | range / law |
|---|---|---|---|---|---|---|
| 0 | 77748203 | **Ratio** (UI: AMOUNT) | % | **0.15** | 15.0 | linear, 0…100 % (text = 100·n) |
| 1 | 2423 | Low-cut | Hz | 0.0 | 20.00 | log, f = 20·10^(3n) Hz, 20…20000 |
| 2 | 2299 | High-cut | Hz | 1.0 | 20.00k | log, f = 20·10^(3n) Hz, 20…20000 |
| 3 | 2081894039 | Freeze | — | 0 | OFF | bool, OFF < 0.5 ≤ ON |
| 4 | 78973 | Pan | % | 0.5 | 50.0 | linear 0…100 (text = 100·n) |
| 5 | 2372003 | Stereo Mode | — | 0 | L/R | choice: 0 = L/R, 1 = M/S |
| 6 | 2099 | Attack | ms | 0.674743 | 5.00 | log, t = 0.01·10^(4n) ms, **0.01…100 ms** |
| 7 | 2618 | Release | ms | 0.519181 | 30.00 | log, t = 700^n ms, **1…700 ms** |
| 8 | 941475754 | Sidechain (SC Audition) | — | 0 | OFF | bool |
| 9 | 1652125811 | **Bypass** | — | 0 | Off | bool — **undocumented 10th parameter** |

Text at 0 / 0.25 / 0.5 / 0.75 / 1 (verbatim from dump):

- Ratio: 0.0 / 25.0 / 50.0 / 75.0 / 100.0
- Low-cut & High-cut: 20.00 / 112.47 / 632.46 / 3.56k / 20.00k
- Pan: 0.0 / 25.0 / 50.0 / 75.0 / 100.0
- Attack: 0.01 / 0.10 / 1.00 / 10.00 / 100.00 ms
- Release: 1.00 / 5.14 / 26.46 / 136.09 / 700.00 ms

SPEC corrections needed: amount default is **15 %** (not 0), internal name `Ratio`; attack minimum
is **0.01 ms**; release maximum is **700 ms**; a **Bypass** parameter exists.

---

## 4. Amount law (A) — highly non-linear, steep

Main = 1 kHz sine −12 dBFS, side = 1 kHz sine −6 dBFS, 10 s renders, steady window 8.0–9.5 s,
gain measured on the output 1 kHz tone. **Steady: no drift over 10 s** (per-second gain constant).

| Ratio norm | Ratio % | gain dB |
|---|---|---|
| 0.02 | 2 | −0.32 |
| 0.05 | 5 | −2.05 |
| 0.10 | 10 | −8.65 |
| 0.15 | 15 | −21.49 |
| 0.20 | 20 | −31.25 |
| 0.25 | 25 | −32.34 |
| 0.30 | 30 | −32.74 |
| 0.35 | 35 | −32.91 |
| 0.40 | 40 | −32.99 |
| 0.50 | 50 | −33.06 |
| 0.75 | 75 | −33.09 |
| 1.00 | 100 | **−33.10** |

The jump is real (confirmed with 10 s steady renders, two different side levels and a second run).
`Ratio` is not a linear depth knob.

Same measurement with side = −30 dBFS (less saturated): 0.05 −0.13, 0.10 −0.51, 0.15 −1.15,
0.20 −2.07, 0.25 −3.26, 0.30 −4.75, 0.40 −8.74, 0.50 −14.30, 0.75 −30.65, 1.00 −32.35 dB.

---

## 5. Sidechain level law (D) and the combined law fit

Gain of the 1 kHz main vs sidechain level (10 s, Ratio = 1.0 / 0.5 / 0.25):

| side dBFS | Ratio 1.0 | Ratio 0.5 | Ratio 0.25 |
|---|---|---|---|
| 0 | −33.10 | −33.09 | — |
| −6 | −33.10 | −33.06 | −32.34 |
| −12 | −33.09 | −32.91 | −29.59 |
| −18 | −33.06 | −32.34 | −14.22 |
| −24 | −32.92 | −29.67 | −6.70 |
| −30 | −32.35 | — | −3.26 |
| −36 | −29.74 | −6.73 | −1.61 |
| −48 | −6.76 | −1.62 | −0.40 |
| −60 | −1.63 | −0.40 | −0.10 |

The curves are **shifted copies of one sigmoid**: changing Ratio shifts the threshold, it does not
change the knee shape. Fitted (least squares over all of the above + the amount-law points):

```
depth_dB(L, amount) = 33.10 / (1 + exp(-(L - T(amount)) / 3.75))
T(amount)           = -43.5 - 42.0 * log10(amount_norm)          [dBFS]
```

- `L` = band level in dBFS (a tone at X dBFS reads ≈ X in its band).
- `amount_norm` = Ratio normalised 0…1 (guard > 0).
- Dmax = **33.10 dB**; knee width ≈ 3.75 dB logistic scale (≈2.2 dB of reduction per dB of
  level at the knee, i.e. an expander-like slope, saturating at 33.1 dB).
- Fit RMSE ≈ 1.6 dB across 30 points. This is the single most useful calibration for the clone.

Practical consequence: at Ratio = 100 %, any band whose level is above ≈ −35 dBFS is already at
or near full −33 dB reduction; at Ratio = 15 %, it takes a band level around −9 dBFS to reach
−21 dB depth. This is why the UI default 15 % sounds moderate and 25 % already sounds extreme
with loud sidechain material.

---

## 6. Filterbank structure — **the key discovery**

Reduction curve of a saturated render (`M3a_mix_self.wav`, main = noise + 1 kHz sine, side = same,
Ratio = 1.0) at 1/48-oct resolution shows a **log-periodic pattern**:

- **band centres (least attenuation, ≈ −33 dB)** at
  106.9, 133.3, 165.5, 208.0, 260.7, 323.7, 405.8, 508.3, 632.8, 791.0, 990.2, 1236.3, 1541.0,
  1930.7, 2409.7, 3010.3, 3767.6, 4710.9, 5893.1, 7387.2 Hz … → **ratio ≈ 1.2495**
  (≈ **0.321 octave per band ≈ 3.1 bands/octave**; 32 bands × 0.32 oct ≈ 10.3 octaves,
  consistent with the documented **32 bands** covering ≈18 Hz … ≈22 kHz).
- **band boundaries (deep notches)** midway between centres (geometric mean), same ratio 1.2495:
  118.7, 147.9, 186.0, 231.4, 290.0, 361.8, 452.6, 565.4, 707.5, 883.3, 1104.5, 1379.9, 1724.1,
  2154.8, 2692.4, 3363.3, 4204.1, 5252.9, 6564.0, 8201.7, 10249.5 Hz. Attenuation there is
  **deeper than at the centres and grows with frequency**: −46.5 dB at 119 Hz up to ≈ −76 dB at
  6.5 kHz (then less deep as the HF roll-off starts).

Interpretation (from data, to be validated by the clone's design): the original is a **32-band
log filterbank with ≈1.25 frequency spacing between band centres**, where each band contributes a
gain (max ≈ −33.1 dB at the centre) and the band responses overlap; the measured "notches" at
band boundaries are the signature of the filterbank summing (parallel/overlap structure), not a
simple smooth EQ curve. A clone that applies one smooth gain curve interpolated over FFT bins
will **not** reproduce the boundary-notch pattern; a clone that applies 32 band gains through a
filterbank (or 32 peaking sections) will.

Frequency-dependent max attenuation (tone self-sidechain at −12 dBFS, Ratio 1.0, gain at tone):

| tone Hz | 50 | 100 | 200 | 400 | 800 | 1k | 2k | 4k | 8k | 12k | 16k | 20k |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gain dB | −41.6 | −36.8 | −34.3 | −33.2 | −33.1 | −33.1 | −33.7 | −34.7 | −34.8 | −20.6 | −42.7 | −56.2 |

So the effective maximum reduction is ≈−33 dB in the 400 Hz–8 kHz range, deeper at the frequency
extremes (LF and >15 kHz), and only ≈−21 dB around 12 kHz.

---

## 7. Reduction shape around 1 kHz (B) and broadband noise (C)

**B** — main = white noise −18 dBFS, side = 1 kHz sine −6 dBFS, Ratio 1.0, attack fastest,
release longest, output/input magnitude spectrum (`B_shape.wav`):

| Hz | 200 | 315 | 400 | 500 | 630 | 700 | 800 | 900 | 1000 | 1120 | 1250 | 1400 | 1600 | 2000 | 2500 | 3150 | 4000 | 6000 | 8000 | 12000 | 16000 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| dB | −34.7 | −33.7 | −33.2 | −33.2 | −33.0 | −50.4 | −33.1 | −45.1 | −33.1 | −47.2 | −33.0 | −46.3 | −33.9 | −33.8 | −33.4 | −33.6 | −34.7 | −29.0 | −35.0 | −20.6 | −55.5 |

Caveat: at Ratio 100 % / side −6 dBFS the analysis is saturated, so the plateaus are the max depth
and the −45…−55 dB spikes are the band-boundary notches from §6 (the measurement noise floor is
−90 dB, so they are real). In the unsaturated regime the shape is a broad hump around the side
component: for the same setup at side −30 dBFS the curve is 100 Hz −5.7, 200 −11.1, 300 −16.8,
400 −22.9, 500 −28.2, 630 −31.5, 800 −32.3, 900 −44.4, 1000 −32.4, 1120 −46.4, 1250 −32.2,
1600 −32.5, 2000 −27.8, 3000 −18.1, 5000 −10.1, 8000 −5.5, 12000 −2.7 dB — i.e. a **wide hump
≈6 octaves wide** (≈8–9 dB/oct skirts) around the driven band, again consistent with broad
overlapping analysis bands plus the steep level law.

**C** — main = white noise −18 dBFS, side = **independent** white noise −18 dBFS
(`C_ratio_*.wav`), output/input spectrum:

| Ratio | 100 Hz | 200 | 500 | 1k | 2k | 5k | 10k | min | out RMS |
|---|---|---|---|---|---|---|---|---|---|
| 0.15 | −0.48 | −0.66 | −1.07 | −1.46 | −1.95 | −2.55 | −2.36 | −2.60 | −19.57 dBFS |
| 0.30 | −1.90 | −2.65 | −4.27 | −5.85 | −7.89 | −10.50 | −9.90 | −10.80 | −23.24 |
| 0.50 | −5.24 | −7.33 | −11.88 | −16.35 | −22.40 | −31.61 | −31.57 | −73.07 | −26.53 |
| 1.00 | −20.36 | −28.26 | −33.08 | −33.04 | −33.78 | −33.73 | −31.69 | −76.42 | −26.79 |

No phase inversion / gain increase; the `min` column is the deepest band-boundary notch. The
smooth low→high tilt (LF less reduced) reflects the band-level spectrum of white noise in a
log filterbank plus the band-boundary structure.

---

## 8. Attack / release (E)

Stimulus: main = steady 1 kHz −12 dBFS; side = 1 kHz −6 dBFS gated exactly ON at 1.0 s and OFF at
2.0 s (`--set Ratio=1 --set Attack=… --set Release=…`, 0.5 ms envelopes, `BAL_*.wav`).
10–90 % step times of the output envelope:

**Attack** (side turns on; measured 90 % of the drop):

| Attack norm | 0.00 | 0.25 | 0.50 | 0.75 | 1.00 |
|---|---|---|---|---|---|
| nominal (getText) | 0.01 ms | 0.10 ms | 1.00 ms | 10.0 ms | 100 ms |
| measured 10–90 % | 3 ms | 3 ms | 4 ms | 1–2 ms | 5–7 ms |

The Attack knob has only a weak influence on the *effective* gain reduction speed: the measured
onset is always a few ms because the steep level law (§5) converts a slowly rising detector
envelope into a fast gain change. For the clone: implement the knob as a per-band one-pole with
the nominal time constant, but expect the audible onset to be ~3–7 ms.

**Release** (side turns off):

| Release norm | 0.00 | 0.25 | 0.50 | 0.75 | 1.00 |
|---|---|---|---|---|---|
| nominal (getText) | 1.00 ms | 5.14 ms | 26.5 ms | 136 ms | 700 ms |
| measured 10–90 % recovery | 4 ms | 18 ms | 89 ms | 452 ms | 1894 ms |
| ratio measured/nominal | 4.0 | 3.5 | 3.36 | 3.32 | 2.7 |

=> the release knob behaves like a one-pole **time constant** equal to the displayed ms value, and
the 10–90 % envelope recovery is ≈3.3–3.4× that. (The 700 ms setting is effectively ~1.9 s for a
full recovery; there is a long tail.)

Freeze (parameter 3, ON from the beginning): no reduction is ever applied — it holds the current
reduction curve, which is 0 dB until some reduction has occurred (`H_freeze_on.wav` = flat 0 dB;
`H_freeze_off.wav` shows the normal −33 dB while the side gate is open).

---

## 9. Low-cut / high-cut filters (F) and SC Audition

Manual: Low-cut/High-cut limit "the range to which the effect is applied"; SC Audition outputs the
sidechain with those filters applied. Measured via SC Audition (main = silence, side = white noise,
`Sidechain=ON`; output/input spectrum; the audition path has a small auto-gain, so read slopes, not
absolute levels):

- **Low-cut** = high-pass: at Low-cut = 632 Hz the response below the corner is
  −26.4 dB @20 Hz, −18.8 @50, −12.7 @100, −6.4 @200, ≈0 @400 Hz → **≈6 dB/octave (1st order)**.
- **High-cut** = low-pass: at High-cut = 3.56 kHz the response above the corner is
  −6.2 dB @4 kHz, −10.4 @5 k, −13.6 @6.3 k, −16.5 @8 k, −19.3 @10 k, −22.3 @12.5 k,
  −26.6 @16 k, −33.3 @20 k → **≈12 dB/octave (2nd order)**.
- Default filters (20 Hz / 20 kHz) are essentially flat: +0.2 dB from 20 Hz–2 kHz, −0.4 @5 k,
  −1.7 @10 k, −4.2 @16 k (a gentle HF roll-off of the sidechain analysis path).
- SC Audition output level ≈ **+0.17 dB** relative to the side input at 1 kHz, and it is
  **independent of Amount** (`H_scaud_tone.wav` vs `H_scaud_tone_amt0.wav` are bit-identical).

The filters act on the **analysis sidechain** only: with Ratio = 0, cutting the filters to extremes
leaves the output **exactly equal to the input** (`F_amt0_extreme.wav`, gain 0.00 dB), so they do
not filter the main path.

Note/limitation: renders with Low-cut/High-cut engaged plus a strong sidechain tone showed
unexpected behaviour (e.g. Low-cut = 2 kHz produced −80 dB attenuation above 2 kHz, High-cut =
500 Hz made the reduction appear only below 500 Hz). That is consistent with the filters shaping
the per-band analysis/detection (and the audible sidechain content), but the exact interaction
between the cut filters and the band gains was not fully resolved — treat extreme cut settings as
**not calibrated** and reproduce them with the 6/12 dB-oct filters + the §5 law first.

---

## 10. Pan / Stereo Mode (G), Bypass, UI screenshot

Main = split signal (L = 1 kHz, R = 200 Hz, −12 dBFS each), side = copy of main, Ratio 1.0,
10 s, gain per channel:

**L/R mode**

| Pan | 0.00 | 0.25 | 0.50 | 0.75 | 1.00 |
|---|---|---|---|---|---|
| L (1 kHz) | **−33.10** | −33.09 | −33.08 | −33.02 | **0.00** |
| R (200 Hz) | **0.00** | −34.28 | −34.34 | −34.35 | **−34.34** |

(0 = effect on left only, 1 = effect on right only; every intermediate value measured reduces
both channels almost fully — the crossfade is only observable at the extreme settings with this
test signal.)

**M/S mode** (same signal)

| Pan | 0.00 | 0.25 | 0.50 | 0.75 | 1.00 |
|---|---|---|---|---|---|
| L | −2.99 | −33.08 | −33.08 | −33.08 | −2.99 |
| R | −3.03 | −34.27 | −34.34 | −34.28 | −3.03 |

M/S, main = mid-only 1 kHz, side = L-only 1 kHz: Pan 0.00 → L = R = −33.09 dB, 0.50 → −33.06 /
−33.06, 1.00 → 0.00 / 0.00.

Bypass (param 9) = ON, Ratio 1: output gain **0.0000 dB** (bit-transparent).

**UI screenshot limitation:** `TrackspacerHost shot` produces valid PNGs
(`screenshots/original-650.png`, `original-1300.png`) but the editor contents render as a flat
grey panel with the JUCE splash in the corner — the original uses OpenGL on Windows and its
component tree does not paint through Wine/yabridge into JUCE's `createComponentSnapshot`.
The extracted original background assets in `/tmp/tsimg_2.png` (1300×1300) remain the best UI
reference. Both screenshot files are kept for completeness.

---

## 11. Recommended constants for the native clone (calibration summary)

1. **Latency: 0 samples.** Do not use an STFT/overlap-add engine (the current 2048-sample design
   is incompatible). Use a per-band IIR/filterbank design.
2. **32 bands**, log-spaced with centre ratio **≈1.2495** (0.321 octave / ~3.11 bands per octave),
   covering roughly 18 Hz … 22 kHz (measured centres listed in §6).
3. **Max depth 33.1 dB** at Ratio = 100 % (band centres; deeper at band boundaries — §6).
4. **Per-band gain law** (primary calibration):
   `depth_dB = 33.10 / (1 + exp(-(L - T)/3.75))`, `T = -43.5 - 42.0*log10(amount_norm)`.
   `L` = band level dBFS. This reproduces the amount law, the level law and the 15 % default.
5. **Analysis selectivity:** the band detectors are broad. A 1 kHz side tone at −30 dBFS still
   produces ≈−5.7 dB reduction at 100 Hz and ≈−10.7 dB at 5 kHz (≈ −19 dB / −16 dB of analysis
   leakage at 3.3 / 2.3 octaves). Implement band filters with gentle skirts (≈6–9 dB/oct) or an
   equivalent overlapping-band detector; a sharp 1/3-octave bandpass will be far too selective.
6. **Attack** = per-band one-pole, time constant from the displayed value (0.01…100 ms); effective
   onset measured 3–7 ms across the whole range.
7. **Release** = per-band one-pole, time constant = displayed value (1…700 ms); 10–90 % recovery
   ≈3.4× the displayed value (≈4 ms … 1.9 s).
8. **Low-cut** = 1st-order high-pass (≈6 dB/oct), **High-cut** = 2nd-order low-pass (≈12 dB/oct),
   both applied to the sidechain analysis only, 20 Hz…20 kHz log mapping (f = 20·10^(3n)).
9. **Amount/Ratio** parameter: normalised 0…1 → text 0…100 %, **default 0.15**; internal name is
   `Ratio`. Pan: 0…100 %, default 50. Stereo Mode: L/R (0) | M/S (1). Sidechain: bool (audition).
   Add **Bypass** (bool) if parameter-count compatibility matters.
10. **Freeze** holds the current per-band gains (enabling it before any sidechain activity freezes
    0 dB = no effect).
11. **SC Audition** outputs the filtered sidechain (≈unity gain, independent of Amount).
12. Band-boundary notch pattern (§6) is a strong signature of the original's filterbank summing.
    If the clone uses 32 peaking sections or an equivalent structure, this pattern will appear
    naturally; if it uses a smooth interpolated gain curve it will not — decide how much this
    matters for "interchangeability".

## 12. What failed / could not be measured

- Internal-sidechain mode (sidechain bus deactivated, 2-in layout) crashes yabridge 5.1.1 inside
  `processAudio`; emulated with `side = copy of main`. Cannot verify whether the real internal mode
  differs (e.g. uses pre-filtered main).
- UI screenshot content (blank/grey through Wine, see §10).
- Exact LC/HC interaction with the reduction at extreme settings (§9 caveat).
- The exact phase response of the band filterbank / whether the reduction is a parallel sum or a
  subtraction structure; §6 gives the magnitude signature only.
- Demo-mode behaviour (the original is unlicensed in this environment; no 45 s noise burst was
  observed within our ≤10 s renders, so it did not affect the measurements).

---

## 13. Comparison with our clone (as of build 2026-11-08 19:04, `build-ref/CarveSpace.vst3`)

Dump of the clone: `reference/clone_dump.txt`. The clone had already been updated with the
reference parameter IDs/names/defaults and 0 latency by the time of this comparison; the DSP
numbers below are from that build and are expected to change as the DSP is rewritten. Re-run with:

```
build-ref/CarveSpaceHost dump build-ref/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3
```

### 13.1 Metadata / bus / latency

| item | reference | clone (19:04) | verdict |
|---|---|---|---|
| latencySamples | 0 | 0 | OK (impulse measured 0 for both) |
| inputs / outputs | 4 / 2 | 4 / 2 | OK |
| input buses | Input (stereo), Sidechain (stereo) | same names/layouts | OK |
| output bus | Output (stereo) | same | OK |
| param count | 10 | 10 | OK (Bypass included) |
| param IDs | 77748203, 2423, 2299, 2081894039, 78973, 2372003, 2099, 2618, 941475754, 1652125811 | identical | OK |
| identity | Trackspacer 2.5 / Wavesfactory | CarveSpace / Exxo | intentional |

### 13.2 Parameter mapping mismatches (getText at 0 / .25 / .5 / .75 / 1)

| param | reference | clone | action |
|---|---|---|---|
| Low-cut | 20.00 / 112.47 / 632.46 / 3.56k / 20.00k | 20 Hz / 39 Hz / 632 Hz / 4.7 kHz / 20 kHz | **wrong law**: use f = 20·10^(3n) Hz |
| High-cut | same as reference | same as Low-cut clone | **wrong law**: f = 20·10^(3n) Hz |
| Attack | 0.01 / 0.10 / 1.00 / 10.0 / 100 ms (default norm 0.674743) | 0.01 / 0.02 / 1.00 / 14.7 / 100 ms (default 0.63749) | **wrong law + default norm**: use t = 0.01·10^(4n), default 0.674743 |
| Release | 1.00 / 5.14 / 26.46 / 136.09 / 700 ms (default 0.519181) | 1.00 / 1.93 / 26.5 / 178 / 700 ms (default 0.513818) | **wrong law + default norm**: use t = 700^n, default 0.519181 |
| Freeze | OFF/…/OFF at 0.5/ON at 0.75 | Off at 0, On at 0.5 | threshold should be > 0.5 (ref shows OFF at 0.5) |
| Sidechain | OFF/…/OFF at 0.5/ON at 0.75 | On at 0.5 | threshold > 0.5 |
| Bypass | Off/Off/On/On/On | Off/Off/On/On/On | OK |
| text format | numeric only, e.g. `20.00`, `3.56k`, `5.00`, `OFF` | units embedded, e.g. `20 Hz`, `4.7 kHz`, `5.00 ms`, `Off` | match reference strings if interchangeability is wanted (VST3 shows the label separately) |

### 13.3 Audio behaviour mismatch (build 19:04)

| measurement | reference | clone (19:04) | action |
|---|---|---|---|
| max reduction, Ratio 1.0, side 1k −6 dBFS | **−33.10 dB** | −6.04 dB | max depth must be 33.1 dB |
| amount law 0.15 / 0.25 / 1.0 | −21.49 / −32.34 / −33.10 dB | −9.89 / −6.33 / −6.04 dB | apply §5 law |
| side silent, Ratio 1 | 0.000 dB | 0.000 dB | OK |
| reduction shape (noise main, 1k tone side, Ratio 1) | ≈−33 dB from 100 Hz–8 kHz, −20.6 @12 k, HF rolloff | flat ≈−4…−6 dB | needs filterbank (§6) + level law (§5) |
| multitone self-sidechain | −36.8 / −33.2 / −33.1 / −34.7 / −20.6 dB @ 100/400/1k/4k/12k | ≈−4…−6.7 dB flat | same |
| level law, Ratio 1, side −36 / −30 / −24 dBFS | −29.74 / −32.35 / −32.92 dB | −6.99 / −6.25 / −6.10 dB | same |
| release 10–90 % at norm 1.0 (700 ms) | 2925 ms | 2977 ms | good match |
| attack 90 % at norm 1.0 (100 ms) | 13 ms | ~0 ms (too fast) | add the attack smoothing |
| latency (impulse) | 0 | 0 | OK |

### 13.4 Concrete next steps for the clone DSP

1. Bypass/level: implement the §5 per-band law **depth_dB = 33.10/(1+exp(-(L−T)/3.75))**,
   **T = −43.5 − 42.0·log10(amount)**, `L` = band level in dBFS. This single change fixes the
   max depth, the amount law (incl. the 15 % default), and the sidechain level law.
2. Replace the STFT gain-curve engine with a 32-band log filterbank (centre ratio ≈1.2495,
   ≈18 Hz–22 kHz) whose per-band gains are the law above; the band-boundary notch signature
   (−46…−76 dB) comes from the filterbank summing and will not appear with a smooth FFT-bin curve.
3. Use broad analysis bands (≈6–9 dB/oct skirts; measured leakage −19 dB at 3.3 octaves,
   −16 dB at 2.3 octaves). A narrow 1/3-octave bandpass is too selective.
4. Fix parameter getText laws: Low-cut/High-cut f = 20·10^(3n); Attack t = 0.01·10^(4n) ms;
   Release t = 700^n ms (defaults 0.674743 / 0.519181); boolean thresholds > 0.5 for
   Freeze/Sidechain; reference text formatting.
5. Attack: per-band one-pole with the displayed time constant; measured effective onset 3–7 ms.
   Release: time constant = displayed value; measured 10–90 % ≈ 3.4× that.
6. Low-cut 1st-order HPF, High-cut 2nd-order LPF on the analysis signal only.
