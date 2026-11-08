#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <complex>
#include <vector>

#include "BandAnalyzer.h"
#include "Parameters.h"

namespace ts
{
struct CarverSettings
{
    float amountPercent = 0.0f;   // 0 .. 100
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float pan = 0.0f;             // -100 .. 100
    int mode = 0;                 // 0 = L/R, 1 = M/S
    bool freeze = false;
    bool scAudition = false;
};

/** The spectral "carver": analyses the sidechain and applies an inverse EQ
    curve to the main signal with per-band attack/release ballistics.

    Implementation is an STFT overlap-add engine:
      - FFT size 2048, 8x overlap (hop 256), Hann analysis + synthesis window.
      - Sidechain is high/low-cut filtered, then analysed into 32 log bands.
      - Band levels drive per-band reduction gains (negative dB) that are
        smoothed with attack/release.
      - The gain curve is interpolated across FFT bins and applied to the main
        signal.  L/R and M/S processing are both supported, with the effect
        panned between channels.

    Reported latency is exactly fftSize samples.
*/
class CarverEngine
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 8;
    static constexpr int numBands = params::numBands;
    static constexpr int latencySamples = fftSize;

    // Largest attenuation (dB) reached at Amount = 100 % for a full-scale band.
    static constexpr float maxReductionDb = params::maxReductionDb;

    CarverEngine();
    ~CarverEngine() = default;

    void prepare (double sampleRate);
    void reset();

    /** Processes main in place. `sidechain` may be empty (internal sidechain). */
    void process (juce::AudioBuffer<float>& main,
                  const juce::AudioBuffer<float>& sidechain,
                  int numSamples,
                  const CarverSettings& settings);

    int getLatencySamples() const noexcept { return latencySamples; }

    /** Fills a thread-safe snapshot for the UI. */
    void getDisplayFrame (DisplayFrame& destination) const;

private:
    using Complex = std::complex<float>;

    void updateFilters (const CarverSettings& settings);
    void processHop (const CarverSettings& settings);
    void renderHop();
    float bandGainDbForFrequency (float frequencyHz) const noexcept;

    double sr = 48000.0;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
                                                  juce::dsp::WindowingFunction<float>::hann,
                                                  false };

    BandAnalyzer analyzer;
    std::vector<float> analysisBuffer;          // 2 * fftSize scratch
    std::vector<float> msScratch;               // 2 * fftSize, M/S accumulation
    std::vector<float> synthWindow;             // fftSize, window * overlap norm
    std::vector<Complex> spectrumIn, spectrumOut;

    // Input rings (main L/R + filtered sidechain L/R) of length fftSize.
    std::vector<float> inL, inR, scL, scR;
    // Overlap-add accumulation rings of length fftSize.
    std::vector<float> accL, accR;

    std::vector<float> outFifoL, outFifoR, outFifoScl, outFifoScr;
    int outFifoPos = 0;

    int inWrite = 0;
    int totalSamples = 0;

    // Cached per-block settings used by the hop renderer.
    int cachedMode = 0;
    float cachedPan = 0.0f;

    // Filters applied to the sidechain analysis signal.
    juce::dsp::IIR::Filter<float> hpfL, hpfR, lpfL, lpfR;
    float currentLowCut = -1.0f;
    float currentHighCut = -1.0f;

    std::array<float, numBands> sidechainBandDb {};
    std::array<float, numBands> targetReductionDb {};
    std::array<float, numBands> currentReductionDb {};
    std::array<float, numBands> displayBandFrequencies {};

    // Display snapshot (audio thread writes, message thread reads).
    mutable juce::SpinLock displayLock;
    DisplayFrame displayFrame;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarverEngine)
};

} // namespace ts
