#!/usr/bin/env python3
"""Generate the deterministic WAV fixtures used by tests/validate.sh.

All files are stereo float32 WAVs.  Usage:
    python3 tests/gen_wavs.py <output-dir>
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wavlib import write_wav_float32  # noqa: E402

DURATION = 2.0
AMP = 0.5


def tone(freq, sr, duration, amp=AMP, phase=0.0):
    t = np.arange(int(round(sr * duration)), dtype=np.float64) / sr
    return amp * np.sin(2.0 * np.pi * freq * t + phase)


def stereo(signal):
    return np.stack([signal, signal], axis=1)


def noise(sr, duration, amp=0.4, seed=1):
    rng = np.random.default_rng(seed)
    n = int(round(sr * duration))
    return amp * rng.standard_normal(n)


def write(path, samples, sr):
    write_wav_float32(path, samples, int(sr))


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "tests/fixtures"
    os.makedirs(outdir, exist_ok=True)

    for sr in (48000, 44100, 96000):
        tag = str(sr)
        dur = 0.75 if sr != 48000 else DURATION

        # Mixed main signal: 1 kHz + 100 Hz (effect / unity / band tests)
        mix = tone(1000.0, sr, dur) + tone(100.0, sr, dur)
        write(os.path.join(outdir, "main_mix_%s.wav" % tag), stereo(mix), sr)

        # Single 1 kHz main tone
        write(os.path.join(outdir, "main_1k_%s.wav" % tag),
              stereo(tone(1000.0, sr, dur)), sr)

        # Deterministic broadband main (latency estimate, filter tests)
        write(os.path.join(outdir, "main_noise_%s.wav" % tag),
              stereo(noise(sr, dur, amp=0.4, seed=11)), sr)

        # Sidechain tone at 1 kHz
        write(os.path.join(outdir, "side_1k_%s.wav" % tag),
              stereo(tone(1000.0, sr, dur)), sr)

        # Broadband sidechain (independent noise)
        write(os.path.join(outdir, "side_noise_%s.wav" % tag),
              stereo(noise(sr, dur, amp=0.4, seed=22)), sr)

        # L-only main: 1 kHz on L, silence on R (M/S + pan checks)
        left = tone(1000.0, sr, dur)
        write(os.path.join(outdir, "main_split_%s.wav" % tag),
              np.stack([left, np.zeros_like(left)], axis=1), sr)

        # 440 Hz main, 2 kHz sidechain (sidechain audition)
        write(os.path.join(outdir, "main_440_%s.wav" % tag),
              stereo(tone(440.0, sr, dur)), sr)
        write(os.path.join(outdir, "side_2k_%s.wav" % tag),
              stereo(tone(2000.0, sr, dur)), sr)

        # Freeze fixture: sidechain tone for the first second, then silence.
        pat = tone(1000.0, sr, dur).copy()
        pat[int(sr * 1.0):] = 0.0
        write(os.path.join(outdir, "side_pattern_%s.wav" % tag), stereo(pat), sr)

        # Reference-calibration fixtures (levels match reference/measurements.md).
        write(os.path.join(outdir, "main_1k_m12_%s.wav" % tag),
              stereo(tone(1000.0, sr, dur, amp=0.2511886)), sr)
        write(os.path.join(outdir, "side_1k_m12_%s.wav" % tag),
              stereo(tone(1000.0, sr, dur, amp=0.2511886)), sr)
        write(os.path.join(outdir, "side_1k_m30_%s.wav" % tag),
              stereo(tone(1000.0, sr, dur, amp=0.0316228)), sr)

        # Mono variants (mono main input and mono sidechain input are allowed).
        write(os.path.join(outdir, "main_1k_mono_%s.wav" % tag),
              tone(1000.0, sr, dur)[:, None], sr)
        write(os.path.join(outdir, "side_1k_mono_%s.wav" % tag),
              tone(1000.0, sr, dur)[:, None], sr)

        # 100 blocks of 512 samples of silence
        write(os.path.join(outdir, "silence_%s.wav" % tag),
              np.zeros((100 * 512, 2), dtype=np.float64), sr)

        # Extremely quiet signal: denormal / numerical blow-up probe
        write(os.path.join(outdir, "main_tiny_%s.wav" % tag),
              stereo(1.0e-25 * tone(1000.0, sr, min(dur, 1.0))), sr)

    print("wrote fixtures to", outdir)


if __name__ == "__main__":
    main()
