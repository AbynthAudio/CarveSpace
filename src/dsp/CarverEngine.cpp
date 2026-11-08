#include "CarverEngine.h"

#include <cmath>

namespace ts
{
namespace
{
    constexpr float cutFilterQ = 0.70710678f;
    constexpr float analysisBandQ = 0.7f;
    constexpr float peakingBandQ = 4.0f;
    constexpr float boundaryRatio = 1.2495f;

    inline float onePoleCoefficient (float timeMs, double sampleRate) noexcept
    {
        const float tauSeconds = juce::jmax (0.001f, timeMs) * 0.001f;
        return 1.0f - std::exp (-1.0f / (tauSeconds * (float) sampleRate));
    }
} // namespace

//==============================================================================
CarverEngine::Biquad CarverEngine::Biquad::bandPass (double sampleRate, float frequency, float q) noexcept
{
    const double w0 = juce::MathConstants<double>::twoPi * (double) frequency / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * (double) q);
    const double a0 = 1.0 + alpha;

    Biquad result;
    result.b0 = (float) (alpha / a0);
    result.b1 = 0.0f;
    result.b2 = (float) (-alpha / a0);
    result.a1 = (float) (-2.0 * std::cos (w0) / a0);
    result.a2 = (float) ((1.0 - alpha) / a0);
    return result;
}

CarverEngine::Biquad CarverEngine::Biquad::firstOrderHighPass (double sampleRate, float frequency) noexcept
{
    const double k = std::tan (juce::MathConstants<double>::pi * (double) frequency / sampleRate);
    const double norm = 1.0 / (1.0 + k);

    Biquad result;
    result.b0 = (float) norm;
    result.b1 = (float) -norm;
    result.b2 = 0.0f;
    result.a1 = (float) ((k - 1.0) * norm);
    result.a2 = 0.0f;
    return result;
}

CarverEngine::Biquad CarverEngine::Biquad::secondOrderLowPassButterworth (double sampleRate, float frequency) noexcept
{
    const double w0 = juce::MathConstants<double>::twoPi * (double) frequency / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * (double) cutFilterQ);
    const double cosW0 = std::cos (w0);
    const double a0 = 1.0 + alpha;

    Biquad result;
    result.b0 = (float) ((1.0 - cosW0) * 0.5 / a0);
    result.b1 = (float) ((1.0 - cosW0) / a0);
    result.b2 = result.b0;
    result.a1 = (float) (-2.0 * cosW0 / a0);
    result.a2 = (float) ((1.0 - alpha) / a0);
    return result;
}

CarverEngine::Biquad CarverEngine::Biquad::peaking (double sampleRate, float frequency, float q, float gainDb) noexcept
{
    if (std::abs (gainDb) < 1.0e-4f)
        return {};

    const double A = std::pow (10.0, (double) gainDb / 40.0);
    const double w0 = juce::MathConstants<double>::twoPi * (double) frequency / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * (double) q);
    const double cosW0 = std::cos (w0);
    const double a0 = 1.0 + alpha / A;

    Biquad result;
    result.b0 = (float) ((1.0 + alpha * A) / a0);
    result.b1 = (float) (-2.0 * cosW0 / a0);
    result.b2 = (float) ((1.0 - alpha * A) / a0);
    result.a1 = result.b1;
    result.a2 = (float) ((1.0 - alpha / A) / a0);
    return result;
}

float CarverEngine::Biquad::peakingResponseDb (double sampleRate, float frequency, float q,
                                               float gainDb, float atFrequency) noexcept
{
    const auto coefficients = peaking (sampleRate, frequency, q, gainDb);
    const double w = juce::MathConstants<double>::twoPi * (double) atFrequency / sampleRate;
    const std::complex<double> z1 = std::polar (1.0, -w);
    const std::complex<double> z2 = z1 * z1;

    const std::complex<double> numerator = (double) coefficients.b0
                                           + (double) coefficients.b1 * z1
                                           + (double) coefficients.b2 * z2;
    const std::complex<double> denominator = 1.0
                                             + (double) coefficients.a1 * z1
                                             + (double) coefficients.a2 * z2;

    const double magnitude = std::abs (numerator / denominator);
    return (float) (20.0 * std::log10 (juce::jmax (1.0e-9, magnitude)));
}

