#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ts
{
/** Parameter identifiers and ranges, matched to the reference plug-in so that
    parameter IDs, names, order, ranges and defaults are interchangeable.

    Reference VST3 parameter IDs are the JUCE 32-bit string hashes of the
    uppercase IDs below, which is why the strings look terse.
*/
namespace params
{
inline constexpr const char* amount      = "RATIO";       // host name "Ratio", UI "Amount"
inline constexpr const char* lowCut      = "LC";
inline constexpr const char* highCut     = "HC";
inline constexpr const char* freeze      = "FREEZE";
inline constexpr const char* pan         = "PAN";
inline constexpr const char* mode        = "MODE";
inline constexpr const char* attack      = "AT";
inline constexpr const char* release     = "RL";
inline constexpr const char* scAudition  = "SIDECHAIN";

// 32 one-third-octave bands, 20 Hz .. 20 kHz.
inline constexpr int numBands = 32;

inline constexpr float amountMin      = 0.0f;
inline constexpr float amountMax      = 100.0f;
inline constexpr float amountDefault  = 15.0f;

inline constexpr float cutMin         = 20.0f;
inline constexpr float cutMax         = 20000.0f;
inline constexpr float lowCutDefault  = 20.0f;
inline constexpr float highCutDefault = 20000.0f;

inline constexpr float attackMin      = 0.01f;
inline constexpr float attackMax      = 100.0f;
inline constexpr float attackDefault  = 5.0f;

inline constexpr float releaseMin     = 1.0f;
inline constexpr float releaseMax     = 700.0f;
inline constexpr float releaseDefault = 30.0f;

inline constexpr float panMin         = 0.0f;
inline constexpr float panMax         = 100.0f;
inline constexpr float panDefault     = 50.0f;

// Largest attenuation applied to one band at Amount = 100 %.
inline constexpr float maxReductionDb = -33.0f;
} // namespace params

/** Snapshot of what the spectrum display should draw. */
struct DisplayFrame
{
    static constexpr int numBands = params::numBands;

    float bandFrequenciesHz[numBands] {};
    float sidechainDb[numBands] {};      // filtered sidechain spectrum, dBFS
    float reductionDb[numBands] {};      // resulting EQ curve at band centres, dB
    float lowCutHz = params::cutMin;
    float highCutHz = params::cutMax;
    float hoverFrequencyHz = 0.0f;
    float hoverDb = 0.0f;
};

} // namespace ts
