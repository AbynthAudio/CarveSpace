#include "BandAnalyzer.h"

#include <cmath>

namespace ts
{
BandAnalyzer::BandAnalyzer()
    : fftBuffer ((size_t) fftSize * 2, 0.0f)
{
    rebuildBinEdges (sr);
}

float BandAnalyzer::bandFrequency (int band) noexcept
{
    // 32 bands covering 20 Hz .. 20 kHz (log spaced).
    constexpr float low = 20.0f;
    constexpr float high = 20000.0f;
    const float ratio = std::pow (high / low, 1.0f / (float) (numBands - 1));
    return low * std::pow (ratio, (float) band);
}

void BandAnalyzer::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    rebuildBinEdges (sr);
    reset();
}

void BandAnalyzer::reset()
{
    std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
}

void BandAnalyzer::rebuildBinEdges (double sampleRate)
{
    // Geomectric midpoints between adjacent band centres define band edges.
    const float ratio = std::pow (20000.0f / 20.0f, 1.0f / (float) (numBands - 1));
    const float halfRatio = std::sqrt (ratio);

    for (int i = 0; i <= numBands; ++i)
    {
        const float centre = 20.0f * std::pow (ratio, (float) i);
        const float lower = (i == 0) ? 20.0f : centre / halfRatio;
        const float upper = (i == numBands) ? 20000.0f : centre * halfRatio;
        juce::ignoreUnused (upper);

        int bin = (int) std::lround (lower * (double) fftSize / sampleRate);
        bin = juce::jlimit (1, fftSize / 2, bin);
        binEdges[(size_t) i] = bin;
    }

    // Guarantee that every band owns at least one bin at low sample rates.
    for (int i = 0; i < numBands; ++i)
        if (binEdges[(size_t) (i + 1)] <= binEdges[(size_t) i])
            binEdges[(size_t) (i + 1)] = std::min (binEdges[(size_t) i] + 1, fftSize / 2);

    // Hann window has coherent gain 0.5, so a full-scale sine gives a peak bin
    // magnitude of amplitude * fftSize * 0.5 / 2. Scaling by 4 / fftSize
    // converts that back to the amplitude of the sine.
    amplitudeScale = 4.0f / (float) fftSize;
}

void BandAnalyzer::analyze (const float* mono, float* bandDbOut) noexcept
{
    std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::copy (mono, mono + fftSize, fftBuffer.begin());

    window.multiplyWithWindowingTable (fftBuffer.data(), (size_t) fftSize);
    fft.performRealOnlyForwardTransform (fftBuffer.data(), true);

    for (int band = 0; band < numBands; ++band)
    {
        const int startBin = binEdges[(size_t) band];
        const int endBin = binEdges[(size_t) (band + 1)];
        const int count = std::max (1, endBin - startBin);

        float power = 0.0f;

        for (int bin = startBin; bin < endBin; ++bin)
        {
            const float re = fftBuffer[(size_t) bin * 2];
            const float im = fftBuffer[(size_t) bin * 2 + 1];
            power += re * re + im * im;
        }

        const float amplitude = std::sqrt (power / (float) count) * amplitudeScale;
        bandDbOut[band] = juce::Decibels::gainToDecibels (amplitude, -100.0f);
    }
}

} // namespace ts
