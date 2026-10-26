#include "Visualizer.h"
#include "Theme.h"

namespace
{
constexpr int refreshHz = 60;
constexpr float minDb = -60.0f, maxDb = 6.0f;
constexpr int headerHeight = 24;
const juce::Colour trace { 0xff8fd3ff };
const juce::Colour peakColour { 0xffe8c547 };

float toDb (float gain) { return juce::Decibels::gainToDecibels (gain, -100.0f); }

juce::String dbText (float gain)
{
    const float db = toDb (gain);
    return db <= -99.0f ? juce::String ("-inf") : juce::String (db, 1);
}

void drawTitle (juce::Graphics& g, juce::Rectangle<int> panel, const juce::String& title)
{
    Theme::drawPanelFrame (g, panel.toFloat(), (float) headerHeight);
    g.setColour (Theme::text);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (title, panel.withHeight (headerHeight).withTrimmedLeft (12), juce::Justification::centredLeft);
}
} // namespace

VisPage::VisPage (ScopeFifo& s) : scope (s)
{
    left.resize (16384);
    right.resize (16384);
}

VisPage::~VisPage()
{
    scope.setEnabled (false);
}

void VisPage::visibilityChanged()
{
    // Only stream audio while someone is looking
    scope.setEnabled (isVisible());
    if (isVisible()) startTimerHz (refreshHz);
    else             stopTimer();
}

void VisPage::resized()
{
    auto r = getLocalBounds();
    gonioPanel = r.removeFromLeft (r.getHeight());
    r.removeFromLeft (8);
    meterPanel = r;

    const auto plotArea = gonioPanel.withTrimmedTop (headerHeight).reduced (14).toFloat();
    const float side = juce::jmin (plotArea.getWidth(), plotArea.getHeight());
    gonioPlot = plotArea.withSizeKeepingCentre (side, side);

    // Trails are drawn at 2x for crisp lines on retina screens
    trails = juce::Image (juce::Image::ARGB, (int) side * 2, (int) side * 2, true);
}

void VisPage::refresh()
{
    const int n = scope.pull (left.data(), right.data(), (int) left.size());
    const float dt = 1.0f / (float) refreshHz;

    double ll = 0, rr = 0, lr = 0, mm = 0, ss = 0;
    float blockPeak[2] {};
    for (int i = 0; i < n; ++i)
    {
        const float l = left[(size_t) i], r = right[(size_t) i];
        ll += l * l;  rr += r * r;  lr += l * r;
        const float m = (l + r) * 0.7071f, s = (l - r) * 0.7071f;
        mm += m * m;  ss += s * s;
        blockPeak[0] = juce::jmax (blockPeak[0], std::abs (l));
        blockPeak[1] = juce::jmax (blockPeak[1], std::abs (r));
    }

    // Meters: ~300 ms RMS, peaks fall at 11.8 dB/s, peak hold for 2 s
    const float rmsCoeff = 1.0f - std::exp (-dt / 0.3f);
    const float peakFall = juce::Decibels::decibelsToGain (-11.8f * dt);
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const double sums[2] = { ll, rr };
    for (int ch = 0; ch < 2; ++ch)
    {
        const float meanSq = n > 0 ? (float) (sums[ch] / n) : 0.0f;
        rmsSq[ch] += rmsCoeff * (meanSq - rmsSq[ch]);
        peak[ch] = juce::jmax (blockPeak[ch], peak[ch] * peakFall);
        if (blockPeak[ch] >= hold[ch] || now - holdTime[ch] > 2.0)
        {
            hold[ch] = blockPeak[ch];
            holdTime[ch] = now;
        }
    }

    midSq += rmsCoeff * ((n > 0 ? (float) (mm / n) : 0.0f) - midSq);
    sideSq += rmsCoeff * ((n > 0 ? (float) (ss / n) : 0.0f) - sideSq);

    // Correlation: +1 mono, 0 fully decorrelated (wide), -1 out of phase
    const float target = (ll > 1e-9 && rr > 1e-9) ? (float) (lr / std::sqrt (ll * rr)) : 0.0f;
    correlation += 0.12f * (target - correlation);

    updateGoniometer (n);
    repaint();
}

