#include "Graphs.h"
#include "Theme.h"
#include "FxChain.h"
#include <complex>

namespace
{
float waveValue (int wave, float t)
{
    switch (wave)
    {
        case 0:  return 2.0f * t - 1.0f;
        case 1:  return t < 0.5f ? 1.0f : -1.0f;
        case 2:  return 1.0f - 4.0f * std::abs (t - 0.5f);
        default: return std::sin (juce::MathConstants<float>::twoPi * t);
    }
}

float lfoValue (int shape, float phase, int cycle)
{
    switch (shape)
    {
        case 0: return std::sin (juce::MathConstants<float>::twoPi * phase);
        case 1: return 1.0f - 4.0f * std::abs (phase - 0.5f);
        case 2: return 2.0f * phase - 1.0f;
        case 3: return phase < 0.5f ? 1.0f : -1.0f;
        default: {
            // Deterministic "random" steps so the S&H preview stays still
            auto h = (uint32_t) cycle * 2654435761u;
            h ^= h >> 15;
            return (float) (h & 0xffff) / 32767.5f - 1.0f;
        }
    }
}

void fillUnder (juce::Graphics& g, const juce::Path& curve, juce::Rectangle<float> p, float baseY, juce::Colour c)
{
    juce::Path fill (curve);
    fill.lineTo (p.getRight(), baseY);
    fill.lineTo (p.getX(), baseY);
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.38f), 0.0f, p.getY(),
                                             c.withAlpha (0.04f), 0.0f, p.getBottom(), false));
    g.fillPath (fill);
}
} // namespace

//==============================================================================
void GraphBase::drawBackground (juce::Graphics& g, int verticalLines, int horizontalLines)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::graphBg);
    g.fillRoundedRectangle (b, 1.5f);

    const auto p = plot();
    g.setColour (Theme::graphGrid);
    for (int i = 1; i < verticalLines; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / (float) verticalLines;
        g.drawVerticalLine (juce::roundToInt (x), p.getY(), p.getBottom());
    }
    for (int i = 1; i < horizontalLines; ++i)
    {
        const float y = p.getY() + p.getHeight() * (float) i / (float) horizontalLines;
        g.drawHorizontalLine (juce::roundToInt (y), p.getX(), p.getRight());
    }

    g.setColour (Theme::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 1.5f, 1.0f);
}

void GraphBase::drawReadout (juce::Graphics& g)
{
    if (readout.isEmpty())
        return;

    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    const auto r = getLocalBounds().reduced (8, 6).removeFromTop (16);
    g.setColour (Theme::text);
    g.drawText (readout, r, juce::Justification::topRight);
}

void GraphBase::drawPlayhead (juce::Graphics& g, juce::Point<float> pt, juce::Colour c)
{
    const auto p = plot();
    g.setColour (c.withAlpha (0.18f));
    g.drawVerticalLine (juce::roundToInt (pt.x), p.getY(), p.getBottom());
    for (float r : { 11.0f, 7.0f })
    {
        g.setColour (c.withAlpha (r > 8.0f ? 0.15f : 0.35f));
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt));
    }
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (pt));
}

void GraphBase::drawHandle (juce::Graphics& g, juce::Point<float> pt, juce::Colour c, bool active)
{
    const float r = active ? 6.0f : 4.0f;
    if (active)
    {
        g.setColour (c.withAlpha (0.3f));
        g.fillEllipse (juce::Rectangle<float> (r * 3.0f, r * 3.0f).withCentre (pt));
    }
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt));
    g.setColour (c);
    g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (pt), 1.5f);
}

//==============================================================================
EnvelopeGraph::EnvelopeGraph (juce::AudioProcessorValueTreeState& s, const juce::String& a, const juce::String& d,
                              const juce::String& su, const juce::String& r, juce::Colour c, bool amp)
    : attack (s, a, [this] { repaint(); }),
      decay (s, d, [this] { repaint(); }),
      sustain (s, su, [this] { repaint(); }),
      release (s, r, [this] { repaint(); }),
      isAmp (amp)
{
    accent = c;
    setRepaintsOnMouseActivity (false);
}

