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
} // namespace Theme

// Every FX module has its own colour, used for its LED, tag, power button, knobs and graph
// Each section on the OSC tab has its own colour
namespace SectionColours
{
const juce::Colour sub    { 0xffff9a3c };
const juce::Colour noise  { 0xffb8c4d0 };
const juce::Colour oscA   { 0xffa6e34b };
const juce::Colour oscB   { 0xff4fd8c4 };
const juce::Colour filter { 0xff3ea6ff };
const juce::Colour env    { 0xffff6b9a };
const juce::Colour lfo    { 0xffb38cff };
const juce::Colour pump   { 0xfff2d03a };
} // namespace SectionColours

namespace FxColours
{
const juce::Colour distortion { 0xffff5a4e };
const juce::Colour phaser     { 0xffe65bd6 };
const juce::Colour chorus     { 0xff3fd0e0 };
const juce::Colour delay      { 0xff5bd67a };
const juce::Colour compressor { 0xfff2b134 };
const juce::Colour reverb     { 0xff8c7cff };
const juce::Colour eq         { 0xffc6e04a };
} // namespace FxColours

namespace Theme
{
// Subtle brushed-metal grain: horizontal streaks plus fine noise, tiled over surfaces
inline const juce::Image& brushedTexture()
{
    static const juce::Image image = []
    {
        constexpr int size = 256;
        juce::Image im (juce::Image::ARGB, size, size, true);
        juce::Random rng (0x5eed);
        juce::Image::BitmapData px (im, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < size; ++y)
        {
            const float streak = rng.nextFloat() * 2.0f - 1.0f;
            for (int x = 0; x < size; ++x)
            {
                const float n = 0.65f * streak + 0.35f * (rng.nextFloat() * 2.0f - 1.0f);
                const auto alpha = (juce::uint8) juce::jlimit (0, 255, (int) (std::abs (n) * 30.0f));
                px.setPixelColour (x, y, (n > 0.0f ? juce::Colours::white : juce::Colours::black).withAlpha (alpha));
            }
        }
        return im;
    }();
    return image;
}

inline void drawTexture (juce::Graphics& g, juce::Rectangle<float> area, float opacity = 1.0f)
{
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());
    g.setTiledImageFill (brushedTexture(), 0, 0, opacity);
    g.fillRect (area);
}

// Bevelled panel body with a title strip, shared by every section on every tab
inline void drawPanelFrame (juce::Graphics& g, juce::Rectangle<float> b, float headerHeight)
{
    g.setGradientFill (juce::ColourGradient (panelTop, 0.0f, b.getY(), panelBottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 1.5f);
    drawTexture (g, b.reduced (1.0f));

    const auto h = b.withHeight (headerHeight).reduced (1.0f, 1.0f);
    g.setGradientFill (juce::ColourGradient (headerTop, 0.0f, h.getY(), headerBot, 0.0f, h.getBottom(), false));
    g.fillRoundedRectangle (h, 1.5f);
    drawTexture (g, h, 0.8f);

    // Bevel: light top edge, dark bottom edge
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + 1.0f, b.getRight() - 1.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 1.5f, 1.0f);
}
} // namespace Theme
