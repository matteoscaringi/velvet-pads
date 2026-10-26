#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
const juce::Colour background  { 0xff15181c };
const juce::Colour panelTop    { 0xff30363d };
const juce::Colour panelBottom { 0xff252a30 };
const juce::Colour headerTop   { 0xff434c56 };
const juce::Colour headerBot   { 0xff353c45 };
const juce::Colour edge        { 0xff0b0d10 };
const juce::Colour graphBg     { 0xff0c1014 };
const juce::Colour graphGrid   { 0xff1b232b };
const juce::Colour field       { 0xff15191d };
const juce::Colour track       { 0xff14171b };
const juce::Colour text        { 0xffdfe6ee };
const juce::Colour dimText     { 0xff8d98a4 };

const juce::Colour accent      { 0xff3ea6ff };   // blue: knobs, envelopes, filter
const juce::Colour wave        { 0xffa6e34b };   // lime: oscillator displays
const juce::Colour lfo         { 0xffb38cff };   // violet: LFO
const juce::Colour gold        { 0xffe6b86a };   // title
// Bevelled panel body with a title strip, shared by every section on every tab
inline void drawPanelFrame (juce::Graphics& g, juce::Rectangle<float> b, float headerHeight)
{
    g.setGradientFill (juce::ColourGradient (panelTop, 0.0f, b.getY(), panelBottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);

    const auto h = b.withHeight (headerHeight).reduced (1.0f, 1.0f);
    g.setGradientFill (juce::ColourGradient (headerTop, 0.0f, h.getY(), headerBot, 0.0f, h.getBottom(), false));
    g.fillRoundedRectangle (h, 3.0f);

    g.setColour (edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
}
} // namespace Theme
