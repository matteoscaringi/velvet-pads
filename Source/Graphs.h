#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Monitor.h"

//==============================================================================
// Binds one parameter to a graph: tracks its value and writes host-visible gestures.
class ParamRef
{
public:
    ParamRef (juce::AudioProcessorValueTreeState& state, const juce::String& id, std::function<void()> onChange)
        : param (*state.getParameter (id)),
          attachment (param, [this, onChange] (float v) { value = v; if (onChange) onChange(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }

    float get() const  { return value; }
    float norm() const { return param.convertTo0to1 (value); }

    void begin() { attachment.beginGesture(); }
    void end()   { attachment.endGesture(); }
    void set (float v)      { attachment.setValueAsPartOfGesture (param.getNormalisableRange().snapToLegalValue (v)); }
    void setNorm (float n)  { set (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n))); }
    void reset()            { attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
    juce::String text() const { return param.getCurrentValueAsText(); }

    juce::RangedAudioParameter& param;

private:
    float value = 0.0f;
    juce::ParameterAttachment attachment;
};

//==============================================================================
class GraphBase : public juce::Component,
                  private juce::Timer
{
public:
    // Animates a playhead dot from the most recent voice; pass nullptr for no dot.
    void setMonitor (const VoiceMonitor* m)
    {
        monitor = m;
        if (m != nullptr) startTimerHz (30);
    }

    void setAccent (juce::Colour c) { accent = c; repaint(); }

protected:
    juce::Colour accent = juce::Colour (0xff3ea6ff);
    void drawPlayhead (juce::Graphics&, juce::Point<float>, juce::Colour);
    const VoiceMonitor* monitor = nullptr;
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (8.0f, 10.0f); }
    void drawBackground (juce::Graphics&, int verticalLines, int horizontalLines);
    void drawReadout (juce::Graphics&);
    void drawHandle (juce::Graphics&, juce::Point<float>, juce::Colour, bool active);

    juce::String readout; // shown in the corner while dragging

private:
    void timerCallback() override
    {
        // Repaint while a note is sounding, plus once more to clear the dot
        const bool active = monitor->active.load();
        if (active || wasActive)
            repaint();
        wasActive = active;
    }
    bool wasActive = false;
};

//==============================================================================
// ADSR display with draggable attack peak, decay/sustain corner and release end.
class EnvelopeGraph : public GraphBase
{
public:
    EnvelopeGraph (juce::AudioProcessorValueTreeState&, const juce::String& attackId, const juce::String& decayId,
                   const juce::String& sustainId, const juce::String& releaseId, juce::Colour, bool isAmpEnvelope);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    struct Points { juce::Point<float> start, peak, sustain, sustainEnd, end; float segment; };
    Points points() const;
    int handleAt (juce::Point<float>) const;
    void updateReadout();

    ParamRef attack, decay, sustain, release;
    bool isAmp;
    int hover = -1, dragging = -1;
};

//==============================================================================
// Filter magnitude response. Drag horizontally for cutoff, vertically for resonance.
class FilterGraph : public GraphBase
{
public:
    FilterGraph (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    float magnitudeDb (float freq) const;
    float freqToX (float f) const;
    float dbToY (float db) const;

    ParamRef type, cutoff, reso, lowCut;
    float dragStartReso = 0.0f;
};

//==============================================================================
// Oscillator waveform with its unison stack. Drag vertically to change detune.
class OscGraph : public GraphBase
{
public:
    OscGraph (juce::AudioProcessorValueTreeState&, const juce::String& prefix);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    ParamRef wave, voices, detune, level;
    float dragStart = 0.0f;
};

//==============================================================================
// Distortion transfer curve. Drag vertically for drive.
class DistGraph : public GraphBase
{
public:
    DistGraph (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    ParamRef type, drive;
    float dragStart = 0.0f;
};

//==============================================================================
// EQ response. Drag the mid point: horizontally for frequency, vertically for gain.
class EqGraph : public GraphBase
{
public:
    EqGraph (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    float responseDb (float freq) const;
    float freqToX (float f) const;
    float dbToY (float db) const;

    ParamRef low, mid, midFreq, high;
};

//==============================================================================
// Three seconds of LFO output including its fade-in. Drag the handle for delay,
// drag anywhere else vertically for rate.
class LfoGraph : public GraphBase
{
public:
    LfoGraph (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    static constexpr float windowSeconds = 3.0f;
    juce::Point<float> delayHandle() const;

    ParamRef shape, rate, delay;
    bool hoverHandle = false, draggingDelay = false, draggingRate = false;
    float dragStart = 0.0f;
};
