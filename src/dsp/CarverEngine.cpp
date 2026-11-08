#include "CarverEngine.h"

#include <cmath>

namespace ts
{
namespace
{
    // Sidechain level below which a band is considered silent. Bands at or
    // above 0 dBFS get the full reduction.
    constexpr float sidechainFloorDb = -54.0f;

    // 2nd-order Butterworth (Q = 0.7071) for the low/high cut analysis filters.
    constexpr float cutFilterQ = 0.70710678f;

    float onePoleCoefficient (float timeMs, float sampleRate, float intervalSeconds)
    {
        const float tau = juce::jmax (0.01f, timeMs) * 0.001f;
        return 1.0f - std::exp (-intervalSeconds / tau);
    }
} // namespace

CarverEngine::CarverEngine()
    : analysisBuffer ((size_t) fftSize * 2, 0.0f),
      msScratch ((size_t) fftSize * 2, 0.0f),
      synthWindow ((size_t) fftSize, 0.0f),
      spectrumIn ((size_t) fftSize),
      spectrumOut ((size_t) fftSize),
      inL ((size_t) fftSize, 0.0f),
      inR ((size_t) fftSize, 0.0f),
      scL ((size_t) fftSize, 0.0f),
      scR ((size_t) fftSize, 0.0f),
      accL ((size_t) fftSize, 0.0f),
      accR ((size_t) fftSize, 0.0f),
      outFifoL ((size_t) hopSize, 0.0f),
      outFifoR ((size_t) hopSize, 0.0f),
      outFifoScl ((size_t) hopSize, 0.0f),
      outFifoScr ((size_t) hopSize, 0.0f)
{
    for (int i = 0; i < numBands; ++i)
        displayBandFrequencies[(size_t) i] = BandAnalyzer::bandFrequency (i);

    // Hann analysis + synthesis window with overlap normalisation.
    // Sum of squared Hann windows over hopSize shifts is 3 * fftSize / (8 * hopSize).
    window.multiplyWithWindowingTable (synthWindow.data(), (size_t) fftSize);
    const float normalisation = (float) hopSize * 8.0f / (3.0f * (float) fftSize);

    for (auto& value : synthWindow)
        value *= normalisation;

    displayFrame.lowCutHz = 20.0f;
    displayFrame.highCutHz = 20000.0f;
}

void CarverEngine::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    analyzer.prepare (sr);
    reset();
}

void CarverEngine::reset()
{
    std::fill (inL.begin(), inL.end(), 0.0f);
    std::fill (inR.begin(), inR.end(), 0.0f);
    std::fill (scL.begin(), scL.end(), 0.0f);
    std::fill (scR.begin(), scR.end(), 0.0f);
    std::fill (accL.begin(), accL.end(), 0.0f);
    std::fill (accR.begin(), accR.end(), 0.0f);
    std::fill (outFifoL.begin(), outFifoL.end(), 0.0f);
    std::fill (outFifoR.begin(), outFifoR.end(), 0.0f);
    std::fill (outFifoScl.begin(), outFifoScl.end(), 0.0f);
    std::fill (outFifoScr.begin(), outFifoScr.end(), 0.0f);
    outFifoPos = hopSize;
    inWrite = 0;
    totalSamples = 0;
    currentReductionDb.fill (0.0f);
    targetReductionDb.fill (0.0f);
    sidechainBandDb.fill (-100.0f);
    hpfL.reset(); hpfR.reset(); lpfL.reset(); lpfR.reset();
    currentLowCut = -1.0f;
    currentHighCut = -1.0f;
}

void CarverEngine::updateFilters (const CarverSettings& settings)
{
    const float low = juce::jlimit (20.0f, 20000.0f, settings.lowCutHz);
    const float high = juce::jlimit (20.0f, 20000.0f, settings.highCutHz);

    if (std::abs (low - currentLowCut) > 0.01f)
    {
        currentLowCut = low;
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, low, cutFilterQ);
        *hpfL.coefficients = *coefficients;
        *hpfR.coefficients = *coefficients;
        hpfL.reset();
        hpfR.reset();
    }

    if (std::abs (high - currentHighCut) > 0.01f)
    {
        currentHighCut = high;
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, high, cutFilterQ);
        *lpfL.coefficients = *coefficients;
        *lpfR.coefficients = *coefficients;
        lpfL.reset();
        lpfR.reset();
    }
}

