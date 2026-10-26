#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Graphs.h"
#include "Theme.h"
#include "Visualizer.h"

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
class VelvetLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VelvetLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.5f, juce::Font::bold); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return juce::FontOptions (12.0f, juce::Font::bold); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

//==============================================================================
// Rotary knob with its name underneath; value pops up while hovering/dragging.
class Knob : public juce::Component
{
public:
    Knob (APVTS&, const juce::String& paramId, const juce::String& name);
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String name;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// Compact "NAME  value" box, dragged up/down like a number field.
class Field : public juce::Component
{
public:
    Field (APVTS&, const juce::String& paramId, const juce::String& name);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String name;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// Choice parameter as a centred display with < > stepping arrows.
class Selector : public juce::Component
{
public:
    Selector (APVTS&, const juce::String& paramId);
    void resized() override;

private:
    juce::ComboBox box;
    juce::ArrowButton prev { "prev", 0.5f, Theme::dimText }, next { "next", 0.0f, Theme::dimText };
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

//==============================================================================
// Bevelled panel with a title strip; subclasses lay out content().
class Panel : public juce::Component
{
public:
    Panel (const juce::String& t, APVTS* state = nullptr, const juce::String& powerId = {});
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

protected:
    static constexpr int headerHeight = 24;
    juce::Rectangle<int> header() const  { return getLocalBounds().removeFromTop (headerHeight); }
    juce::Rectangle<int> content() const { return getLocalBounds().withTrimmedTop (headerHeight).reduced (8, 6); }
    static void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*>);

    juce::String title;

private:
    juce::ToggleButton power;   // the square in the header: switches the section on/off
    std::unique_ptr<APVTS::ButtonAttachment> powerAttachment;
};

class OscPanel : public Panel
{
public:
    OscPanel (APVTS&, int index);
    void resized() override;

private:
    Selector wave;
    Field octave, semi, fine;
    OscGraph graph;
    Field unison;
    Knob detune, width, level;
};

class SubPanel : public Panel
{
public:
    explicit SubPanel (APVTS&);
    void resized() override;

private:
    Selector wave, octave;
    Knob level;
};

class AirPanel : public Panel
{
public:
    explicit AirPanel (APVTS&);
    void resized() override;

private:
    Knob level;
};

class FilterPanel : public Panel
{
public:
    explicit FilterPanel (APVTS&);
    void resized() override;

private:
    Selector type;
    FilterGraph graph;
    Knob cutoff, reso, drive, envAmount, keyTrack, lowCut;
};

class EnvPanel : public Panel
{
public:
    explicit EnvPanel (APVTS&);
    void setMonitor (const VoiceMonitor* m) { ampGraph.setMonitor (m); filterGraph.setMonitor (m); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int);

    juce::TextButton ampTab { "ENV 1  AMP" }, filterTab { "ENV 2  FILTER" };
    EnvelopeGraph ampGraph, filterGraph;
    Knob aA, aD, aS, aR, vel, fA, fD, fS, fR;
    int page = 0;
};

class LfoPanel : public Panel
{
public:
    explicit LfoPanel (APVTS&);
    void setMonitor (const VoiceMonitor* m) { graph.setMonitor (m); }
    void resized() override;

private:
    Selector shape;
    LfoGraph graph;
    Knob rate, delay, pitch, cutoff, amp;
};

class PumpPanel : public Panel
{
public:
    explicit PumpPanel (APVTS&);
    void resized() override;

private:
    Selector rate, shape;
    Knob depth;
};

// One effect in the FX rack: optional graph on the left, selectors on top, knob row below.
class FxModule : public Panel
{
public:
    FxModule (APVTS&, const juce::String& title, const juce::String& powerId,
              std::initializer_list<std::pair<const char*, const char*>> knobs,
              std::initializer_list<const char*> selectorIds = {},
              std::unique_ptr<juce::Component> graph = nullptr);
    void resized() override;

private:
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Selector> selectors;
    std::unique_ptr<juce::Component> graph;
};

// The FX tab: the whole effects rack, laid out in signal-flow order.
class FxPage : public juce::Component
{
public:
    explicit FxPage (APVTS&);
    void resized() override;

private:
    FxModule distortion, phaser, chorus, delay, compressor, reverb, eq;
};

//==============================================================================
class VelvetPadEditor : public juce::AudioProcessorEditor
{
public:
    explicit VelvetPadEditor (VelvetPadProcessor&);
    ~VelvetPadEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int);

    VelvetLookAndFeel lnf;

    SubPanel sub;
    AirPanel air;
    OscPanel oscA, oscB;
    FilterPanel filter;
    EnvPanel env;
    LfoPanel lfo;
    PumpPanel pump;
    FxPage fxPage;
    VisPage visPage;
    juce::TextButton oscTab { "OSC" }, fxTab { "FX" }, visTab { "VIS" };
    Knob master;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetPadEditor)
};
