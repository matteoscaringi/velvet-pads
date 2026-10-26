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
    void setBoxedLabel (bool b) { boxedLabel = b; repaint(); }
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String name;
    bool boxedLabel = false;
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
    void setAccent (juce::Colour c) { box.setColour (juce::ComboBox::textColourId, c); }
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

    // Colours the section: stripe, power square, knobs, value boxes, selectors, tabs and graphs
    void setAccent (juce::Colour);

protected:
    juce::Colour accent = Theme::accent;
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

    // Routing: which sources go through the filter (A, B, Noise, Sub)
    juce::ToggleButton routeA { "A" }, routeB { "B" }, routeN { "N" }, routeS { "S" };
    std::unique_ptr<APVTS::ButtonAttachment> routeAttachments[4];
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

// One effect strip in the FX rack: title tag + selectors on the left, optional graph,
// Vertical list of LED buttons for a choice parameter (e.g. OFF / PRE / POST, HALL / PLATE / ROOM)
class ChoiceLeds : public juce::Component
{
public:
    ChoiceLeds (APVTS&, const juce::String& paramId, juce::Colour);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int preferredHeight() const { return names.size() * 18; }

private:
    ParamRef choice;
    juce::StringArray names;
    juce::Colour colour;
};

// One FX module as a rack unit: ears with bolts, coloured tag, a single row of controls,
// Mix set apart on the right and the power button on the right ear.
class RackStrip : public juce::Component
{
public:
    RackStrip (APVTS&, const juce::String& title, juce::Colour, const juce::String& powerId);

    // Builders, in left-to-right order
    RackStrip& selector (const char* paramId, int width = 120);
    RackStrip& leds (const char* paramId, int width = 64);
    RackStrip& graph (std::unique_ptr<juce::Component>, int width);
    RackStrip& knob (const char* paramId, const char* name);
    RackStrip& mix (const char* paramId);

    static constexpr int unitHeight = 88;
    void setSelected (bool);
    std::function<void()> onSelect;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override { if (onSelect) onSelect(); }

    const juce::String title;
    const juce::Colour colour;

private:
    struct Item { juce::Component* component; int width; bool isKnob; };

    APVTS& state;
    std::vector<Item> items;
    juce::OwnedArray<juce::Component> owned;
    Knob* mixKnob = nullptr;
    juce::ToggleButton power;
    std::unique_ptr<APVTS::ButtonAttachment> powerAttachment;
    bool selected = false;
};

// One row in the module list: LED switch + name. Clicking the name selects (and enables) it.
class RackListItem : public juce::Component
{
public:
    RackListItem (APVTS&, const juce::String& name, juce::Colour, const juce::String& powerId);

    bool isOn() const { return led.getToggleState(); }
    void setSelected (bool);
    std::function<void()> onSelect, onPowerChanged;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::String name;
    juce::Colour colour;
    juce::ToggleButton led;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
    bool selected = false;
};

// The FX tab: module list on the left, rack of enabled modules on the right (signal-flow order).
class FxPage : public juce::Component
{
public:
    explicit FxPage (APVTS&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void select (int index);
    void layoutRack();

    struct RackContent : public juce::Component
    {
        bool empty = true;
        void paint (juce::Graphics&) override;
    };

    juce::OwnedArray<RackStrip> strips;
    juce::OwnedArray<RackListItem> items;
    RackContent rack;
    juce::Viewport viewport;
    juce::Rectangle<int> listArea, rackArea;
    int selectedIndex = 0;
};

//==============================================================================
// On-screen mod wheel. Shows the hardware wheel (MIDI CC1) and can be dragged like one.
class ModWheel : public juce::Component,
                 public juce::SettableTooltipClient,
                 private juce::Timer
{
public:
    explicit ModWheel (std::atomic<float>& value);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { value = 0.0f; repaint(); }

private:
    void timerCallback() override;

    std::atomic<float>& value;
    float shown = -1.0f, dragStart = 0.0f;
};

//==============================================================================
// Master output meters: two segmented bars (green > orange > red) with peak hold and a clip light.
class OutputMeter : public juce::Component,
                    public juce::SettableTooltipClient,
                    private juce::Timer
{
public:
    explicit OutputMeter (std::atomic<float>* peaks);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { clipped[0] = clipped[1] = false; repaint(); }

private:
    void timerCallback() override;

    std::atomic<float>* peaks;
    float level[2] {}, hold[2] {};
    int holdFrames[2] {};
    bool clipped[2] {};
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
    OutputMeter outputMeter;
    ModWheel modWheel;
    Knob wheelCutoff, wheelVibrato;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelvetPadEditor)
};
