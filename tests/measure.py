#!/usr/bin/env python3
"""Audio measurement helpers for tests/validate.sh.

Every command prints `key=value` lines (values are decimal floats / ints) so the
shell driver can parse them with a tiny grep/cut helper.

Commands:
  stats   <wav>
  delay   <input.wav> <output.wav> [--max N]
  unity   <input.wav> <output.wav> <latency>
  tone    <wav> --sr SR --freqs "100,1000" [--start S] [--dur S] [--channel mix|0|1]
  bands   <wav> --sr SR [--start S] [--dur S] [--bands "50:200,8000:15000"]
  channels <wav> --sr SR [--start S] [--dur S]
  peak    <wav>
  png     <pngfile>
"""

import argparse
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wavlib import read_wav, rms_db, to_mono  # noqa: E402


def emit(key, value, decimals=6):
    if isinstance(value, (int, np.integer)):
        print("%s=%d" % (key, value))
    else:
        print("%s=%.*f" % (key, int(decimals), float(value)))


def load(path):
    samples, sr = read_wav(path)
    return samples, sr


def segment(samples, sr, start, dur):
    n = samples.shape[0]
    first = int(round(max(0.0, start) * sr))
    count = int(round(dur * sr)) if dur > 0 else n - first
    first = min(first, n)
    count = max(0, min(count, n - first))
    return samples[first:first + count]


def hann(n):
    if n <= 1:
        return np.ones(max(n, 1))
    return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(n) / (n - 1))


def channel_signal(seg, channel):
    if channel == "mix":
        return to_mono(seg)
    index = int(channel)
    return seg[:, min(index, seg.shape[1] - 1)]


def tone_amplitude(signal, sr, freq, window):
    """Windowed complex demodulation -> (amplitude, phase_deg)."""
    if signal.size == 0:
        return 0.0, 0.0
    n = np.arange(signal.size)
    mixed = signal * window * np.exp(-2j * np.pi * freq * n / sr)
    value = np.sum(mixed)
    norm = np.sum(window)
    amplitude = 2.0 * abs(value) / norm
    phase = np.degrees(np.angle(value))
    return amplitude, phase


def band_power_db(signal, sr, f_lo, f_hi, window):
    """Mean-square power (dBFS) of a band via a single windowed FFT."""
    if signal.size < 16:
        return -200.0
    windowed = signal * window
    spectrum = np.fft.rfft(windowed)
    freqs = np.fft.rfftfreq(signal.size, 1.0 / sr)
    psd = (np.abs(spectrum) ** 2) / (sr * np.sum(window ** 2))
    mask = (freqs >= f_lo) & (freqs <= f_hi)
    df = freqs[1] - freqs[0] if freqs.size > 1 else 1.0
    power = float(np.sum(psd[mask]) * df)
    return 10.0 * np.log10(max(power, 1e-30))


def estimate_delay(reference, observed, max_delay):
    ref = to_mono(reference)
    obs = to_mono(observed)
    n = len(ref) + len(obs)
    size = 1 << (n - 1).bit_length()
    cross = np.fft.irfft(
        np.fft.rfft(obs, size) * np.conj(np.fft.rfft(ref, size)), size
    )
    max_delay = int(min(max_delay, len(ref) - 1, len(obs) - 1))
    lags = np.arange(-max_delay, max_delay + 1)
    values = cross[lags % size]
    best = int(np.argmax(np.abs(values)))
    return int(lags[best]), float(values[best] / max(np.sum(ref ** 2), 1e-30))


def cmd_stats(args):
    samples, sr = load(args.wav)
    finite = np.isfinite(samples)
    finite_values = samples[finite]
    emit("samples", samples.shape[0])
    emit("channels", samples.shape[1])
    emit("sample_rate", sr, 1)
    emit("nonfinite", int(np.count_nonzero(~finite)))
    emit("min", float(finite_values.min()) if finite_values.size else 0.0)
    emit("max", float(finite_values.max()) if finite_values.size else 0.0)
    emit("max_abs", float(np.max(np.abs(finite_values))) if finite_values.size else 0.0)
    emit("rms_db", rms_db(finite_values) if finite_values.size else -240.0, 3)
    emit("dc", float(np.mean(finite_values)) if finite_values.size else 0.0, 9)
    return 0


def cmd_peak(args):
    samples, _ = load(args.wav)
    emit("max_abs", float(np.max(np.abs(samples))) if samples.size else 0.0, 9)
    return 0


