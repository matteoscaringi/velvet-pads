#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
// Deep blue-black HUD palette with a cyan accent
const juce::Colour background  { 0xff070a0f };
const juce::Colour panelTop    { 0xff131a23 };
const juce::Colour panelBottom { 0xff0d1219 };
const juce::Colour headerTop   { 0xff19222d };
const juce::Colour headerBot   { 0xff121921 };
const juce::Colour edge        { 0xff03050a };
const juce::Colour graphBg     { 0xff05080c };
const juce::Colour graphGrid   { 0xff12202b };
const juce::Colour field       { 0xff080c12 };
const juce::Colour track       { 0xff18212b };
const juce::Colour text        { 0xffe4f1ff };
const juce::Colour dimText     { 0xff71849a };

const juce::Colour accent      { 0xff2fd6ff };   // cyan: global accent
const juce::Colour accent2     { 0xffff4fd8 };   // magenta: title gradient
const juce::Colour wave        { 0xffa6e34b };
const juce::Colour lfo         { 0xffb38cff };
const juce::Colour gold        { 0xffe6b86a };
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
// Condensed sans for the HUD look (falls back to the system font if missing)
inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions ("Avenir Next Condensed", size, bold ? juce::Font::bold : juce::Font::plain))
               .withExtraKerningFactor (0.04f);
}

// Fine tech grid with a little noise, tiled over surfaces
inline const juce::Image& surfaceTexture()
{
    static const juce::Image image = []
    {
        constexpr int size = 192;
        juce::Image im (juce::Image::ARGB, size, size, true);
        juce::Random rng (0x5eed);
        juce::Image::BitmapData px (im, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const bool grid = (x % 12 == 0) || (y % 12 == 0);
                const bool scan = (y % 3 == 0);
                const float noise = rng.nextFloat();
                const float a = (grid ? 0.05f : 0.0f) + (scan ? 0.025f : 0.0f) + noise * 0.035f;
                const auto c = grid ? juce::Colour (0xff6fd8ff) : (noise > 0.5f ? juce::Colours::white : juce::Colours::black);
                px.setPixelColour (x, y, c.withAlpha (a));
            }
        return im;
    }();
    return image;
}

inline void drawTexture (juce::Graphics& g, juce::Rectangle<float> area, float opacity = 1.0f)
{
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());
    g.setTiledImageFill (surfaceTexture(), 0, 0, opacity);
    g.fillRect (area);
}

// Rectangle with the top-left and bottom-right corners cut off
inline juce::Path chamfered (juce::Rectangle<float> r, float cut)
{
    juce::Path p;
    p.startNewSubPath (r.getX() + cut, r.getY());
    p.lineTo (r.getRight(), r.getY());
    p.lineTo (r.getRight(), r.getBottom() - cut);
    p.lineTo (r.getRight() - cut, r.getBottom());
    p.lineTo (r.getX(), r.getBottom());
    p.lineTo (r.getX(), r.getY() + cut);
    p.closeSubPath();
    return p;
}

// Neon line: wide faint passes underneath, sharp line on top
inline void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour c, float width)
{
    g.setColour (c.withAlpha (0.10f));
    g.strokePath (path, juce::PathStrokeType (width * 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.22f));
    g.strokePath (path, juce::PathStrokeType (width * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// HUD panel: chamfered body, grid texture, header strip with a glowing rule, corner brackets
inline void drawPanelFrame (juce::Graphics& g, juce::Rectangle<float> b, float headerHeight, juce::Colour c = accent)
{
    constexpr float cut = 10.0f;
    const auto body = chamfered (b.reduced (0.5f), cut);

    g.setGradientFill (juce::ColourGradient (panelTop, 0.0f, b.getY(), panelBottom, 0.0f, b.getBottom(), false));
    g.fillPath (body);
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (body);
        drawTexture (g, b);

        if (headerHeight > 0.0f)
        {
            const auto h = b.withHeight (headerHeight);
            g.setGradientFill (juce::ColourGradient (headerTop, 0.0f, h.getY(), headerBot, 0.0f, h.getBottom(), false));
            g.fillRect (h);
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.10f), h.getX(), 0.0f, juce::Colours::transparentBlack, h.getRight(), 0.0f, false));
            g.fillRect (h);
            // Glowing rule under the header, fading out to the right
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.8f), h.getX(), 0.0f, c.withAlpha (0.05f), h.getRight(), 0.0f, false));
            g.fillRect (h.getX(), h.getBottom() - 1.0f, h.getWidth(), 1.0f);
        }
    }

    g.setColour (c.withAlpha (0.18f));
    g.strokePath (body, juce::PathStrokeType (1.0f));

    // Corner brackets on the two square corners
    juce::Path brackets;
    constexpr float len = 12.0f;
    brackets.startNewSubPath (b.getRight() - len, b.getY() + 0.5f);
    brackets.lineTo (b.getRight() - 0.5f, b.getY() + 0.5f);
    brackets.lineTo (b.getRight() - 0.5f, b.getY() + len);
    brackets.startNewSubPath (b.getX() + 0.5f, b.getBottom() - len);
    brackets.lineTo (b.getX() + 0.5f, b.getBottom() - 0.5f);
    brackets.lineTo (b.getX() + len, b.getBottom() - 0.5f);
    strokeGlow (g, brackets, c.withAlpha (0.85f), 1.2f);
}
} // namespace Theme