EnvelopeGraph::Points EnvelopeGraph::points() const
{
    const auto p = plot();
    Points pts;
    pts.segment = p.getWidth() * 0.27f;
    const float hold = p.getWidth() * 0.19f;

    pts.start      = { p.getX(), p.getBottom() };
    pts.peak       = { p.getX() + attack.norm() * pts.segment, p.getY() };
    pts.sustain    = { pts.peak.x + decay.norm() * pts.segment, p.getBottom() - sustain.get() * p.getHeight() };
    pts.sustainEnd = { pts.sustain.x + hold, pts.sustain.y };
    pts.end        = { pts.sustainEnd.x + release.norm() * pts.segment, p.getBottom() };
    return pts;
}

int EnvelopeGraph::handleAt (juce::Point<float> pos) const
{
    const auto pts = points();
    const juce::Point<float> handles[] = { pts.peak, pts.sustain, pts.end };

    int best = 0;
    float bestDist = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i)
    {
        const float d = handles[i].getDistanceFrom (pos);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

void EnvelopeGraph::updateReadout()
{
    switch (dragging)
    {
        case 0:  readout = "ATTACK  " + attack.text(); break;
        case 1:  readout = "DECAY  " + decay.text() + "   SUSTAIN  " + sustain.text(); break;
        case 2:  readout = "RELEASE  " + release.text(); break;
        default: readout = {};
    }
}

void EnvelopeGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 8, 4);
    const auto p = plot();
    const auto pts = points();

    juce::Path curve;
    curve.startNewSubPath (pts.start);
    curve.lineTo (pts.peak);
    curve.lineTo (pts.sustain);
    curve.lineTo (pts.sustainEnd);
    curve.lineTo (pts.end);
    curve.lineTo (p.getRight(), p.getBottom());

    fillUnder (g, curve, p, p.getBottom(), accent);
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Note-off marker
    g.setColour (Theme::dimText.withAlpha (0.4f));
    const float dashes[] = { 3.0f, 3.0f };
    g.drawDashedLine ({ pts.sustainEnd.x, p.getY(), pts.sustainEnd.x, p.getBottom() }, dashes, 2);

    const juce::Point<float> handles[] = { pts.peak, pts.sustain, pts.end };
    for (int i = 0; i < 3; ++i)
        drawHandle (g, handles[i], accent, i == dragging || (dragging < 0 && i == hover));

    // Playhead: x follows the stage timing, y is the envelope's real level
    if (monitor != nullptr && monitor->active.load())
    {
        const float level = isAmp ? monitor->ampEnv.load() : monitor->filterEnv.load();
        const float t = monitor->noteTime.load(), tr = monitor->releaseTime.load();
        const float a = attack.get(), d = decay.get();
        float x;

        if (tr >= 0.0f)
            x = pts.sustainEnd.x + juce::jmin (1.0f, tr / release.get()) * (pts.end.x - pts.sustainEnd.x);
        else if (t < a)
            x = pts.start.x + (t / a) * (pts.peak.x - pts.start.x);
        else if (t < a + d)
            x = pts.peak.x + ((t - a) / d) * (pts.sustain.x - pts.peak.x);
        else
            x = pts.sustain.x;

        drawPlayhead (g, { x, p.getBottom() - level * p.getHeight() }, accent);
    }

    drawReadout (g);
}