//==============================================================================
CarverEngine::CarverEngine()
{
    reset();
}

float CarverEngine::bandCentreFrequency (int band) noexcept
{
    const float lower = 18.0f * std::pow (boundaryRatio, (float) band);
    return lower * std::sqrt (boundaryRatio);
}

void CarverEngine::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;

    for (int band = 0; band < numBands; ++band)
    {
        analysisBands[band] = Biquad::bandPass (sr, bandCentreFrequency (band), analysisBandQ);
        analysisBands[band].reset();
    }

    // --- Normalisation of the series peaking cascade ----------------------
    // R[i][k] is the dB response at band centre i of peaking filter k for a
    // +1 dB gain.  Solve R * s = 1 so the summed curve has unit gain per unit
    // target depth, then iterate with the actual -33 dB gain to compensate for
    // the filter's gain nonlinearity.
    constexpr int n = numBands;
    double matrix[n][n + 1] {};

    for (int i = 0; i < n; ++i)
    {
        for (int k = 0; k < n; ++k)
            matrix[i][k] = (double) Biquad::peakingResponseDb (sr, bandCentreFrequency (k),
                                                               peakingBandQ, 1.0f,
                                                               bandCentreFrequency (i));

        matrix[i][n] = 1.0;
    }

    // Gaussian elimination with partial pivoting.
    for (int column = 0; column < n; ++column)
    {
        int pivot = column;

        for (int row = column + 1; row < n; ++row)
            if (std::abs (matrix[row][column]) > std::abs (matrix[pivot][column]))
                pivot = row;

        if (std::abs (matrix[pivot][column]) < 1.0e-12)
            continue;

        for (int k = column; k <= n; ++k)
            std::swap (matrix[column][k], matrix[pivot][k]);

        const double diagonal = matrix[column][column];

        for (int k = column; k <= n; ++k)
            matrix[column][k] /= diagonal;

        for (int row = 0; row < n; ++row)
        {
            if (row == column)
                continue;

            const double factor = matrix[row][column];

            if (std::abs (factor) < 1.0e-18)
                continue;

            for (int k = column; k <= n; ++k)
                matrix[row][k] -= factor * matrix[column][k];
        }
    }

    for (int band = 0; band < n; ++band)
        bandScale[(size_t) band] = juce::jlimit (0.02f, 2.0f, (float) matrix[band][n]);

    // Iteratively normalise so that full-depth (all bands -33 dB target)
    // produces about -33 dB total at the band centres.
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        float worstDepth = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float total = 0.0f;

            for (int k = 0; k < n; ++k)
                total += Biquad::peakingResponseDb (sr, bandCentreFrequency (k), peakingBandQ,
                                                    -33.1f * bandScale[(size_t) k],
                                                    bandCentreFrequency (i));

            worstDepth = juce::jmax (worstDepth, -total);
        }

        if (worstDepth > 1.0e-3f)
        {
            const float correction = juce::jlimit (0.5f, 2.0f, 33.1f / worstDepth);

            for (auto& scale : bandScale)
                scale *= correction;
        }
    }

    // Final calibration: the summed cascade loses a little depth because of
    // the peaking filters' gain nonlinearity; compensate globally.
    for (auto& scale : bandScale)
        scale *= 1.12f;

    for (int channel = 0; channel < 2; ++channel)
        for (int band = 0; band < numBands; ++band)
            peakingBands[channel][band] = Biquad {};

    currentLowCut = -1.0f;
    currentHighCut = -1.0f;
    reset();
}

void CarverEngine::reset()
{
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int band = 0; band < numBands; ++band)
        {
            peakingBands[channel][band] = Biquad {};
            gainDbCurrent[channel][band] = 0.0f;
            gainDbTarget[channel][band] = 0.0f;
        }

        scHighPass[channel].reset();
        scLowPass[channel].reset();
    }

    for (int band = 0; band < numBands; ++band)
    {
        analysisBands[band].reset();
        envelope[(size_t) band] = 0.0f;
        bandN[(size_t) band] = 0.0f;
        bandDisplayDb[(size_t) band] = -100.0f;
    }

    displayCounter = 0;
    samplesUntilControl = 0;
    samplesUntilCoefficients = 0;

    displayFrame.lowCutHz = 20.0f;
    displayFrame.highCutHz = 20000.0f;

    for (int band = 0; band < numBands; ++band)
    {
        displayFrame.bandFrequenciesHz[band] = bandCentreFrequency (band);
        displayFrame.sidechainDb[band] = -100.0f;
        displayFrame.reductionDb[band] = 0.0f;
    }
}

