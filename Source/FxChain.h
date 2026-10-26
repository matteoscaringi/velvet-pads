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
// The FX rack: Distortion > Phaser > Chorus > Delay > Compressor > Reverb > EQ.
// Every module has its own on/off switch and is skipped entirely when off.
class FxChain
{
public:
    explicit FxChain (juce::AudioProcessorValueTreeState&);

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>&, double bpm);

private:
    void processDistortion (juce::AudioBuffer<float>&);
    void processDelay (juce::AudioBuffer<float>&, double bpm);
    void updateEq();

    struct ParamPtrs
    {
        std::atomic<float> *distOn, *distType, *distDrive, *distTone, *distMix,
                           *phaserOn, *phaserRate, *phaserDepth, *phaserFeedback, *phaserMix,
                           *chorusOn, *chorusRate, *chorusDepth, *chorusMix,
                           *delayOn, *delaySync, *delayMode, *delayFeedback, *delayTone, *delayMix,
                           *compOn, *compThreshold, *compRatio, *compAttack, *compRelease, *compGain,
                           *reverbOn, *reverbSize, *reverbDamp, *reverbWidth, *reverbMix,
                           *eqOn, *eqLow, *eqMid, *eqMidFreq, *eqHigh;
    } p;

    static bool isOn (const std::atomic<float>* a) { return a->load() > 0.5f; }

    double sampleRate = 44100.0;

    // Distortion
    juce::AudioBuffer<float> dry;
    float toneState[2] {}, dcX[2] {}, dcY[2] {}, distAutoGain = 1.0f;

    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> chorus;

    // Delay
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    float delayLp[2] {};
    juce::SmoothedValue<float> delaySamples;

    juce::dsp::Compressor<float> compressor;
    juce::dsp::Reverb reverb;

    using EqBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    EqBand eqLow, eqMid, eqHigh;
    float lastEq[4] { -999.0f, -999.0f, -999.0f, -999.0f };

    // Clear a module's tail when it's switched back on
    bool wasOn[7] {};
};
