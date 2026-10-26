#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace Dist
{
// Waveshapers for the distortion module; the editor draws the same curves.
inline float shape (int type, float x, float drive)
{
    const float g = juce::Decibels::decibelsToGain (drive * 36.0f);
    switch (type)
    {
        case 0: return std::tanh (x * g);                                          // soft clip
        case 1: return juce::jlimit (-1.0f, 1.0f, x * g);                          // hard clip
        case 2: return std::tanh (x * g + 0.3f) - std::tanh (0.3f);                // tube (asymmetric)
        case 3: return std::sin (x * g * juce::MathConstants<float>::halfPi);      // wavefolder
        default: {                                                                 // bitcrush, 12 down to 2 bits
            const float steps = std::pow (2.0f, 12.0f - drive * 10.0f);
            return std::round (x * steps) / steps;
        }
    }
}
} // namespace Dist

//==============================================================================
// Simple stereo TPT state-variable filter used inside the FX (LP / BP / HP outputs).
struct StereoSvf
{
    float ic1[2] {}, ic2[2] {}, a1 = 0, a2 = 0, a3 = 0, k = 1.414f;

    void reset() { std::fill (std::begin (ic1), std::end (ic1), 0.0f); std::fill (std::begin (ic2), std::end (ic2), 0.0f); }

    void set (float freq, float q, double sr)
    {
        const float g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, (float) sr * 0.45f, freq) / (float) sr);
        k = 1.0f / juce::jmax (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // type: 0 = LP, 1 = BP (unity peak), 2 = HP
    float process (int ch, float x, int type)
    {
        const float v3 = x - ic2[ch];
        const float v1 = a1 * ic1[ch] + a2 * v3;
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
        ic1[ch] = 2.0f * v1 - ic1[ch];
        ic2[ch] = 2.0f * v2 - ic2[ch];
        return type == 0 ? v2 : type == 1 ? v1 * k : x - k * v1 - v2;
    }
};

//==============================================================================
// Algorithmic reverb: pre-delay > input diffusion > 8-line feedback delay network with
// modulated reads ("spin"), damping, decay time, then low/high cut and width on the tail.
class FdnReverb
{
public:
    struct Settings
    {
        int mode = 0;               // 0 hall, 1 plate, 2 room
        float preDelayMs = 20.0f, size = 0.8f, decaySeconds = 3.5f;
        float lowCut = 200.0f, highCut = 7000.0f, spinHz = 0.6f, spinDepth = 0.4f, width = 1.0f;
    };

    void prepare (double sampleRate);
    void reset();
    // Adds nothing to the input: returns the wet signal only, in wetL/wetR
    void process (const float* inL, const float* inR, float* wetL, float* wetR, int numSamples, const Settings&);

private:
    static constexpr int numLines = 8, numDiffusers = 4;

    struct DelayLine
    {
        std::vector<float> buffer;
        int write = 0;
        void resize (int n) { buffer.assign ((size_t) juce::jmax (4, n), 0.0f); write = 0; }
        void push (float x) { buffer[(size_t) write] = x; if (++write >= (int) buffer.size()) write = 0; }
        float read (float delay) const   // fractional, 1 <= delay < size
        {
            // Split into whole and fractional samples using integers, so float rounding can
            // never produce an index equal to the buffer size (that read past the end of the
            // buffer and fed garbage into the reverb, which then exploded).
            const int n = (int) buffer.size();
            const float d = juce::jlimit (1.0f, (float) (n - 2), delay);
            const int whole = (int) d;
            const float frac = d - (float) whole;
            int i0 = write - whole;            // sample 'whole' steps back
            if (i0 < 0) i0 += n;
            int i1 = i0 - 1;                   // one step further back
            if (i1 < 0) i1 += n;
            return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
        }
    };

    double sr = 44100.0;
    DelayLine preDelay[2], lines[numLines], diffusers[2][numDiffusers];
    float damp[numLines] {}, spinPhase[numLines] {};
    StereoSvf lowCut, highCut;
};

//==============================================================================
// The FX rack: Distortion > Phaser > Chorus > Delay > Compressor > Reverb > EQ.
// Every module has its own on/off switch and is skipped entirely when off.
class FxChain
{
public:
    explicit FxChain (juce::AudioProcessorValueTreeState&);

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>&, double bpm);
    void reset();   // clears all tails and filter states (no allocation, audio-thread safe)

private:
    void processDistortion (juce::AudioBuffer<float>&);
    void processDelay (juce::AudioBuffer<float>&, double bpm);
    void processReverb (juce::AudioBuffer<float>&);
    void updateEq();

    juce::AudioProcessorValueTreeState& state;
    std::atomic<float>* raw (const char* id) const { return state.getRawParameterValue (id); }
    static float get (const std::atomic<float>* param) { return param->load(); }

    struct ParamPtrs
    {
        std::atomic<float> *distOn, *distType, *distDrive, *distFilterMode, *distFilterType, *distFilterFreq, *distFilterQ, *distMix,
                           *phaserOn, *phaserRate, *phaserDepth, *phaserFreq, *phaserFeedback, *phaserMix,
                           *chorusOn, *chorusRate, *chorusDepth, *chorusDelay, *chorusFeedback, *chorusMix,
                           *delayOn, *delaySync, *delayMode, *delayFeedback, *delayLowCut, *delayHighCut, *delayMix,
                           *compOn, *compThreshold, *compRatio, *compAttack, *compRelease, *compGain, *compMix,
                           *reverbOn, *reverbMode, *reverbPreDelay, *reverbSize, *reverbDecay, *reverbLowCut,
                           *reverbHighCut, *reverbSpin, *reverbSpinDepth, *reverbWidth, *reverbMix,
                           *eqOn, *eqLow, *eqMid, *eqMidFreq, *eqMidQ, *eqHigh;
    } p;

    static bool isOn (const std::atomic<float>* a) { return a->load() > 0.5f; }

    double sampleRate = 44100.0;
    juce::AudioBuffer<float> dry, wet;

    // Distortion
    StereoSvf distFilter;
    float dcX[2] {}, dcY[2] {}, distAutoGain = 1.0f;

    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> chorus;

    // Delay
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    StereoSvf delayLowCutFilter, delayHighCutFilter;
    juce::SmoothedValue<float> delaySamples;

    juce::dsp::Compressor<float> compressor;
    FdnReverb reverb;

    using EqBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    EqBand eqLow, eqMid, eqHigh;
    float lastEq[5] { -999.0f, -999.0f, -999.0f, -999.0f, -999.0f };

    // Clear a module's tail when it's switched back on
    bool wasOn[7] {};
};