def cmd_delay(args):
    reference, _ = load(args.reference)
    observed, _ = load(args.observed)
    delay, corr = estimate_delay(reference, observed, args.max)
    emit("delay", delay)
    emit("correlation", corr, 6)
    return 0


def cmd_unity(args):
    reference, sr = load(args.reference)
    observed, _ = load(args.observed)
    latency = int(args.latency)
    usable = min(len(reference), len(observed)) - latency
    if usable <= 0:
        emit("usable", 0)
        return 1

    errors = []
    for channel in range(min(reference.shape[1], observed.shape[1])):
        expected = reference[:usable, channel]
        actual = observed[latency:latency + usable, channel]
        errors.append(actual - expected)

    error = np.stack(errors, axis=1)
    reference_slice = reference[:usable, :error.shape[1]]
    error_rms = float(np.sqrt(np.mean(error ** 2)))
    signal_rms = float(np.sqrt(np.mean(reference_slice ** 2)))
    emit("usable", usable)
    emit("signal_rms_db", 20.0 * np.log10(max(signal_rms, 1e-15)), 3)
    emit("error_rms_db", 20.0 * np.log10(max(error_rms, 1e-15)), 3)
    ratio = error_rms / max(signal_rms, 1e-15)
    emit("error_rel_db", 20.0 * np.log10(max(ratio, 1e-15)), 3)
    emit("error_max_abs", float(np.max(np.abs(error))) if error.size else 0.0, 9)
    emit("out_dc", float(np.mean(observed)), 9)
    emit("out_max_abs", float(np.max(np.abs(observed))) if observed.size else 0.0, 6)
    return 0


def cmd_response(args):
    """Magnitude-response comparison (allpass/phase change is ignored)."""
    reference, ref_sr = load(args.reference)
    observed, _ = load(args.observed)
    sr = args.sr if args.sr > 0 else ref_sr
    a = segment(reference, sr, args.start, args.dur)
    b = segment(observed, sr, args.start, args.dur)
    x = to_mono(a)
    y = to_mono(b)
    n = min(len(x), len(y))
    if n < 64:
        emit("bins", 0)
        return 1
    x = x[:n]
    y = y[:n]
    window = hann(n)
    ref_db = 20.0 * np.log10(np.abs(np.fft.rfft(x * window)) + 1e-12)
    out_db = 20.0 * np.log10(np.abs(np.fft.rfft(y * window)) + 1e-12)
    freqs = np.fft.rfftfreq(n, 1.0 / sr)
    peak = np.max(ref_db)
    mask = (freqs >= args.fmin) & (freqs <= args.fmax) & (ref_db > peak - 80.0)
    deviation = out_db[mask] - ref_db[mask]
    emit("bins", int(np.count_nonzero(mask)))
    if deviation.size == 0:
        emit("max_abs_dev_db", 0.0)
        emit("mean_dev_db", 0.0)
        return 1
    emit("max_abs_dev_db", float(np.max(np.abs(deviation))), 3)
    emit("mean_dev_db", float(np.mean(deviation)), 3)
    emit("p95_abs_dev_db", float(np.percentile(np.abs(deviation), 95)), 3)

    # Probe frequencies via complex demodulation (phase independent).
    for text in args.probe.split(","):
        freq = float(text.strip())
        amp_ref, _ = tone_amplitude(x, sr, freq, window)
        amp_out, _ = tone_amplitude(y, sr, freq, window)
        key = "probe_%g_db" % freq
        emit(key, 20.0 * np.log10(max(amp_out, 1e-15) / max(amp_ref, 1e-15)), 3)

    # Time-domain residual (informational: zero-latency IIR designs are allpass).
    error_rms = float(np.sqrt(np.mean((y - x) ** 2)))
    signal_rms = float(np.sqrt(np.mean(x ** 2)))
    emit("time_error_rel_db", 20.0 * np.log10(max(error_rms, 1e-15) / max(signal_rms, 1e-15)), 3)
    return 0


def cmd_tone(args):
    samples, file_sr = load(args.wav)
    sr = args.sr if args.sr > 0 else file_sr
    seg = segment(samples, sr, args.start, args.dur)
    signal = channel_signal(seg, args.channel)
    window = hann(signal.size)
    emit("window_samples", signal.size)
    emit("window_rms_db", rms_db(signal) if signal.size else -240.0, 3)
    for text in args.freqs.split(","):
        freq = float(text.strip())
        amplitude, phase = tone_amplitude(signal, sr, freq, window)
        key = "tone_%g" % freq
        emit(key + "_db", 20.0 * np.log10(max(amplitude, 1e-15)), 3)
        emit(key + "_phase_deg", phase, 2)
    return 0