void CarverEngine::updateCutFilters (const CarverSettings& settings)
{
    const float low = juce::jlimit (params::cutMin, params::cutMax, settings.lowCutHz);
    const float high = juce::jlimit (params::cutMin, params::cutMax, settings.highCutHz);

    if (std::abs (low - currentLowCut) > 0.01f)
    {
        currentLowCut = low;
        const auto coefficients = Biquad::firstOrderHighPass (sr, low);
        for (auto& filter : scHighPass)
        {
            filter = coefficients;
            filter.reset();
        }
    }

    if (std::abs (high - currentHighCut) > 0.01f)
    {
        currentHighCut = high;
        const auto coefficients = Biquad::secondOrderLowPassButterworth (sr, high);
        for (auto& filter : scLowPass)
        {
            filter = coefficients;
            filter.reset();
        }
    }
}

//==============================================================================
float CarverEngine::depthDbForLevel (float levelDb, float amountNorm) noexcept
{
    if (amountNorm <= 1.0e-5f)
        return 0.0f;

    const float threshold = -43.5f - 42.0f * std::log10 (amountNorm);
    const float depth = 33.10f / (1.0f + std::exp (-(levelDb - threshold) / 3.75f));
    return juce::jlimit (0.0f, 33.10f, depth);
}

void CarverEngine::updateControl (const CarverSettings& settings)
{
    if (settings.freeze)
        return;

    const float amountNorm = juce::jlimit (0.0f, 1.0f, settings.amountPercent / 100.0f);
    const float panNorm = juce::jlimit (-1.0f, 1.0f, (settings.pan - 50.0f) / 50.0f);
    const float weightA = juce::jlimit (0.0f, 1.0f, 1.0f - panNorm);
    const float weightB = juce::jlimit (0.0f, 1.0f, 1.0f + panNorm);
    const float weights[2] = { weightA, weightB };

    for (int band = 0; band < numBands; ++band)
    {
        const float levelDb = juce::Decibels::gainToDecibels (envelope[(size_t) band], -120.0f);
        bandDisplayDb[(size_t) band] = levelDb;

        const float depthDb = depthDbForLevel (levelDb, amountNorm);

        // Small depth-dependent compensation: the peaking cascade loses a
        // little depth in the transition region (measured against the
        // reference), more so at low depths than at full depth.
        const float compensation = 1.45f - 0.36f * (depthDb / 33.10f);

        bandN[(size_t) band] = juce::jlimit (0.0f, 1.0f, depthDb * compensation / 33.10f);

        for (int channel = 0; channel < 2; ++channel)
            gainDbTarget[channel][band] = -depthDb * compensation
                                          * bandScale[(size_t) band] * weights[channel];
    }
}

void CarverEngine::updateCoefficients()
{
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int band = 0; band < numBands; ++band)
        {
            const float gainDb = gainDbCurrent[channel][band];

            if (std::abs (gainDb) < 1.0e-4f)
            {
                peakingBands[channel][band] = Biquad {};
                continue;
            }

            auto& filter = peakingBands[channel][band];
            const auto newCoefficients = Biquad::peaking (sr, bandCentreFrequency (band),
                                                          peakingBandQ, gainDb);

            filter.b0 = newCoefficients.b0;
            filter.b1 = newCoefficients.b1;
            filter.b2 = newCoefficients.b2;
            filter.a1 = newCoefficients.a1;
            filter.a2 = newCoefficients.a2;
        }
    }
}

