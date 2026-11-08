#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ts
{
/** Parameter identifiers shared by the processor and the editor.

    Names/ranges mirror Wavesfactory Trackspacer 2.5:
      - Amount        0 .. 100 %   (depth of the automatic EQ reduction)
      - Low-cut       20 .. 20k Hz (lower limit of the processed range)
      - High-cut      20 .. 20k Hz (upper limit of the processed range)
      - Attack        ms           (reduction onset ballistics)
      - Release       ms           (reduction recovery ballistics)
      - Pan           -100 .. 100  (pan of the effect: L<->R or M<->S)
      - Mode          L/R | M/S
      - Freeze        hold the current reduction curve
      - ScAudition    listen to the (filtered) sidechain signal
*/
namespace params
{
inline constexpr const char* amount      = "amount";
inline constexpr const char* lowCut      = "lowcut";
inline constexpr const char* highCut     = "highcut";
inline constexpr const char* freeze      = "freeze";
inline constexpr const char* attack      = "attack";
inline constexpr const char* release     = "release";
inline constexpr const char* pan         = "pan";
inline constexpr const char* mode        = "mode";
inline constexpr const char* scAudition  = "scaudition";

// 32 one-third-octave bands, 20 Hz .. 20 kHz (as used by the spectral engine).
inline constexpr int numBands = 32;

// Provisional ranges/defaults; these are tuned against the reference plugin.
inline constexpr float amountMin      = 0.0f;
inline constexpr float amountMax      = 100.0f;
inline constexpr float amountDefault  = 0.0f;

inline constexpr float cutMin         = 20.0f;
inline constexpr float cutMax         = 20000.0f;
inline constexpr float lowCutDefault  = 20.0f;
inline constexpr float highCutDefault = 20000.0f;

inline constexpr float attackMin      = 0.1f;
inline constexpr float attackMax      = 100.0f;
inline constexpr float attackDefault  = 10.0f;

inline constexpr float releaseMin     = 1.0f;
inline constexpr float releaseMax     = 1000.0f;
inline constexpr float releaseDefault = 100.0f;

inline constexpr float panMin         = -100.0f;
inline constexpr float panMax         = 100.0f;
inline constexpr float panDefault     = 0.0f;

// Largest attenuation applied to a band at Amount = 100 %.
inline constexpr float maxReductionDb = -30.0f;
} // namespace params

/** Snapshot of what the spectrum display should draw.

    Written by the audio thread (or the engine's render loop) and read by the
    editor through a lock-free triple buffer. Values are in dB.
*/
struct DisplayFrame
{
    static constexpr int numBands = params::numBands;

    /** Absolute frequency of the centre of each band, Hz. */
    float bandFrequenciesHz[numBands] {};

    /** Sidechain spectrum, dBFS-ish (display range roughly -72 .. 0 dB). */
    float sidechainDb[numBands] {};

    /** Applied reduction as a negative gain in dB (0 .. maxReductionDb). */
    float reductionDb[numBands] {};

    /** Low/high cut positions for the graphical feedback, Hz. */
    float lowCutHz = params::cutMin;
    float highCutHz = params::cutMax;

    /** Mouse hover readout, set by the display itself. */
    float hoverFrequencyHz = 0.0f;
    float hoverDb = 0.0f;
};

} // namespace ts