void EnvelopeGraph::mouseMove (const juce::MouseEvent& e)
{
    const int h = handleAt (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void EnvelopeGraph::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void EnvelopeGraph::mouseDown (const juce::MouseEvent& e)
{
    dragging = handleAt (e.position);
    if (dragging == 0) attack.begin();
    if (dragging == 1) { decay.begin(); sustain.begin(); }
    if (dragging == 2) release.begin();
    mouseDrag (e);
}

void EnvelopeGraph::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = plot();
    const auto pts = points();

    if (dragging == 0)
        attack.setNorm ((e.position.x - pts.start.x) / pts.segment);
    else if (dragging == 1)
    {
        decay.setNorm ((e.position.x - pts.peak.x) / pts.segment);
        sustain.set (juce::jlimit (0.0f, 1.0f, (p.getBottom() - e.position.y) / p.getHeight()));
    }
    else if (dragging == 2)
        release.setNorm ((e.position.x - pts.sustainEnd.x) / pts.segment);

    updateReadout();
    repaint();
}

void EnvelopeGraph::mouseUp (const juce::MouseEvent&)
{
    if (dragging == 0) attack.end();
    if (dragging == 1) { decay.end(); sustain.end(); }
    if (dragging == 2) release.end();
    dragging = -1;
    readout = {};
    repaint();
}

void EnvelopeGraph::mouseDoubleClick (const juce::MouseEvent&)
{
    attack.reset();
    decay.reset();
    sustain.reset();
    release.reset();
}

//==============================================================================
FilterGraph::FilterGraph (juce::AudioProcessorValueTreeState& s)
    : type (s, "filterType", [this] { repaint(); }),
      cutoff (s, "cutoff", [this] { repaint(); }),
      reso (s, "reso", [this] { repaint(); }),
      lowCut (s, "lowCut", [this] { repaint(); })
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

float FilterGraph::magnitudeDb (float freq) const
{
    // Analog prototype of the SVF used in the voice
    const float w = freq / cutoff.get();
    const float k = 2.0f - 1.96f * reso.get();
    const float w2 = w * w;
    const float den = std::sqrt ((1.0f - w2) * (1.0f - w2) + (k * w) * (k * w));

    float mag;
    switch ((int) type.get())
    {
        case 0:  mag = 1.0f / den; break;
        case 1:  mag = 1.0f / den / std::sqrt ((1.0f - w2) * (1.0f - w2) + 2.0f * w2); break;
        case 2:  mag = k * w / den; break;
        default: mag = w2 / den; break;
    }
    // Global 12 dB/oct low cut that follows the filter
    const float h = lowCut.get() / freq;
    mag /= std::sqrt (1.0f + h * h * h * h);

    return juce::Decibels::gainToDecibels (mag, -100.0f);
}

float FilterGraph::freqToX (float f) const
{
    const auto p = plot();
    return p.getX() + std::log (f / 20.0f) / std::log (1000.0f) * p.getWidth();
}

float FilterGraph::dbToY (float db) const
{
    const auto p = plot();
    return juce::jmap (juce::jlimit (-48.0f, 30.0f, db), -48.0f, 30.0f, p.getBottom(), p.getY());
}

void FilterGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 1, 1);
    const auto p = plot();

    g.setFont (juce::FontOptions (9.5f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = freqToX (f);
        g.setColour (Theme::graphGrid);
        g.drawVerticalLine (juce::roundToInt (x), p.getY(), p.getBottom());
        g.setColour (Theme::dimText.withAlpha (0.5f));
        g.drawText (f < 1000.0f ? "100" : (f < 10000.0f ? "1k" : "10k"),
                    juce::Rectangle<float> (x + 3.0f, p.getBottom() - 12.0f, 30.0f, 12.0f), juce::Justification::left);
    }
    g.setColour (Theme::graphGrid.brighter (0.2f));
    g.drawHorizontalLine (juce::roundToInt (dbToY (0.0f)), p.getX(), p.getRight());

    juce::Path curve;
    const int steps = juce::jmax (2, (int) p.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / (float) steps;
        const float f = 20.0f * std::pow (1000.0f, (float) i / (float) steps);
        const float y = dbToY (magnitudeDb (f));
        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    fillUnder (g, curve, p, p.getBottom(), accent);
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    const float fc = cutoff.get();
    drawHandle (g, { freqToX (fc), dbToY (magnitudeDb (fc)) }, accent, isMouseButtonDown());
    drawReadout (g);
}

void FilterGraph::mouseDown (const juce::MouseEvent& e)
{
    dragStartReso = reso.get();
    cutoff.begin();
    reso.begin();
    mouseDrag (e);
}

void FilterGraph::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = plot();
    const float t = juce::jlimit (0.0f, 1.0f, (e.position.x - p.getX()) / p.getWidth());
    cutoff.set (20.0f * std::pow (1000.0f, t));
    reso.set (juce::jlimit (0.0f, 1.0f, dragStartReso - (float) e.getDistanceFromDragStartY() / (p.getHeight() * 1.2f)));
    readout = "CUTOFF  " + cutoff.text() + "   RES  " + reso.text();
    repaint();
}

void FilterGraph::mouseUp (const juce::MouseEvent&)
{
    cutoff.end();
    reso.end();
    readout = {};
    repaint();
}

void FilterGraph::mouseDoubleClick (const juce::MouseEvent&)
{
    cutoff.reset();
    reso.reset();
}

void FilterGraph::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    reso.begin();
    reso.set (juce::jlimit (0.0f, 1.0f, reso.get() + w.deltaY * 0.25f));
    reso.end();
}

