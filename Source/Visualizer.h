#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ScopeFifo.h"

// The VIS tab: goniometer (stereo field), L/R peak + RMS meters, phase correlation
// and a stereo-width readout, all computed from the plugin's final output.
class VisPage : public juce::Component,
                private juce::Timer
{
public:
    explicit VisPage (ScopeFifo&);
    ~VisPage() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    // Pull new audio, update the analysis and trails (called by the timer)
    void refresh();

private:
    void timerCallback() override { refresh(); }
    void updateGoniometer (int numSamples);
    void paintGoniometer (juce::Graphics&);
    void paintMeters (juce::Graphics&);

    ScopeFifo& scope;
    std::vector<float> left, right;

    juce::Rectangle<int> gonioPanel, meterPanel;
    juce::Rectangle<float> gonioPlot;
    juce::Image trails;
    float gonioScale = 4.0f;

    float rmsSq[2] {}, peak[2] {}, hold[2] {};
    double holdTime[2] {};
    float midSq = 0.0f, sideSq = 0.0f, correlation = 0.0f;
};