void VisPage::updateGoniometer (int n)
{
    if (! trails.isValid())
        return;

    juce::Graphics g (trails);
    const float size = (float) trails.getWidth();

    // Fade the previous frames to leave trails
    g.setColour (Theme::graphBg.withAlpha (0.22f));
    g.fillAll();

    if (n < 2)
        return;

    // Auto-gain so quiet material still fills the scope
    float maxXY = 0.0f;
    for (int i = 0; i < n; ++i)
        maxXY = juce::jmax (maxXY, std::abs (left[(size_t) i]), std::abs (right[(size_t) i]));
    const float target = juce::jlimit (1.0f, 40.0f, 0.6f / juce::jmax (maxXY, 1.0e-4f));
    gonioScale += 0.08f * (target - gonioScale);

    const float c = size * 0.5f, radius = size * 0.5f;
    const int stride = juce::jmax (1, n / 1500);
    juce::Path path;
    for (int i = 0; i < n; i += stride)
    {
        // Rotate 45 degrees: mid goes up, left-only leans left, right-only leans right
        const float l = left[(size_t) i], r = right[(size_t) i];
        const float x = c + (r - l) * 0.7071f * gonioScale * radius;
        const float y = c - (l + r) * 0.7071f * gonioScale * radius;
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
    }
    g.setColour (trace.withAlpha (0.55f));
    g.strokePath (path, juce::PathStrokeType (1.6f));
}

void VisPage::paint (juce::Graphics& g)
{
    paintGoniometer (g);
    paintMeters (g);
}

void VisPage::paintGoniometer (juce::Graphics& g)
{
    drawTitle (g, gonioPanel, "GONIOMETER");
    const auto p = gonioPlot;

    g.setColour (Theme::graphBg);
    g.fillRoundedRectangle (p, 3.0f);

    g.drawImage (trails, p);

    // Guide lines on top of the trails: L and R diagonals, M vertical, S horizontal
    g.setColour (Theme::dimText.withAlpha (0.22f));
    g.drawLine (p.getX(), p.getY(), p.getRight(), p.getBottom(), 1.0f);
    g.drawLine (p.getRight(), p.getY(), p.getX(), p.getBottom(), 1.0f);
    g.drawLine (p.getCentreX(), p.getY(), p.getCentreX(), p.getBottom(), 1.0f);
    g.drawLine (p.getX(), p.getCentreY(), p.getRight(), p.getCentreY(), 0.6f);
    g.drawEllipse (p.reduced (p.getWidth() * 0.25f), 0.8f);

    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("L", juce::Rectangle<float> (p.getX() + 6.0f, p.getY() + 4.0f, 20.0f, 18.0f), juce::Justification::centred);
    g.drawText ("M", juce::Rectangle<float> (p.getCentreX() - 10.0f, p.getY() + 4.0f, 20.0f, 18.0f), juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (p.getRight() - 26.0f, p.getY() + 4.0f, 20.0f, 18.0f), juce::Justification::centred);
    g.drawText ("+S", juce::Rectangle<float> (p.getX() + 4.0f, p.getCentreY() - 20.0f, 24.0f, 16.0f), juce::Justification::centred);
    g.drawText ("-S", juce::Rectangle<float> (p.getRight() - 28.0f, p.getCentreY() - 20.0f, 24.0f, 16.0f), juce::Justification::centred);

    g.setColour (Theme::edge);
    g.drawRoundedRectangle (p, 3.0f, 1.0f);
}