//==============================================================================
OscGraph::OscGraph (juce::AudioProcessorValueTreeState& s, const juce::String& prefix)
    : wave (s, prefix + "Wave", [this] { repaint(); }),
      voices (s, prefix + "Voices", [this] { repaint(); }),
      detune (s, prefix + "Detune", [this] { repaint(); }),
      level (s, prefix + "Level", [this] { repaint(); })
{
    accent = Theme::wave;
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void OscGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 4, 2);
    const auto p = plot();
    const int w = (int) wave.get();
    const int n = (int) voices.get();
    const float amp = p.getHeight() * 0.44f * juce::jmax (0.08f, level.get());
    const float mid = p.getCentreY();
    const int steps = juce::jmax (2, (int) p.getWidth() * 2);

    // Two cycles; each unison voice drawn phase-shifted by its share of the detune
    auto makePath = [&] (float phaseOffset)
    {
        juce::Path path;
        for (int i = 0; i <= steps; ++i)
        {
            const float x = p.getX() + p.getWidth() * (float) i / (float) steps;
            float t = 2.0f * (float) i / (float) steps + phaseOffset;
            t -= std::floor (t);
            const float y = mid - waveValue (w, t) * amp;
            if (i == 0) path.startNewSubPath (x, y);
            else        path.lineTo (x, y);
        }
        return path;
    };

    for (int u = 0; u < n; ++u)
    {
        const float spread = n == 1 ? 0.0f : 2.0f * (float) u / (float) (n - 1) - 1.0f;
        if (std::abs (spread) < 0.001f)
            continue;
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (makePath (spread * detune.get() * 0.18f), juce::PathStrokeType (1.2f));
    }

    const auto main = makePath (0.0f);
    fillUnder (g, main, p, mid, accent.withAlpha (0.6f));
    g.setColour (accent);
    g.strokePath (main, juce::PathStrokeType (2.0f));

    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.setColour (Theme::dimText);
    g.drawText ("x" + juce::String (n), getLocalBounds().reduced (8, 6).removeFromBottom (14),
                juce::Justification::bottomRight);
    drawReadout (g);
}

void OscGraph::mouseDown (const juce::MouseEvent&)
{
    dragStart = detune.get();
    detune.begin();
}

void OscGraph::mouseDrag (const juce::MouseEvent& e)
{
    detune.set (juce::jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / 150.0f));
    readout = "DETUNE  " + detune.text();
    repaint();
}

void OscGraph::mouseUp (const juce::MouseEvent&)
{
    detune.end();
    readout = {};
    repaint();
}

void OscGraph::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    detune.begin();
    detune.set (juce::jlimit (0.0f, 1.0f, detune.get() + w.deltaY * 0.2f));
    detune.end();
}

//==============================================================================
LfoGraph::LfoGraph (juce::AudioProcessorValueTreeState& s)
    : shape (s, "lfoShape", [this] { repaint(); }),
      rate (s, "lfoRate", [this] { repaint(); }),
      delay (s, "lfoDelay", [this] { repaint(); })
{
    accent = Theme::lfo;
}

juce::Point<float> LfoGraph::delayHandle() const
{
    const auto p = plot();
    const float x = p.getX() + juce::jmin (1.0f, delay.get() / windowSeconds) * p.getWidth();
    return { x, p.getY() };
}

void LfoGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 6, 2);
    const auto p = plot();
    const float mid = p.getCentreY();
    const float amp = p.getHeight() * 0.48f;
    const int s = (int) shape.get();
    const float r = rate.get(), d = delay.get();

    g.setFont (juce::FontOptions (9.5f));
    g.setColour (Theme::dimText.withAlpha (0.5f));
    for (int sec = 1; sec < (int) windowSeconds; ++sec)
        g.drawText (juce::String (sec) + " s",
                    juce::Rectangle<float> (p.getX() + p.getWidth() * (float) sec / windowSeconds + 3.0f,
                                            p.getBottom() - 12.0f, 30.0f, 12.0f), juce::Justification::left);

    juce::Path curve, fadeTop, fadeBottom;
    const int steps = juce::jmax (2, (int) p.getWidth() * 3);
    for (int i = 0; i <= steps; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / (float) steps;
        const float t = windowSeconds * (float) i / (float) steps;
        const float cycles = t * r;
        const float fade = d > 0.0f ? juce::jmin (1.0f, t / d) : 1.0f;
        const float y = mid - lfoValue (s, cycles - std::floor (cycles), (int) cycles) * fade * amp;

        if (i == 0)
        {
            curve.startNewSubPath (x, y);
            fadeTop.startNewSubPath (x, mid - fade * amp);
            fadeBottom.startNewSubPath (x, mid + fade * amp);
        }
        else
        {
            curve.lineTo (x, y);
            fadeTop.lineTo (x, mid - fade * amp);
            fadeBottom.lineTo (x, mid + fade * amp);
        }
    }

    fillUnder (g, curve, p, mid, accent);
    g.setColour (accent.withAlpha (0.35f));
    const float dashes[] = { 3.0f, 3.0f };
    juce::PathStrokeType dashStroke (1.0f);
    juce::Path dashed;
    dashStroke.createDashedStroke (dashed, fadeTop, dashes, 2);
    g.fillPath (dashed);
    dashStroke.createDashedStroke (dashed, fadeBottom, dashes, 2);
    g.fillPath (dashed);

    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    drawHandle (g, delayHandle(), accent, hoverHandle || draggingDelay);

    // Playhead: wrap time back into the drawn window once the fade-in is over,
    // since the curve is periodic from there on
    if (monitor != nullptr && monitor->active.load())
    {
        float t = monitor->noteTime.load();
        const float period = 1.0f / r;
        if (t > windowSeconds)
        {
            const float settled = juce::jmax (d, 0.0f);
            const float cyclesBefore = std::ceil (settled / period);  // start of a whole cycle after the fade
            const float loopStart = cyclesBefore * period;
            t = loopStart + period <= windowSeconds ? loopStart + std::fmod (t - loopStart, period)
                                                    : windowSeconds;
        }
        const float x = p.getX() + t / windowSeconds * p.getWidth();
        drawPlayhead (g, { x, mid - monitor->lfoValue.load() * amp }, accent);
    }
    drawReadout (g);
}

void LfoGraph::mouseMove (const juce::MouseEvent& e)
{
    const bool h = std::abs (e.position.x - delayHandle().x) < 10.0f;
    if (h != hoverHandle) { hoverHandle = h; repaint(); }
    setMouseCursor (h ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
}

void LfoGraph::mouseExit (const juce::MouseEvent&)
{
    hoverHandle = false;
    repaint();
}

void LfoGraph::mouseDown (const juce::MouseEvent& e)
{
    draggingDelay = std::abs (e.position.x - delayHandle().x) < 10.0f;
    draggingRate = ! draggingDelay;
    dragStart = rate.norm();
    (draggingDelay ? delay : rate).begin();
}

void LfoGraph::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = plot();
    if (draggingDelay)
    {
        delay.set (juce::jlimit (0.0f, windowSeconds, (e.position.x - p.getX()) / p.getWidth() * windowSeconds));
        readout = "DELAY  " + delay.text();
    }
    else
    {
        rate.setNorm (dragStart - (float) e.getDistanceFromDragStartY() / 200.0f);
        readout = "RATE  " + rate.text();
    }
    repaint();
}

void LfoGraph::mouseUp (const juce::MouseEvent&)
{
    if (draggingDelay) delay.end();
    if (draggingRate)  rate.end();
    draggingDelay = draggingRate = false;
    readout = {};
    repaint();
}

void LfoGraph::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    rate.begin();
    rate.setNorm (rate.norm() + w.deltaY * 0.15f);
    rate.end();
}

//==============================================================================
DistGraph::DistGraph (juce::AudioProcessorValueTreeState& s)
    : type (s, "distType", [this] { repaint(); }),
      drive (s, "distDrive", [this] { repaint(); })
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void DistGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 2, 2);
    const auto p = plot();
    const int t = (int) type.get();

    // Unity line for reference, then the shaper's in > out curve
    g.setColour (Theme::dimText.withAlpha (0.25f));
    g.drawLine (p.getX(), p.getBottom(), p.getRight(), p.getY(), 1.0f);

    juce::Path curve;
    const int steps = juce::jmax (2, (int) p.getWidth() * 2);
    for (int i = 0; i <= steps; ++i)
    {
        const float in = -1.0f + 2.0f * (float) i / (float) steps;
        const float out = juce::jlimit (-1.2f, 1.2f, Dist::shape (t, in, drive.get()));
        const float x = p.getX() + p.getWidth() * (float) i / (float) steps;
        const float y = p.getCentreY() - out * p.getHeight() * 0.42f;
        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));
    drawReadout (g);
}

void DistGraph::mouseDown (const juce::MouseEvent&)
{
    dragStart = drive.get();
    drive.begin();
}

