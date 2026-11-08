"""Minimal WAV reader/writer for the Trackspacer validation harness.

Supports the formats the offline host is expected to read/write:
  - PCM  8/16/24/32-bit integer
  - IEEE float 32/64-bit
  - WAVE_FORMAT_EXTENSIBLE (0xFFFE) wrapping the above

Only numpy is required (no scipy / soundfile dependency).
"""

import struct
import numpy as np

WAVE_FORMAT_PCM = 1
WAVE_FORMAT_IEEE_FLOAT = 3
WAVE_FORMAT_EXTENSIBLE = 0xFFFE


def _parse_fmt(fmt):
    audio_format, channels, rate, _byte_rate, _block_align, bits = struct.unpack(
        "<HHIIHH", fmt[:16]
    )
    if audio_format == WAVE_FORMAT_EXTENSIBLE:
        # cbSize(2) validBits(2) channelMask(4) subFormat GUID(16)
        audio_format = struct.unpack("<H", fmt[24:26])[0]
    return audio_format, channels, rate, bits


def read_wav(path):
    """Return (samples float64 ndarray [frames, channels], sample_rate)."""
    with open(path, "rb") as f:
        data = f.read()

    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a RIFF/WAVE file: %s" % path)

    fmt = None
    payload = None
    pos = 12
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos + 4]
        size = struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if chunk_id == b"fmt ":
            fmt = body
        elif chunk_id == b"data":
            payload = body
        pos += 8 + size + (size & 1)

    if fmt is None or payload is None:
        raise ValueError("missing fmt/data chunk: %s" % path)

    audio_format, channels, rate, bits = _parse_fmt(fmt)
    channels = max(1, channels)

    if audio_format == WAVE_FORMAT_IEEE_FLOAT:
        dtype = {32: "<f4", 64: "<f8"}[bits]
        samples = np.frombuffer(payload, dtype=dtype).astype(np.float64)
    elif audio_format == WAVE_FORMAT_PCM:
        if bits == 8:
            samples = (np.frombuffer(payload, dtype=np.uint8).astype(np.float64) - 128.0) / 128.0
        elif bits == 16:
            samples = np.frombuffer(payload, dtype="<i2").astype(np.float64) / 32768.0
        elif bits == 24:
            raw = np.frombuffer(payload, dtype=np.uint8).reshape(-1, 3)
            ints = (
                raw[:, 0].astype(np.int32)
                | (raw[:, 1].astype(np.int32) << 8)
                | (raw[:, 2].astype(np.int32) << 16)
            )
            ints = np.where(ints >= (1 << 23), ints - (1 << 24), ints)
            samples = ints.astype(np.float64) / float(1 << 23)
        elif bits == 32:
            samples = np.frombuffer(payload, dtype="<i4").astype(np.float64) / float(1 << 31)
        else:
            raise ValueError("unsupported PCM bit depth %d" % bits)
    else:
        raise ValueError("unsupported WAV format tag %d" % audio_format)

    samples = samples[: (len(samples) // channels) * channels]
    return samples.reshape(-1, channels), float(rate)


def write_wav_float32(path, samples, sample_rate):
    """Write a 32-bit IEEE-float stereo/multichannel WAV (format tag 3)."""
    samples = np.asarray(samples, dtype=np.float32)
    if samples.ndim == 1:
        samples = samples[:, None]
    channels = samples.shape[1]
    interleaved = np.ascontiguousarray(samples, dtype="<f4").tobytes()
    byte_rate = sample_rate * channels * 4
    block_align = channels * 4
    fmt = struct.pack("<HHIIHH", WAVE_FORMAT_IEEE_FLOAT, channels, sample_rate,
                      byte_rate, block_align, 32)
    fact = struct.pack("<I", 0)
    chunks = [
        b"fmt " + struct.pack("<I", len(fmt)) + fmt,
        b"fact" + struct.pack("<I", len(fact)) + fact,
        b"data" + struct.pack("<I", len(interleaved)) + interleaved,
    ]
    body = b"WAVE" + b"".join(chunks)
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", len(body)) + body)


def rms_db(signal):
    signal = np.asarray(signal, dtype=np.float64)
    rms = float(np.sqrt(np.mean(signal ** 2))) if signal.size else 0.0
    return 20.0 * np.log10(max(rms, 1e-15))


def to_mono(signal):
    signal = np.asarray(signal, dtype=np.float64)
    if signal.ndim == 1:
        return signal
    return signal.mean(axis=1)