void VisPage::paintMeters (juce::Graphics& g)
{
    drawTitle (g, meterPanel, "WIDTH & LEVELS");
    auto area = meterPanel.withTrimmedTop (headerHeight).reduced (16, 12);

    // --- Stereo width readout
    const float mid = std::sqrt (midSq), side = std::sqrt (sideSq);
    const bool silent = mid + side < 1.0e-4f;
    const float width = silent ? 0.0f : juce::jlimit (0.0f, 200.0f, side / juce::jmax (mid, 1.0e-6f) * 100.0f);
    // Balance: one side 6 dB+ louder means "lopsided", not "wide"
    const float balanceDb = toDb (std::sqrt (rmsSq[0])) - toDb (std::sqrt (rmsSq[1]));
    const char* verdict = silent ? "silence"
                        : correlation < -0.1f ? "out of phase!"
                        : balanceDb > 6.0f ? "leaning left"
                        : balanceDb < -6.0f ? "leaning right"
                        : width < 5.0f ? "mono"
                        : width < 35.0f ? "narrow"
                        : width < 75.0f ? "wide"
                        : "very wide";

    auto top = area.removeFromTop (78);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("STEREO WIDTH  (side / mid)", top.removeFromTop (16), juce::Justification::centredLeft);
    g.setColour (Theme::text);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText (juce::String (juce::roundToInt (width)) + "%", top.removeFromLeft (130), juce::Justification::centredLeft);
    g.setColour (correlation < -0.1f ? juce::Colour (0xffe5675f) : Theme::accent);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (verdict, top, juce::Justification::centredLeft);

    // --- Correlation bar at the bottom
    auto corrArea = area.removeFromBottom (58);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    auto corrRow = corrArea.removeFromTop (16);
    g.drawText ("CORRELATION", corrRow, juce::Justification::centredLeft);
    g.drawText (juce::String (correlation, 2), corrRow, juce::Justification::centredRight);
    auto bar = corrArea.removeFromTop (14).toFloat();
    juce::ColourGradient grad (juce::Colour (0xffe5675f), bar.getX(), 0.0f, juce::Colour (0xff6ccf7a), bar.getRight(), 0.0f, false);
    grad.addColour (0.5, juce::Colour (0xffd9b24a));
    g.setGradientFill (grad);
    g.setOpacity (0.35f);
    g.fillRoundedRectangle (bar, 3.0f);
    g.setOpacity (1.0f);
    const float cx = bar.getX() + (correlation * 0.5f + 0.5f) * bar.getWidth();
    g.setColour (Theme::text);
    g.fillRect (juce::Rectangle<float> (3.0f, bar.getHeight() + 6.0f).withCentre ({ cx, bar.getCentreY() }));
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (10.0f));
    auto labels = corrArea.removeFromTop (16);
    g.drawText ("-1", labels, juce::Justification::centredLeft);
    g.drawText ("0", labels, juce::Justification::centred);
    g.drawText ("+1", labels, juce::Justification::centredRight);

    // --- L/R meters: blue RMS body, yellow up to the peak, line at the peak hold
    area.removeFromBottom (14);
    auto readouts = area.removeFromTop (34);
    auto meters = area.withTrimmedTop (6);
    auto scaleArea = meters.removeFromLeft (34);

    auto yFor = [&] (float gain)
    {
        const float db = juce::jlimit (minDb, maxDb, toDb (gain));
        return (float) meters.getBottom() - (db - minDb) / (maxDb - minDb) * (float) meters.getHeight();
    };

    g.setFont (juce::FontOptions (9.5f));
    for (float db : { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f })
    {
        const float y = yFor (juce::Decibels::decibelsToGain (db));
        g.setColour (db == 0.0f ? Theme::dimText : Theme::graphGrid.brighter (0.2f));
        g.drawHorizontalLine (juce::roundToInt (y), (float) meters.getX(), (float) meters.getRight());
        g.setColour (Theme::dimText);
        g.drawText (juce::String ((int) db), scaleArea.withY ((int) y - 7).withHeight (14), juce::Justification::centredRight);
    }

    const int barW = juce::jmin (46, (meters.getWidth() - 30) / 2);
    const int gap = (meters.getWidth() - 2 * barW) / 3;
    const char* names[] = { "L", "R" };
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto col = juce::Rectangle<float> ((float) (meters.getX() + gap + ch * (barW + gap)), (float) meters.getY(),
                                                 (float) barW, (float) meters.getHeight());
        g.setColour (Theme::graphBg);
        g.fillRect (col);

        const float rms = std::sqrt (rmsSq[ch]);
        const float yRms = yFor (rms), yPeak = yFor (peak[ch]);
        g.setColour (peakColour);
        g.fillRect (col.withTop (yPeak).withBottom (yRms));
        g.setColour (Theme::accent.withAlpha (0.85f));
        g.fillRect (col.withTop (yRms));
        g.setColour (Theme::text);
        g.fillRect (col.withTop (yFor (hold[ch])).withHeight (2.0f));

        // Numbers above each meter: peak hold, then RMS
        auto txt = readouts.withX ((int) col.getX() - 10).withWidth ((int) col.getWidth() + 20);
        g.setColour (Theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (names[ch], txt.removeFromTop (12), juce::Justification::centred);
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (peakColour);
        g.drawText (dbText (hold[ch]), txt.removeFromTop (11), juce::Justification::centred);
        g.setColour (Theme::accent);
        g.drawText (dbText (rms), txt.removeFromTop (11), juce::Justification::centred);
    }
}