void DistGraph::mouseDrag (const juce::MouseEvent& e)
{
    drive.set (juce::jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / 150.0f));
    readout = "DRIVE  " + drive.text();
    repaint();
}

void DistGraph::mouseUp (const juce::MouseEvent&)
{
    drive.end();
    readout = {};
    repaint();
}

//==============================================================================
EqGraph::EqGraph (juce::AudioProcessorValueTreeState& s)
    : low (s, "eqLow", [this] { repaint(); }),
      mid (s, "eqMid", [this] { repaint(); }),
      midFreq (s, "eqMidFreq", [this] { repaint(); }),
      high (s, "eqHigh", [this] { repaint(); })
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

float EqGraph::responseDb (float freq) const
{
    // Same biquads as the audio path, evaluated at 48 kHz
    constexpr double sr = 48000.0;
    using AC = juce::dsp::IIR::ArrayCoefficients<float>;
    auto gain = [] (float db) { return juce::Decibels::decibelsToGain (db); };
    const std::array<float, 6> bands[] = { AC::makeLowShelf (sr, 200.0, 0.707f, gain (low.get())),
                                           AC::makePeakFilter (sr, midFreq.get(), 1.0f, gain (mid.get())),
                                           AC::makeHighShelf (sr, 6000.0, 0.707f, gain (high.get())) };
    const std::complex<double> z = std::polar (1.0, juce::MathConstants<double>::twoPi * freq / sr);
    double mag = 1.0;
    for (const auto& c : bands)
    {
        const auto num = (double) c[0] + (double) c[1] / z + (double) c[2] / (z * z);
        const auto den = (double) c[3] + (double) c[4] / z + (double) c[5] / (z * z);
        mag *= std::abs (num) / std::abs (den);
    }
    return (float) juce::Decibels::gainToDecibels (mag, -100.0);
}

float EqGraph::freqToX (float f) const
{
    const auto p = plot();
    return p.getX() + std::log (f / 20.0f) / std::log (1000.0f) * p.getWidth();
}

float EqGraph::dbToY (float db) const
{
    const auto p = plot();
    return juce::jmap (juce::jlimit (-15.0f, 15.0f, db), -15.0f, 15.0f, p.getBottom(), p.getY());
}

void EqGraph::paint (juce::Graphics& g)
{
    drawBackground (g, 1, 2);
    const auto p = plot();

    g.setFont (juce::FontOptions (9.5f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = freqToX (f);
        g.setColour (Theme::graphGrid);
        g.drawVerticalLine (juce::roundToInt (x), p.getY(), p.getBottom());
        g.setColour (Theme::dimText.withAlpha (0.5f));
        g.drawText (f < 1000.0f ? "100" : (f < 10000.0f ? "1k" : "10k"),
                    juce::Rectangle<float> (x + 3.0f, p.getBottom() - 12.0f, 30.0f, 12.0f), juce::Justification::left);
    }

    juce::Path curve;
    const int steps = juce::jmax (2, (int) p.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float x = p.getX() + p.getWidth() * (float) i / (float) steps;
        const float y = dbToY (responseDb (20.0f * std::pow (1000.0f, (float) i / (float) steps)));
        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    juce::Path fill (curve);
    fill.lineTo (p.getRight(), dbToY (0.0f));
    fill.lineTo (p.getX(), dbToY (0.0f));
    fill.closeSubPath();
    g.setColour (accent.withAlpha (0.18f));
    g.fillPath (fill);
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    drawHandle (g, { freqToX (midFreq.get()), dbToY (mid.get()) }, accent, isMouseButtonDown());
    drawReadout (g);
}

void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    mid.begin();
    midFreq.begin();
    mouseDrag (e);
}

void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = plot();
    const float t = juce::jlimit (0.0f, 1.0f, (e.position.x - p.getX()) / p.getWidth());
    midFreq.set (20.0f * std::pow (1000.0f, t));
    mid.set (juce::jmap (juce::jlimit (p.getY(), p.getBottom(), e.position.y), p.getBottom(), p.getY(), -15.0f, 15.0f));
    readout = "MID  " + mid.text() + "  @ " + midFreq.text();
    repaint();
}

void EqGraph::mouseUp (const juce::MouseEvent&)
{
    mid.end();
    midFreq.end();
    readout = {};
    repaint();
}

void EqGraph::mouseDoubleClick (const juce::MouseEvent&)
{
    low.reset();
    mid.reset();
    midFreq.reset();
    high.reset();
}