void CarverEngine::process (juce::AudioBuffer<float>& main,
                            const juce::AudioBuffer<float>& sidechain,
                            int numSamples,
                            const CarverSettings& settings)
{
    updateFilters (settings);

    cachedMode = settings.mode;
    cachedPan = settings.pan;

    const bool stereoOut = main.getNumChannels() > 1;
    auto* mainL = main.getWritePointer (0);
    auto* mainR = stereoOut ? main.getWritePointer (1) : mainL;

    const bool haveSidechain = sidechain.getNumChannels() > 0;
    const float* scLeft = nullptr;
    const float* scRight = nullptr;

    if (haveSidechain)
    {
        scLeft = sidechain.getReadPointer (0);
        scRight = sidechain.getNumChannels() > 1 ? sidechain.getReadPointer (1) : scLeft;
    }
    else
    {
        scLeft = mainL;
        scRight = mainR;
    }

    const float hopSeconds = (float) hopSize / (float) sr;
    const float attackCoeff = onePoleCoefficient (settings.attackMs, (float) sr, hopSeconds);
    const float releaseCoeff = onePoleCoefficient (settings.releaseMs, (float) sr, hopSeconds);

    for (int i = 0; i < numSamples; ++i)
    {
        // Read the input sample before overwriting the buffer.
        const float inputL = mainL[i];
        const float inputR = stereoOut ? mainR[i] : inputL;

        // Emit the delayed sample.
        if (outFifoPos < hopSize)
        {
            const float outL = settings.scAudition ? outFifoScl[(size_t) outFifoPos] : outFifoL[(size_t) outFifoPos];
            const float outR = settings.scAudition ? outFifoScr[(size_t) outFifoPos] : outFifoR[(size_t) outFifoPos];
            mainL[i] = outL;
            if (stereoOut)
                mainR[i] = outR;
            ++outFifoPos;
        }
        else
        {
            mainL[i] = 0.0f;
            if (stereoOut)
                mainR[i] = 0.0f;
        }

        // Push new samples into the rings, filtering the sidechain.
        inL[(size_t) inWrite] = inputL;
        inR[(size_t) inWrite] = inputR;

        const float rawScL = haveSidechain ? scLeft[i] : inputL;
        const float rawScR = haveSidechain ? scRight[i] : inputR;
        scL[(size_t) inWrite] = lpfL.processSample (hpfL.processSample (rawScL));
        scR[(size_t) inWrite] = lpfR.processSample (hpfR.processSample (rawScR));

        inWrite = (inWrite + 1) % fftSize;
        ++totalSamples;

        if (totalSamples % hopSize == 0)
        {
            if (! settings.freeze)
            {
                processHop (settings);

                for (int band = 0; band < numBands; ++band)
                {
                    const float target = targetReductionDb[(size_t) band];
                    const float current = currentReductionDb[(size_t) band];
                    const float coefficient = (target < current) ? attackCoeff : releaseCoeff;
                    currentReductionDb[(size_t) band] = current + (target - current) * coefficient;
                }
            }
            else
            {
                // Freeze: keep analysing for the display, but hold the gains.
                processHop (settings);
            }

            if (totalSamples >= fftSize)
                renderHop();
            else
                outFifoPos = hopSize;
        }
    }

    // Publish a display snapshot.
    DisplayFrame frame;
    frame.lowCutHz = settings.lowCutHz;
    frame.highCutHz = settings.highCutHz;
    frame.hoverFrequencyHz = displayFrame.hoverFrequencyHz;
    frame.hoverDb = displayFrame.hoverDb;

    for (int band = 0; band < numBands; ++band)
    {
        frame.bandFrequenciesHz[band] = displayBandFrequencies[(size_t) band];
        frame.sidechainDb[band] = sidechainBandDb[(size_t) band];
        frame.reductionDb[band] = currentReductionDb[(size_t) band];
    }

    {
        const juce::SpinLock::ScopedLockType lock (displayLock);
        displayFrame = frame;
    }
}

void CarverEngine::processHop (const CarverSettings& settings)
{
    // Analyse the filtered sidechain as a mono sum into 32 bands.
    const int start = inWrite; // oldest sample of the current window

    for (int i = 0; i < fftSize; ++i)
    {
        const int index = (start + i) % fftSize;
        analysisBuffer[(size_t) i] = 0.5f * (scL[(size_t) index] + scR[(size_t) index]);
    }

    analyzer.analyze (analysisBuffer.data(), sidechainBandDb.data());

    if (! settings.freeze)
    {
        const float depthDb = maxReductionDb * juce::jlimit (0.0f, 1.0f, settings.amountPercent / 100.0f);

        for (int band = 0; band < numBands; ++band)
        {
            const float level = sidechainBandDb[(size_t) band];
            const float weight = juce::jlimit (0.0f, 1.0f,
                                               (level - sidechainFloorDb) / (0.0f - sidechainFloorDb));
            targetReductionDb[(size_t) band] = depthDb * weight;
        }
    }
}