//==============================================================================
void CarverEngine::process (juce::AudioBuffer<float>& main,
                            const juce::AudioBuffer<float>& sidechain,
                            int numSamples,
                            const CarverSettings& settings)
{
    updateCutFilters (settings);

    // Fast peak detector; the reference's Attack control is deliberately weak
    // (measured onset stays in the 3-7 ms range), so the detector attack is
    // clamped to a fast value and the parameter mainly shapes the gain ramp.
    const float attackCoeff = onePoleCoefficient (juce::jmin (settings.attackMs, 0.8f), sr);
    const float releaseCoeff = onePoleCoefficient (settings.releaseMs * 1.55f, sr);

    const bool stereo = main.getNumChannels() > 1;
    auto* mainL = main.getWritePointer (0);
    auto* mainR = stereo ? main.getWritePointer (1) : mainL;

    const bool externalSidechain = sidechain.getNumChannels() > 0
                                   && sidechain.getNumSamples() >= numSamples;

    const float* scL = externalSidechain ? sidechain.getReadPointer (0) : mainL;
    const float* scR = externalSidechain
                           ? (sidechain.getNumChannels() > 1 ? sidechain.getReadPointer (1) : scL)
                           : mainR;

    const bool midSide = settings.mode != 0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inputL = mainL[i];
        const float inputR = stereo ? mainR[i] : inputL;

        float scLeft = scL[i];
        float scRight = scR[i];

        scLeft = scLowPass[0].process (scHighPass[0].process (scLeft));
        scRight = scLowPass[1].process (scHighPass[1].process (scRight));
        const float sideMono = 0.5f * (scLeft + scRight);

        for (int band = 0; band < numBands; ++band)
        {
            const float bandOutput = analysisBands[band].process (sideMono);
            const float rectified = std::abs (bandOutput);
            const bool rising = rectified > envelope[(size_t) band];
            envelope[(size_t) band] += (rectified - envelope[(size_t) band])
                                       * (rising ? attackCoeff : releaseCoeff);
        }

        if (samplesUntilControl <= 0)
        {
            updateControl (settings);
            samplesUntilControl = controlInterval;
        }

        --samplesUntilControl;

        for (int band = 0; band < numBands; ++band)
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                const float target = gainDbTarget[channel][band];
                gainDbCurrent[channel][band] += (target - gainDbCurrent[channel][band])
                                                / (float) controlInterval;
            }
        }

        if (samplesUntilCoefficients <= 0)
        {
            updateCoefficients();
            samplesUntilCoefficients = coefficientInterval;
        }

        --samplesUntilCoefficients;

        float outputL;
        float outputR;

        if (settings.scAudition)
        {
            outputL = scLeft;
            outputR = scRight;
        }
        else if (! midSide)
        {
            float left = inputL;
            float right = inputR;

            for (int band = 0; band < numBands; ++band)
            {
                left = peakingBands[0][band].process (left);
                right = peakingBands[1][band].process (right);
            }

            outputL = left;
            outputR = right;
        }
        else
        {
            const float midIn = 0.5f * (inputL + inputR);
            const float sideIn = 0.5f * (inputL - inputR);

            float mid = midIn;
            float side = sideIn;

            for (int band = 0; band < numBands; ++band)
            {
                mid = peakingBands[0][band].process (mid);
                side = peakingBands[1][band].process (side);
            }

            outputL = mid + side;
            outputR = mid - side;
        }

        mainL[i] = outputL;
        if (stereo)
            mainR[i] = outputR;

        if (++displayCounter >= 64)
        {
            displayCounter = 0;
            updateDisplay (settings);
        }
    }
}

//==============================================================================
void CarverEngine::updateDisplay (const CarverSettings& settings)
{
    DisplayFrame frame;
    frame.lowCutHz = settings.lowCutHz;
    frame.highCutHz = settings.highCutHz;
    frame.hoverFrequencyHz = displayFrame.hoverFrequencyHz;
    frame.hoverDb = displayFrame.hoverDb;

    for (int band = 0; band < numBands; ++band)
    {
        frame.bandFrequenciesHz[band] = bandCentreFrequency (band);
        frame.sidechainDb[band] = bandDisplayDb[(size_t) band];
        frame.reductionDb[band] = -33.10f * bandN[(size_t) band];
    }

    {
        const juce::SpinLock::ScopedLockType lock (displayLock);
        displayFrame = frame;
    }
}

//==============================================================================
void CarverEngine::getDisplayFrame (DisplayFrame& destination) const
{
    const juce::SpinLock::ScopedLockType lock (displayLock);
    destination = displayFrame;
}

} // namespace ts
