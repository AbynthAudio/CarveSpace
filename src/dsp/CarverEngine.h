#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

#include "Parameters.h"

namespace ts
{
struct CarverSettings
{
    float amountPercent = 15.0f;  // 0 .. 100
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float attackMs = 5.0f;
    float releaseMs = 30.0f;
    float pan = 50.0f;            // 0 = left/mid, 50 = centre, 100 = right/side
    int mode = 0;                 // 0 = L/R, 1 = M/S
    bool freeze = false;
    bool scAudition = false;
};

/** Zero-latency 32-band spectral carver.

    The main signal runs through a cascade of 32 peaking filters (one per
    one-third-octave band).  A peaking filter is exactly transparent at 0 dB,
    so with no reduction the processor is bit-transparent and adds no latency.

    The sidechain is high/low-cut filtered and analysed by 32 deliberately wide
    band-pass filters (Q ~ 0.7).  Peak envelope followers with attack/release
    ballistics drive the measured reference control law:

        depth = 33.1 / (1 + exp(-(L - T) / 3.75))
        T     = -43.5 - 42 * log10(amount)

    Each band's gain is `-depth * bandScale[band]`, where bandScale is a
    pre-computed normalisation that makes the cascade's summed dB curve match
    the target curve (adjacent peaking bands overlap, so applying the raw depth
    to every band would over-attenuate).  Gains are updated at control rate and
    smoothed per sample.
*/
class CarverEngine
{
public:
    static constexpr int numBands = params::numBands;
    static constexpr int latencySamples = 0;
    static constexpr float maxReductionDb = params::maxReductionDb;

    CarverEngine();
    ~CarverEngine() = default;

    void prepare (double sampleRate);
    void reset();

    void process (juce::AudioBuffer<float>& main,
                  const juce::AudioBuffer<float>& sidechain,
                  int numSamples,
                  const CarverSettings& settings);

    int getLatencySamples() const noexcept { return latencySamples; }

    void getDisplayFrame (DisplayFrame& destination) const;

private:
    static constexpr int controlInterval = 16;
    static constexpr int coefficientInterval = 4;

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        inline float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        void reset() noexcept { z1 = z2 = 0.0f; }

        static Biquad bandPass (double sampleRate, float frequency, float q) noexcept;
        static Biquad firstOrderHighPass (double sampleRate, float frequency) noexcept;
        static Biquad secondOrderLowPassButterworth (double sampleRate, float frequency) noexcept;
        static Biquad peaking (double sampleRate, float frequency, float q, float gainDb) noexcept;

        static float peakingResponseDb (double sampleRate, float frequency, float q,
                                        float gainDb, float atFrequency) noexcept;
    };

    void updateCutFilters (const CarverSettings& settings);
    void updateControl (const CarverSettings& settings);
    void updateCoefficients();
    void updateDisplay (const CarverSettings& settings);

    static float depthDbForLevel (float levelDb, float amountNorm) noexcept;
    static float bandCentreFrequency (int band) noexcept;

    double sr = 48000.0;

    Biquad analysisBands[numBands];
    Biquad scHighPass[2];
    Biquad scLowPass[2];
    float currentLowCut = -1.0f;
    float currentHighCut = -1.0f;
    std::array<float, numBands> envelope {};

    Biquad peakingBands[2][numBands];
    std::array<float, numBands> bandScale {};

    std::array<float, numBands> gainDbCurrent[2];
    std::array<float, numBands> gainDbTarget[2];

    std::array<float, numBands> bandN {};
    std::array<float, numBands> bandDisplayDb {};

    int displayCounter = 0;
    int samplesUntilControl = 0;
    int samplesUntilCoefficients = 0;

    mutable juce::SpinLock displayLock;
    DisplayFrame displayFrame;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarverEngine)
};

} // namespace ts
