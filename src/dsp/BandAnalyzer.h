#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

#include "Parameters.h"

namespace ts
{
/** FFT analysis of one signal into 32 logarithmically spaced bands.

    The band levels are calibrated so that a full-scale sine in one band reads
    0 dBFS. Levels are the RMS magnitude of the FFT bins falling inside each
    band, which makes the readout independent of the band width.
*/
class BandAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBands = params::numBands;

    BandAnalyzer();

    void prepare (double sampleRate);
    void reset();

    /** Analyses fftSize mono samples (no windowing needed, done internally). */
    void analyze (const float* mono, float* bandDbOut) noexcept;

    /** Centre frequency of a band, Hz. */
    static float bandFrequency (int band) noexcept;

private:
    void rebuildBinEdges (double sampleRate);

    double sr = 48000.0;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
                                                  juce::dsp::WindowingFunction<float>::hann,
                                                  false };
    std::vector<float> fftBuffer; // 2 * fftSize floats
    std::array<int, numBands + 1> binEdges {};
    float amplitudeScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandAnalyzer)
};

} // namespace ts
