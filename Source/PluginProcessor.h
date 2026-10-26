#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "SynthVoice.h"
#include "Monitor.h"
#include "FxChain.h"
#include "ScopeFifo.h"

class VelvetPadProcessor : public juce::AudioProcessor
{
public:
    VelvetPadProcessor();
    ~VelvetPadProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // on-screen keyboard in the editor
    VoiceMonitor monitor;                  // playhead state for the env/LFO graphs
    ScopeFifo scope;                       // final output for the VIS tab

private:
    void updateSettings();
    void applyPump (juce::AudioBuffer<float>&);
    void publishMonitor();

    // Raw parameter pointers, cached once so the audio thread never does string lookups
    struct ParamPtrs
    {
        std::atomic<float>* osc[2][8] {};
        std::atomic<float> *subWave, *subOctave, *subLevel, *noiseLevel,
                           *filterType, *cutoff, *reso, *filterEnv, *keyTrack,
                           *fAttack, *fDecay, *fSustain, *fRelease,
                           *aAttack, *aDecay, *aSustain, *aRelease, *velSens,
                           *lfoShape, *lfoRate, *lfoDelay, *lfoPitch, *lfoCutoff, *lfoAmp,
                           *master,
                           *osc1On, *osc2On, *subOn, *noiseOn, *filterOn, *lfoOn,
                           *lowCut, *drive, *filterA, *filterB, *filterN, *filterS, *pumpOn, *pumpRate, *pumpShape, *pumpDepth;
    } p;

    static constexpr int numVoices = 16;

    SynthSettings settings;
    StringSynth synth { settings };

    FxChain fx { apvts };
    juce::SmoothedValue<float> masterGain;


    double currentSampleRate = 44100.0;
    double pumpBeats = 0.0;     // free-running beat position when the host isn't playing
    float pumpGain = 1.0f;      // smoothed so step edges never click

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetPadProcessor)
};