float CarverEngine::bandGainDbForFrequency (float frequencyHz) const noexcept
{
    constexpr float low = 20.0f;
    constexpr float high = 20000.0f;
    constexpr float ratio = high / low;

    if (frequencyHz <= low)
        return currentReductionDb[0];

    if (frequencyHz >= high)
        return currentReductionDb[(size_t) (numBands - 1)];

    const float position = std::log (frequencyHz / low) / std::log (ratio) * (float) (numBands - 1);
    const int lower = juce::jlimit (0, numBands - 2, (int) std::floor (position));
    const float fraction = position - (float) lower;

    return currentReductionDb[(size_t) lower]
         + (currentReductionDb[(size_t) (lower + 1)] - currentReductionDb[(size_t) lower]) * fraction;
}

void CarverEngine::renderHop()
{
    const int start = inWrite;

    // Delayed filtered sidechain (used by SC Audition).
    for (int i = 0; i < hopSize; ++i)
    {
        const int index = (start + i) % fftSize;
        outFifoScl[(size_t) i] = scL[(size_t) index];
        outFifoScr[(size_t) i] = scR[(size_t) index];
    }

    const bool midSide = (cachedMode != 0);
    const float panNorm = juce::jlimit (-1.0f, 1.0f, cachedPan / 100.0f);
    const float weightA = juce::jlimit (0.0f, 1.0f, 1.0f - panNorm); // left / mid
    const float weightB = juce::jlimit (0.0f, 1.0f, 1.0f + panNorm); // right / side

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const int index = (start + i) % fftSize;
            const float l = inL[(size_t) index];
            const float r = inR[(size_t) index];
            analysisBuffer[(size_t) i] = midSide ? (ch == 0 ? 0.5f * (l + r) : 0.5f * (l - r))
                                                 : (ch == 0 ? l : r);
        }

        window.multiplyWithWindowingTable (analysisBuffer.data(), (size_t) fftSize);

        for (int i = 0; i < fftSize; ++i)
            spectrumIn[(size_t) i] = Complex (analysisBuffer[(size_t) i], 0.0f);

        fft.perform (spectrumIn.data(), spectrumOut.data(), false);

        const float channelWeight = (ch == 0 ? weightA : weightB);
        const int positiveBins = fftSize / 2;

        spectrumOut[0] *= channelWeight;

        for (int bin = 1; bin <= positiveBins; ++bin)
        {
            const float frequency = (float) bin * (float) sr / (float) fftSize;
            const float gainDb = bandGainDbForFrequency (frequency) * channelWeight;
            const float gain = juce::Decibels::decibelsToGain (gainDb);
            spectrumOut[(size_t) bin] *= gain;

            if (bin < positiveBins)
                spectrumOut[(size_t) (fftSize - bin)] *= gain;
        }

        fft.perform (spectrumOut.data(), spectrumIn.data(), true);

        for (int i = 0; i < fftSize; ++i)
            msScratch[(size_t) (ch * fftSize + i)] = spectrumIn[(size_t) i].real() * synthWindow[(size_t) i];
    }

    if (midSide)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const int index = (start + i) % fftSize;
            const float m = msScratch[(size_t) i];
            const float s = msScratch[(size_t) (fftSize + i)];
            accL[(size_t) index] += m + s;
            accR[(size_t) index] += m - s;
        }
    }
    else
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const int index = (start + i) % fftSize;
            accL[(size_t) index] += msScratch[(size_t) i];
            accR[(size_t) index] += msScratch[(size_t) (fftSize + i)];
        }
    }

    // Emit hopSize samples and clear the consumed region.
    for (int i = 0; i < hopSize; ++i)
    {
        const int index = (start + i) % fftSize;
        outFifoL[(size_t) i] = accL[(size_t) index];
        outFifoR[(size_t) i] = accR[(size_t) index];
        accL[(size_t) index] = 0.0f;
        accR[(size_t) index] = 0.0f;
    }

    outFifoPos = 0;
}

void CarverEngine::getDisplayFrame (DisplayFrame& destination) const
{
    const juce::SpinLock::ScopedLockType lock (displayLock);
    destination = displayFrame;
}

} // namespace ts