def cmd_bands(args):
    samples, file_sr = load(args.wav)
    sr = args.sr if args.sr > 0 else file_sr
    seg = segment(samples, sr, args.start, args.dur)
    signal = channel_signal(seg, args.channel)
    window = hann(signal.size)
    emit("window_samples", signal.size)
    emit("window_rms_db", rms_db(signal) if signal.size else -240.0, 3)
    for spec in args.bands.split(","):
        f_lo, f_hi = (float(part) for part in spec.split(":"))
        key = "band_%g_%g_db" % (f_lo, f_hi)
        emit(key, band_power_db(signal, sr, f_lo, f_hi, window), 3)
    return 0


def cmd_channels(args):
    samples, file_sr = load(args.wav)
    sr = args.sr if args.sr > 0 else file_sr
    seg = segment(samples, sr, args.start, args.dur)
    window = hann(seg.shape[0])
    for channel in range(seg.shape[1]):
        signal = seg[:, channel]
        emit("ch%d_rms_db" % channel, rms_db(signal) if signal.size else -240.0, 3)
        if args.freq > 0:
            amplitude, phase = tone_amplitude(signal, sr, args.freq, window)
            emit("ch%d_tone_db" % channel, 20.0 * np.log10(max(amplitude, 1e-15)), 3)
            emit("ch%d_tone_phase_deg" % channel, phase, 2)
    return 0


def cmd_png(args):
    with open(args.png, "rb") as f:
        header = f.read(33)
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        emit("valid", 0)
        return 1
    width, height = struct.unpack(">II", header[16:24])
    emit("valid", 1)
    emit("width", width)
    emit("height", height)

    # Optional image content statistics (needs Pillow; skipped when absent).
    try:
        from PIL import Image
        import numpy as np

        image = np.asarray(Image.open(args.png).convert("RGB"))
        colours = np.unique(image.reshape(-1, 3), axis=0)
        emit("unique_colours", int(len(colours)))
        emit("darkest", int(image.min()))
        emit("brightest", int(image.max()))
    except Exception:
        pass

    return 0


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stats")
    p.add_argument("wav")
    p.set_defaults(func=cmd_stats)

    p = sub.add_parser("peak")
    p.add_argument("wav")
    p.set_defaults(func=cmd_peak)

    p = sub.add_parser("delay")
    p.add_argument("reference")
    p.add_argument("observed")
    p.add_argument("--max", type=int, default=8192)
    p.set_defaults(func=cmd_delay)

    p = sub.add_parser("unity")
    p.add_argument("reference")
    p.add_argument("observed")
    p.add_argument("latency", type=int)
    p.set_defaults(func=cmd_unity)

    p = sub.add_parser("response")
    p.add_argument("reference")
    p.add_argument("observed")
    p.add_argument("--sr", type=float, default=0.0)
    p.add_argument("--start", type=float, default=0.0)
    p.add_argument("--dur", type=float, default=1.0)
    p.add_argument("--fmin", type=float, default=30.0)
    p.add_argument("--fmax", type=float, default=18000.0)
    p.add_argument("--probe", default="100,1000,5000,10000")
    p.set_defaults(func=cmd_response)

    p = sub.add_parser("tone")
    p.add_argument("wav")
    p.add_argument("--sr", type=float, default=0.0)
    p.add_argument("--freqs", default="1000")
    p.add_argument("--start", type=float, default=0.0)
    p.add_argument("--dur", type=float, default=1.0)
    p.add_argument("--channel", default="mix")
    p.set_defaults(func=cmd_tone)

    p = sub.add_parser("bands")
    p.add_argument("wav")
    p.add_argument("--sr", type=float, default=0.0)
    p.add_argument("--start", type=float, default=0.0)
    p.add_argument("--dur", type=float, default=1.0)
    p.add_argument("--channel", default="mix")
    p.add_argument("--bands", default="50:200,8000:15000")
    p.set_defaults(func=cmd_bands)

    p = sub.add_parser("channels")
    p.add_argument("wav")
    p.add_argument("--sr", type=float, default=0.0)
    p.add_argument("--start", type=float, default=0.0)
    p.add_argument("--dur", type=float, default=1.0)
    p.add_argument("--freq", type=float, default=0.0)
    p.set_defaults(func=cmd_channels)

    p = sub.add_parser("png")
    p.add_argument("png")
    p.set_defaults(func=cmd_png)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
