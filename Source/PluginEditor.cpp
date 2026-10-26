#include "PluginEditor.h"

//==============================================================================
VelvetLookAndFeel::VelvetLookAndFeel()
{
    setDefaultSansSerifTypefaceName ("Avenir Next Condensed");   // condensed HUD font everywhere
    setColour (juce::Label::textColourId, Theme::text);
    setColour (juce::ComboBox::backgroundColourId, Theme::field);
    setColour (juce::ComboBox::textColourId, Theme::accent.brighter (0.3f));
    setColour (juce::ComboBox::outlineColourId, Theme::edge);
    setColour (juce::PopupMenu::backgroundColourId, Theme::panelBottom);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);
    setColour (juce::BubbleComponent::backgroundColourId, Theme::graphBg);
    setColour (juce::BubbleComponent::outlineColourId, Theme::accent.withAlpha (0.6f));
    setColour (juce::TooltipWindow::textColourId, Theme::text);
    setColour (juce::Slider::rotarySliderFillColourId, Theme::accent);
    setColour (juce::ToggleButton::tickColourId, Theme::accent);
    setColour (juce::TextButton::buttonOnColourId, Theme::accent);
}

void VelvetLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                        float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

    // Number-box style slider (OCT / SEM / FIN / UNISON)
    if (slider.getProperties()["field"])
    {
        const auto r = bounds.reduced (0.5f);
        g.setColour (Theme::field);
        g.fillRoundedRectangle (r, 1.5f);
        g.setColour (slider.isMouseOverOrDragging() ? slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (0.6f) : Theme::edge);
        g.drawRoundedRectangle (r, 1.5f, 1.0f);
        g.setColour (Theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (slider.getTextFromValue (slider.getValue()), r, juce::Justification::centred);
        return;
    }

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (2.0f, radius * 0.10f);
    const float arcR = radius - lineW;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // Faint tick ring outside the arc
    g.setColour (colour.withAlpha (0.18f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const juce::Point<float> p1 (centre.x + std::sin (a) * (radius + 0.5f), centre.y - std::cos (a) * (radius + 0.5f));
        const juce::Point<float> p2 (centre.x + std::sin (a) * (radius + 2.5f), centre.y - std::cos (a) * (radius + 2.5f));
        g.drawLine ({ p1, p2 }, 1.0f);
    }

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Theme::track);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    // Neon value arc
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        Theme::strokeGlow (g, value, colour, lineW);
    }

    // Dark glass cap with a thin rim
    const float capR = arcR - lineW * 1.6f;
    const auto cap = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222c38), centre.x, cap.getY(),
                                             juce::Colour (0xff080b10), centre.x, cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (cap.reduced (0.5f), 1.0f);

    // Indicator: short neon line ending in a glowing dot
    const juce::Point<float> tip (centre.x + std::sin (angle) * capR * 0.78f, centre.y - std::cos (angle) * capR * 0.78f);
    const juce::Point<float> base (centre.x + std::sin (angle) * capR * 0.30f, centre.y - std::cos (angle) * capR * 0.30f);
    juce::Path pointer;
    pointer.startNewSubPath (base);
    pointer.lineTo (tip);
    Theme::strokeGlow (g, pointer, colour.interpolatedWith (juce::Colours::white, 0.4f), 1.6f);
}

void VelvetLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<int> (width, height).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 1.5f);
    g.setColour (box.isMouseOver (true) ? box.findColour (juce::ComboBox::textColourId).withAlpha (0.6f) : Theme::edge);
    g.drawRoundedRectangle (r, 1.5f, 1.0f);
}

void VelvetLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (box.getLocalBounds());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centred);
}

void VelvetLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    if (b.getToggleState())
    {
        const auto c = b.findColour (juce::TextButton::buttonOnColourId);
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.0f), 0.0f, r.getY(), c.withAlpha (0.18f), 0.0f, r.getBottom(), false));
        g.fillRect (r);
        juce::Path line;
        line.startNewSubPath (r.getX() + 4.0f, r.getBottom() - 1.5f);
        line.lineTo (r.getRight() - 4.0f, r.getBottom() - 1.5f);
        Theme::strokeGlow (g, line, c, 2.0f);
    }
    else if (over)
    {
        g.setColour (Theme::headerTop.withAlpha (0.6f));
        g.fillRect (r);
    }
}

void VelvetLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.setColour (b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId).interpolatedWith (Theme::text, 0.5f) : Theme::dimText);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void VelvetLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const bool on = b.getToggleState();
    const auto colour = b.findColour (juce::ToggleButton::tickColourId);

    if (b.getProperties()["led"])   // pill LED in the FX module list
    {
        const auto r = b.getLocalBounds().toFloat();
        g.setColour (Theme::edge);
        g.fillRoundedRectangle (r, 1.5f);
        if (on)
        {
            g.setColour (colour.withAlpha (0.35f));
            g.fillRoundedRectangle (r.expanded (2.0f), 1.5f);
            g.setGradientFill (juce::ColourGradient (colour.brighter (0.5f), r.getCentreX(), r.getY(),
                                                     colour, r.getCentreX(), r.getBottom(), false));
            g.fillRoundedRectangle (r.reduced (2.0f), 1.0f);
        }
        else
        {
            g.setColour (over ? Theme::track.brighter (0.3f) : Theme::track);
            g.fillRoundedRectangle (r.reduced (2.0f), 1.0f);
        }
        return;
    }

    if (b.getProperties()["powerIcon"])   // power button on a rack strip
    {
        const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (on ? colour : Theme::field);
        g.fillRoundedRectangle (r, 1.5f);
        g.setColour (on ? Theme::edge.withAlpha (0.4f) : (over ? colour.withAlpha (0.7f) : Theme::edge));
        g.drawRoundedRectangle (r, 1.5f, 1.0f);

        const auto c = r.getCentre();
        const float rad = r.getWidth() * 0.25f;
        juce::Path icon;
        icon.addCentredArc (c.x, c.y + 1.0f, rad, rad, 0.0f, 0.65f, juce::MathConstants<float>::twoPi - 0.65f, true);
        icon.startNewSubPath (c.x, c.y - rad - 1.5f);
        icon.lineTo (c.x, c.y);
        g.setColour (on ? Theme::edge : Theme::dimText);
        g.strokePath (icon, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    if (b.getButtonText().isNotEmpty())   // labelled switch, e.g. filter routing A / B / N / S
    {
        auto area = b.getLocalBounds().toFloat();
        const auto box = area.removeFromLeft (16.0f).withSizeKeepingCentre (13.0f, 13.0f);
        g.setColour (on ? colour : Theme::field);
        g.fillRect (box);
        g.setColour (on ? colour.brighter (0.4f) : (over ? colour.withAlpha (0.7f) : Theme::dimText.withAlpha (0.6f)));
        g.drawRect (box, 1.0f);
        g.setColour (on ? Theme::text : Theme::dimText);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (b.getButtonText(), area.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
        return;
    }

    const auto sq = b.getLocalBounds().toFloat().withSizeKeepingCentre (9.0f, 9.0f);
    if (b.getToggleState())
    {
        g.setColour (colour.withAlpha (0.25f));
        g.fillRect (sq.expanded (2.0f));
        g.setColour (colour);
        g.fillRect (sq);
    }
    else
    {
        g.setColour (Theme::field);
        g.fillRect (sq);
        g.setColour (over ? Theme::accent.withAlpha (0.7f) : Theme::dimText.withAlpha (0.6f));
        g.drawRect (sq, 1.0f);
    }
}

//==============================================================================
Knob::Knob (APVTS& state, const juce::String& paramId, const juce::String& n) : name (n)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("knob", true);
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);

    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::paint (juce::Graphics& g)
{
    auto label = getLocalBounds().removeFromBottom (14).toFloat();
    if (boxedLabel)
    {
        // Hardware-style label plate under the knob
        const auto plate = label.withSizeKeepingCentre (juce::jmin (label.getWidth() - 2.0f, 60.0f), 14.0f);
        g.setColour (Theme::field);
        g.fillPath (Theme::chamfered (plate, 3.0f));
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (0.25f));
        g.strokePath (Theme::chamfered (plate, 3.0f), juce::PathStrokeType (1.0f));
    }
    g.setColour (Theme::text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (name.toUpperCase(), label, juce::Justification::centred);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (15);
    const int size = juce::jmin (r.getWidth(), r.getHeight(), 40);
    slider.setBounds (r.withSizeKeepingCentre (size, size));
}

//==============================================================================
Field::Field (APVTS& state, const juce::String& paramId, const juce::String& n) : name (n)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (140);
    slider.getProperties().set ("field", true);
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);

    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Field::paint (juce::Graphics& g)
{
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    if (getHeight() > 34)   // stacked: box above, name below (sits in a knob row)
        g.drawText (name.toUpperCase(), getLocalBounds().removeFromBottom (14), juce::Justification::centred);
    else
        g.drawText (name.toUpperCase(), getLocalBounds().removeFromLeft (30), juce::Justification::centredLeft);
}

void Field::resized()
{
    auto r = getLocalBounds();
    if (getHeight() > 34)
    {
        r.removeFromBottom (15);
        slider.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 46), 22));
    }
    else
    {
        r.removeFromLeft (30);
        slider.setBounds (r);
    }
}

//==============================================================================
Selector::Selector (APVTS& state, const juce::String& paramId)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
        box.addItemList (p->choices, 1);

    addAndMakeVisible (box);
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, box);

    auto step = [this] (int d)
    {
        const int n = box.getNumItems();
        box.setSelectedItemIndex ((box.getSelectedItemIndex() + d + n) % n);
    };
    prev.onClick = [step] { step (-1); };
    next.onClick = [step] { step (1); };
}

void Selector::resized()
{
    auto r = getLocalBounds();
    next.setBounds (r.removeFromRight (16).withSizeKeepingCentre (10, 10));
    r.removeFromRight (3);
    prev.setBounds (r.removeFromLeft (16).withSizeKeepingCentre (10, 10));
    r.removeFromLeft (3);
    box.setBounds (r);
}

//==============================================================================
Panel::Panel (const juce::String& t, APVTS* state, const juce::String& powerId) : title (t)
{
    if (state != nullptr && powerId.isNotEmpty())
    {
        power.setTooltip ("Turn " + t + " on/off");
        power.onStateChange = [this] { repaint(); };
        addAndMakeVisible (power);
        powerAttachment = std::make_unique<APVTS::ButtonAttachment> (*state, powerId, power);
    }
}

void Panel::setAccent (juce::Colour c)
{
    accent = c;
    std::function<void (juce::Component&)> apply = [&] (juce::Component& parent)
    {
        for (auto* child : parent.getChildren())
        {
            if (auto* sl = dynamic_cast<juce::Slider*> (child))           sl->setColour (juce::Slider::rotarySliderFillColourId, c);
            else if (auto* tb = dynamic_cast<juce::ToggleButton*> (child)) tb->setColour (juce::ToggleButton::tickColourId, c);
            else if (auto* bt = dynamic_cast<juce::TextButton*> (child))   bt->setColour (juce::TextButton::buttonOnColourId, c);
            else if (auto* cb = dynamic_cast<juce::ComboBox*> (child))     cb->setColour (juce::ComboBox::textColourId, c);
            if (auto* gr = dynamic_cast<GraphBase*> (child))
                gr->setAccent (c);
            apply (*child);
        }
    };
    apply (*this);
    repaint();
}

void Panel::resized()
{
    power.setBounds (header().removeFromLeft (26).reduced (3));
}

void Panel::paintOverChildren (juce::Graphics& g)
{
    // Dim the whole section while it's switched off (controls stay editable)
    if (powerAttachment != nullptr && ! power.getToggleState())
    {
        g.setColour (Theme::panelBottom.withAlpha (0.65f));
        g.fillPath (Theme::chamfered (getLocalBounds().toFloat().withTrimmedTop ((float) headerHeight).reduced (1.0f), 9.0f));
    }
}

void Panel::paint (juce::Graphics& g)
{
    const bool on = powerAttachment == nullptr || power.getToggleState();
    Theme::drawPanelFrame (g, getLocalBounds().toFloat(), (float) headerHeight, on ? accent : accent.withAlpha (0.4f));

    if (title.isNotEmpty())
    {
        g.setColour (on ? accent.interpolatedWith (Theme::text, 0.55f) : Theme::dimText);
        g.setFont (Theme::font (14.0f, true));
        g.drawText (title, header().withTrimmedLeft (24), juce::Justification::centredLeft);
    }
}

void Panel::layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items)
{
    const int w = area.getWidth() / (int) items.size();
    for (auto* c : items)
        c->setBounds (area.removeFromLeft (w));
}

//==============================================================================
static juce::String oscPrefix (int i) { return "osc" + juce::String (i + 1); }

OscPanel::OscPanel (APVTS& s, int i)
    : Panel (i == 0 ? "OSC A" : "OSC B", &s, oscPrefix (i) + "On"),
      wave (s, oscPrefix (i) + "Wave"),
      octave (s, oscPrefix (i) + "Octave", "Oct"),
      semi (s, oscPrefix (i) + "Semi", "Sem"),
      fine (s, oscPrefix (i) + "Fine", "Fin"),
      graph (s, oscPrefix (i)),
      unison (s, oscPrefix (i) + "Voices", "Unison"),
      detune (s, oscPrefix (i) + "Detune", "Detune"),
      width (s, oscPrefix (i) + "Width", "Width"),
      level (s, oscPrefix (i) + "Level", "Level")
{
    for (auto* c : std::initializer_list<juce::Component*> { &wave, &octave, &semi, &fine, &graph, &unison, &detune, &width, &level })
        addAndMakeVisible (c);
}

void OscPanel::resized()
{
    Panel::resized();
    auto r = content();
    wave.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);

    auto fields = r.removeFromTop (20);
    const int fw = (fields.getWidth() - 12) / 3;
    octave.setBounds (fields.removeFromLeft (fw));
    fields.removeFromLeft (6);
    semi.setBounds (fields.removeFromLeft (fw));
    fields.removeFromLeft (6);
    fine.setBounds (fields);
    r.removeFromTop (6);

    layoutRow (r.removeFromBottom (56), { &unison, &detune, &width, &level });
    r.removeFromBottom (6);
    graph.setBounds (r);
}

SubPanel::SubPanel (APVTS& s)
    : Panel ("SUB", &s, "subOn"), wave (s, "subWave"), octave (s, "subOctave"), level (s, "subLevel", "Level")
{
    addAndMakeVisible (wave);
    addAndMakeVisible (octave);
    addAndMakeVisible (level);
}

void SubPanel::resized()
{
    Panel::resized();
    auto r = content();
    wave.setBounds (r.removeFromTop (22));
    r.removeFromTop (5);
    octave.setBounds (r.removeFromTop (22));
    r.removeFromTop (4);
    level.setBounds (r);
}

AirPanel::AirPanel (APVTS& s) : Panel ("NOISE", &s, "noiseOn"), level (s, "noiseLevel", "Level")
{
    addAndMakeVisible (level);
}

void AirPanel::resized()
{
    Panel::resized();
    level.setBounds (content().withSizeKeepingCentre (content().getWidth(), juce::jmin (content().getHeight(), 84)));
}

FilterPanel::FilterPanel (APVTS& s)
    : Panel ("FILTER", &s, "filterOn"), type (s, "filterType"), graph (s),
      cutoff (s, "cutoff", "Cutoff"), reso (s, "reso", "Res"), drive (s, "drive", "Drive"),
      envAmount (s, "filterEnv", "Env 2"), keyTrack (s, "keyTrack", "Key Trk"), lowCut (s, "lowCut", "Low Cut")
{
    for (auto* c : std::initializer_list<juce::Component*> { &type, &graph, &cutoff, &reso, &drive, &envAmount, &keyTrack, &lowCut })
        addAndMakeVisible (c);

    juce::ToggleButton* routes[] = { &routeA, &routeB, &routeN, &routeS };
    const char* ids[] = { "filterA", "filterB", "filterN", "filterS" };
    const char* tips[] = { "Send Osc A through the filter", "Send Osc B through the filter",
                           "Send Noise through the filter", "Send Sub through the filter" };
    for (int i = 0; i < 4; ++i)
    {
        routes[i]->setTooltip (tips[i]);
        addAndMakeVisible (routes[i]);
        routeAttachments[i] = std::make_unique<APVTS::ButtonAttachment> (s, ids[i], *routes[i]);
    }
}

void FilterPanel::resized()
{
    Panel::resized();
    auto r = content();
    type.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);

    // Bottom: routing column on the left, two rows of knobs on the right
    auto controls = r.removeFromBottom (108);
    r.removeFromBottom (6);
    graph.setBounds (r);

    auto routeCol = controls.removeFromLeft (38);
    const int rowH = routeCol.getHeight() / 4;
    for (auto* b : { &routeA, &routeB, &routeN, &routeS })
        b->setBounds (routeCol.removeFromTop (rowH));

    layoutRow (controls.removeFromTop (controls.getHeight() / 2), { &cutoff, &reso, &drive });
    layoutRow (controls, { &envAmount, &keyTrack, &lowCut });
}

EnvPanel::EnvPanel (APVTS& s)
    : Panel ({}),
      ampGraph (s, "aAttack", "aDecay", "aSustain", "aRelease", Theme::accent, true),
      filterGraph (s, "fAttack", "fDecay", "fSustain", "fRelease", Theme::accent, false),
      aA (s, "aAttack", "Attack"), aD (s, "aDecay", "Decay"), aS (s, "aSustain", "Sustain"),
      aR (s, "aRelease", "Release"), vel (s, "velSens", "Velocity"),
      fA (s, "fAttack", "Attack"), fD (s, "fDecay", "Decay"), fS (s, "fSustain", "Sustain"),
      fR (s, "fRelease", "Release")
{
    for (auto* c : std::initializer_list<juce::Component*> { &ampTab, &filterTab, &ampGraph, &filterGraph,
                                                             &aA, &aD, &aS, &aR, &vel, &fA, &fD, &fS, &fR })
        addAndMakeVisible (c);

    ampTab.setClickingTogglesState (true);
    filterTab.setClickingTogglesState (true);
    ampTab.setRadioGroupId (1);
    filterTab.setRadioGroupId (1);
    ampTab.onClick = [this] { showPage (0); };
    filterTab.onClick = [this] { showPage (1); };
    ampTab.setToggleState (true, juce::dontSendNotification);
    showPage (0);
}

void EnvPanel::showPage (int p)
{
    page = p;
    for (auto* c : std::initializer_list<juce::Component*> { &ampGraph, &aA, &aD, &aS, &aR, &vel })
        c->setVisible (p == 0);
    for (auto* c : std::initializer_list<juce::Component*> { &filterGraph, &fA, &fD, &fS, &fR })
        c->setVisible (p == 1);
}

void EnvPanel::paint (juce::Graphics& g)
{
    Panel::paint (g);
    g.setColour (Theme::dimText.withAlpha (0.7f));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("drag the points", header().reduced (10, 0), juce::Justification::centredRight);
}

void EnvPanel::resized()
{
    auto h = header().reduced (2, 2);
    ampTab.setBounds (h.removeFromLeft (110));
    filterTab.setBounds (h.removeFromLeft (120));

    auto r = content();
    auto knobs = r.removeFromBottom (56);
    r.removeFromBottom (6);
    ampGraph.setBounds (r);
    filterGraph.setBounds (r);

    layoutRow (knobs, { &aA, &aD, &aS, &aR, &vel });
    knobs.removeFromRight (knobs.getWidth() / 5);
    layoutRow (knobs, { &fA, &fD, &fS, &fR });
}

LfoPanel::LfoPanel (APVTS& s)
    : Panel ("LFO", &s, "lfoOn"), shape (s, "lfoShape"), graph (s),
      rate (s, "lfoRate", "Rate"), delay (s, "lfoDelay", "Delay"), pitch (s, "lfoPitch", "Pitch"),
      cutoff (s, "lfoCutoff", "Cutoff"), amp (s, "lfoAmp", "Amp")
{
    for (auto* c : std::initializer_list<juce::Component*> { &shape, &graph, &rate, &delay, &pitch, &cutoff, &amp })
        addAndMakeVisible (c);
}

void LfoPanel::resized()
{
    Panel::resized();
    shape.setBounds (header().reduced (6, 3).removeFromRight (140));
    auto r = content();
    layoutRow (r.removeFromBottom (56), { &rate, &delay, &pitch, &cutoff, &amp });
    r.removeFromBottom (6);
    graph.setBounds (r);
}

PumpPanel::PumpPanel (APVTS& s)
    : Panel ("PUMP", &s, "pumpOn"), rate (s, "pumpRate"), shape (s, "pumpShape"), depth (s, "pumpDepth", "Depth")
{
    addAndMakeVisible (rate);
    addAndMakeVisible (shape);
    addAndMakeVisible (depth);
}

void PumpPanel::resized()
{
    Panel::resized();
    auto r = content();
    rate.setBounds (r.removeFromTop (22));
    r.removeFromTop (5);
    shape.setBounds (r.removeFromTop (22));
    r.removeFromTop (6);
    depth.setBounds (r);
}

ChoiceLeds::ChoiceLeds (APVTS& s, const juce::String& paramId, juce::Colour c)
    : choice (s, paramId, [this] { repaint(); }), colour (c)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (paramId)))
        names = p->choices;
}

void ChoiceLeds::paint (juce::Graphics& g)
{
    const int selectedIndex = juce::roundToInt (choice.get());
    auto area = getLocalBounds().toFloat().withSizeKeepingCentre ((float) getWidth(), (float) preferredHeight());
    for (int i = 0; i < names.size(); ++i)
    {
        auto row = area.removeFromTop (18.0f);
        const auto led = row.removeFromLeft (14.0f).withSizeKeepingCentre (10.0f, 10.0f);
        const bool on = i == selectedIndex;
        if (on)
        {
            g.setColour (colour.withAlpha (0.3f));
            g.fillRect (led.expanded (2.5f));
            g.setColour (colour);
            g.fillRect (led);
        }
        else
        {
            g.setColour (Theme::field);
            g.fillRect (led);
            g.setColour (Theme::dimText.withAlpha (0.5f));
            g.drawRect (led, 1.0f);
        }
        g.setColour (on ? Theme::text : Theme::dimText);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (names[i].toUpperCase(), row.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
    }
}

void ChoiceLeds::mouseDown (const juce::MouseEvent& e)
{
    const float top = ((float) getHeight() - (float) preferredHeight()) * 0.5f;
    const int index = juce::jlimit (0, names.size() - 1, (int) ((e.position.y - top) / 18.0f));
    choice.begin();
    choice.set ((float) index);
    choice.end();
}

//==============================================================================
RackStrip::RackStrip (APVTS& s, const juce::String& t, juce::Colour c, const juce::String& powerId)
    : title (t), colour (c), state (s)
{
    power.getProperties().set ("powerIcon", true);
    power.setColour (juce::ToggleButton::tickColourId, c);
    power.setTooltip ("Turn " + t + " off (removes it from the rack)");
    addAndMakeVisible (power);
    powerAttachment = std::make_unique<APVTS::ButtonAttachment> (s, powerId, power);
}

RackStrip& RackStrip::selector (const char* id, int width)
{
    auto* sel = new Selector (state, id);
    sel->setAccent (colour);
    addAndMakeVisible (owned.add (sel));
    items.push_back ({ sel, width, false });
    return *this;
}

RackStrip& RackStrip::leds (const char* id, int width)
{
    auto* l = new ChoiceLeds (state, id, colour);
    addAndMakeVisible (owned.add (l));
    items.push_back ({ l, width, false });
    return *this;
}

RackStrip& RackStrip::graph (std::unique_ptr<juce::Component> g, int width)
{
    if (auto* gb = dynamic_cast<GraphBase*> (g.get()))
        gb->setAccent (colour);
    auto* raw = g.release();
    addAndMakeVisible (owned.add (raw));
    items.push_back ({ raw, width, false });
    return *this;
}

RackStrip& RackStrip::knob (const char* id, const char* name)
{
    auto* k = new Knob (state, id, name);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k->setBoxedLabel (true);
    addAndMakeVisible (owned.add (k));
    items.push_back ({ k, 56, true });
    return *this;
}

RackStrip& RackStrip::mix (const char* id)
{
    mixKnob = new Knob (state, id, "Mix");
    mixKnob->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    mixKnob->setBoxedLabel (true);
    addAndMakeVisible (owned.add (mixKnob));
    return *this;
}

void RackStrip::setSelected (bool shouldBeSelected)
{
    selected = shouldBeSelected;
    repaint();
}

namespace
{
constexpr int earLeft = 22, earRight = 40, tagWidth = 132, mixWidth = 62;

// Rack bolt: dark socket, cross slot, faint glowing core
void drawBolt (juce::Graphics& g, juce::Point<float> c, juce::Colour accent)
{
    const auto r = juce::Rectangle<float> (12.0f, 12.0f).withCentre (c);
    g.setColour (juce::Colour (0xff05080c));
    g.fillEllipse (r);
    g.setColour (accent.withAlpha (0.35f));
    g.drawEllipse (r.reduced (0.5f), 1.0f);
    g.setColour (Theme::dimText.withAlpha (0.8f));
    g.drawLine (c.x - 3.0f, c.y, c.x + 3.0f, c.y, 1.4f);
    g.drawLine (c.x, c.y - 3.0f, c.x, c.y + 3.0f, 1.4f);
    g.setColour (accent.withAlpha (0.6f));
    g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (c));
}
} // namespace

void RackStrip::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Ears: textured metal with two bolts each
    for (auto ear : { b.withWidth ((float) earLeft), b.withLeft (b.getRight() - (float) earRight) })
    {
        g.setGradientFill (juce::ColourGradient (Theme::headerTop, 0.0f, ear.getY(), Theme::headerBot, 0.0f, ear.getBottom(), false));
        g.fillRect (ear);
        Theme::drawTexture (g, ear);
        g.setColour (Theme::edge);
        g.drawRect (ear, 1.0f);
        drawBolt (g, { ear.getCentreX(), ear.getY() + 11.0f }, colour);
        drawBolt (g, { ear.getCentreX(), ear.getBottom() - 11.0f }, colour);
    }

    // Faceplate between the ears: clean dark gradient, light top edge, colour glow when selected
    const auto face = b.withTrimmedLeft ((float) earLeft).withTrimmedRight ((float) earRight);
    g.setGradientFill (juce::ColourGradient (Theme::panelTop, 0.0f, face.getY(), Theme::panelBottom, 0.0f, face.getBottom(), false));
    g.fillRect (face);
    Theme::drawTexture (g, face, 0.35f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) face.getY(), face.getX(), face.getRight());
    g.setColour (Theme::edge);
    g.drawHorizontalLine ((int) face.getBottom() - 1, face.getX(), face.getRight());
    if (selected)
    {
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.10f), face.getX(), 0.0f, juce::Colours::transparentBlack, face.getX() + 300.0f, 0.0f, false));
        g.fillRect (face);
        juce::Path edge;
        edge.startNewSubPath (face.getX(), face.getY() + 0.5f);
        edge.lineTo (face.getRight(), face.getY() + 0.5f);
        Theme::strokeGlow (g, edge, colour.withAlpha (0.8f), 1.0f);
    }

    // Divider before the Mix knob
    if (mixKnob != nullptr)
    {
        const float x = face.getRight() - (float) mixWidth - 10.0f;
        g.setColour (colour.withAlpha (0.25f));
        g.drawVerticalLine ((int) x, face.getY() + 12.0f, face.getBottom() - 12.0f);
    }

    // Title tag across the left ear, between the bolts
    const auto tag = juce::Rectangle<float> (0.0f, b.getCentreY() - 15.0f, (float) tagWidth, 30.0f);
    juce::Path tagShape;
    tagShape.startNewSubPath (tag.getX(), tag.getY());
    tagShape.lineTo (tag.getRight(), tag.getY());
    tagShape.lineTo (tag.getRight(), tag.getBottom() - 8.0f);
    tagShape.lineTo (tag.getRight() - 8.0f, tag.getBottom());
    tagShape.lineTo (tag.getX(), tag.getBottom());
    tagShape.closeSubPath();
    g.setColour (colour.withAlpha (0.25f));
    g.fillRect (tag.expanded (0.0f, 2.0f).withWidth (tag.getWidth() - 6.0f));
    g.setGradientFill (juce::ColourGradient (colour.brighter (0.15f), 0.0f, tag.getY(), colour.darker (0.25f), 0.0f, tag.getBottom(), false));
    g.fillPath (tagShape);
    g.setColour (Theme::edge);
    g.setFont (Theme::font (17.0f, true));
    g.drawText (title, tag.reduced (10.0f, 0.0f), juce::Justification::centredLeft);
}

void RackStrip::resized()
{
    auto r = getLocalBounds();
    power.setBounds (r.removeFromRight (earRight).withSizeKeepingCentre (28, 28));
    r.removeFromLeft (tagWidth + 10);
    r.removeFromRight (8);
    r = r.reduced (0, 7);

    if (mixKnob != nullptr)
    {
        mixKnob->setBounds (r.removeFromRight (mixWidth));
        r.removeFromRight (20);
    }

    // Fixed-width items keep their size; knobs share what's left, then spare room becomes even gaps
    int fixed = 0, numKnobs = 0;
    for (auto& item : items)
    {
        if (item.isKnob) ++numKnobs;
        else fixed += item.width;
    }
    constexpr int minGap = 6;
    const int knobW = numKnobs > 0 ? juce::jlimit (40, 60, (r.getWidth() - fixed - minGap * (int) (items.size() + 1)) / numKnobs) : 0;
    const int used = fixed + knobW * numKnobs;
    const int gap = juce::jmax (minGap, (r.getWidth() - used) / (int) (items.size() + 1));

    int x = r.getX() + gap;
    for (auto& item : items)
    {
        const int w = item.isKnob ? knobW : item.width;
        auto cell = juce::Rectangle<int> (x, r.getY(), w, r.getHeight());
        if (dynamic_cast<Selector*> (item.component) != nullptr)
            cell = cell.withSizeKeepingCentre (w, 24);
        else if (auto* l = dynamic_cast<ChoiceLeds*> (item.component))
            cell = cell.withSizeKeepingCentre (w, l->preferredHeight());
        item.component->setBounds (cell);
        x += w + gap;
    }
}

//==============================================================================
RackListItem::RackListItem (APVTS& s, const juce::String& n, juce::Colour c, const juce::String& powerId)
    : name (n), colour (c)
{
    led.setColour (juce::ToggleButton::tickColourId, c);
    led.getProperties().set ("led", true);
    led.setTooltip ("Turn " + n + " on/off");
    led.onStateChange = [this] { repaint(); };
    led.onClick = [this] { repaint(); if (onPowerChanged) onPowerChanged(); };   // also fires on automation
    addAndMakeVisible (led);
    attachment = std::make_unique<APVTS::ButtonAttachment> (s, powerId, led);
}

void RackListItem::setSelected (bool shouldBeSelected)
{
    selected = shouldBeSelected;
    repaint();
}

void RackListItem::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const auto shape = Theme::chamfered (b, 8.0f);
    g.setGradientFill (juce::ColourGradient (selected ? colour.withAlpha (0.16f) : Theme::headerTop, 0.0f, 0.0f,
                                             Theme::headerBot, b.getRight(), 0.0f, false));
    g.fillPath (shape);
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (shape);
        Theme::drawTexture (g, b, 0.8f);
    }
    g.setColour (selected ? colour.withAlpha (0.9f) : colour.withAlpha (0.18f));
    g.strokePath (shape, juce::PathStrokeType (selected ? 1.2f : 1.0f));

    g.setColour (isOn() ? Theme::text : Theme::dimText);
    g.setFont (Theme::font (16.0f, true));
    g.drawText (name, getLocalBounds().withTrimmedLeft (62), juce::Justification::centredLeft);
}

void RackListItem::resized()
{
    led.setBounds (getLocalBounds().removeFromLeft (52).withSizeKeepingCentre (32, 14));
}

void RackListItem::mouseDown (const juce::MouseEvent&)
{
    // Picking a module from the list adds it to the rack if it was off
    if (! isOn())
        led.setToggleState (true, juce::sendNotification);
    if (onSelect)
        onSelect();
}

//==============================================================================
void FxPage::RackContent::paint (juce::Graphics& g)
{
    if (! empty)
        return;
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText ("Rack is empty - switch on an effect in the list", getLocalBounds().withHeight (120),
                juce::Justification::centred);
}

FxPage::FxPage (APVTS& s)
{
    // Signal-flow order
    auto add = [this, &s] (const char* t, juce::Colour c, const char* powerId) -> RackStrip&
    {
        return *strips.add (new RackStrip (s, t, c, powerId));
    };

    add ("DISTORTION", FxColours::distortion, "distOn")
        .selector ("distType", 104)
        .leds ("distFilterMode", 58)
        .graph (std::make_unique<MiniFilterGraph> (s, "distFilterType", "distFilterFreq", "distFilterQ", "distFilterMode"), 132)
        .leds ("distFilterType", 40)
        .knob ("distDrive", "Drive")
        .graph (std::make_unique<DistGraph> (s), 64)
        .mix ("distMix");
    add ("PHASER", FxColours::phaser, "phaserOn")
        .knob ("phaserRate", "Rate").knob ("phaserDepth", "Depth").knob ("phaserFreq", "Freq").knob ("phaserFeedback", "Feedback")
        .mix ("phaserMix");
    add ("CHORUS", FxColours::chorus, "chorusOn")
        .knob ("chorusRate", "Rate").knob ("chorusDepth", "Depth").knob ("chorusDelay", "Delay").knob ("chorusFeedback", "Feedback")
        .mix ("chorusMix");
    add ("DELAY", FxColours::delay, "delayOn")
        .leds ("delayMode", 84)
        .selector ("delaySync", 96)
        .knob ("delayFeedback", "Feedback").knob ("delayLowCut", "Low Cut").knob ("delayHighCut", "High Cut")
        .mix ("delayMix");
    add ("COMPRESSOR", FxColours::compressor, "compOn")
        .knob ("compThreshold", "Thresh").knob ("compRatio", "Ratio").knob ("compAttack", "Attack")
        .knob ("compRelease", "Release").knob ("compGain", "Gain")
        .mix ("compMix");
    add ("REVERB", FxColours::reverb, "reverbOn")
        .leds ("reverbMode", 62)
        .knob ("reverbPreDelay", "Pre-Delay").knob ("reverbSize", "Size").knob ("reverbDecay", "Decay")
        .knob ("reverbLowCut", "Low Cut").knob ("reverbHighCut", "High Cut").knob ("reverbSpin", "Spin")
        .knob ("reverbSpinDepth", "Spin Amt").knob ("reverbWidth", "Width")
        .mix ("reverbMix");
    add ("EQ", FxColours::eq, "eqOn")
        .graph (std::make_unique<EqGraph> (s), 190)
        .knob ("eqLow", "Low").knob ("eqMid", "Mid").knob ("eqMidFreq", "Freq").knob ("eqMidQ", "Q").knob ("eqHigh", "High");

    const char* powerIds[] = { "distOn", "phaserOn", "chorusOn", "delayOn", "compOn", "reverbOn", "eqOn" };
    for (int i = 0; i < strips.size(); ++i)
    {
        auto* item = items.add (new RackListItem (s, strips[i]->title, strips[i]->colour, powerIds[i]));
        item->onSelect = [this, i] { select (i); };
        item->onPowerChanged = [this] { layoutRack(); };
        addAndMakeVisible (item);

        strips[i]->onSelect = [this, i] { select (i); };
        rack.addChildComponent (strips[i]);
    }

    viewport.setViewedComponent (&rack, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);

    select (2); // chorus is on by default
}

void FxPage::select (int index)
{
    selectedIndex = index;
    for (int i = 0; i < strips.size(); ++i)
    {
        strips[i]->setSelected (i == index);
        items[i]->setSelected (i == index);
    }
    if (strips[index]->isVisible())
        viewport.setViewPosition (0, juce::jmax (0, strips[index]->getY() - 8));
}

void FxPage::layoutRack()
{
    if (items.size() != strips.size())
        return;

    const int w = viewport.getMaximumVisibleWidth();
    int y = 4;
    rack.empty = true;
    for (int i = 0; i < strips.size(); ++i)
    {
        const bool on = items[i]->isOn();
        strips[i]->setVisible (on);
        if (! on)
            continue;
        strips[i]->setBounds (4, y, w - 8, RackStrip::unitHeight);
        y += RackStrip::unitHeight + 4;
        rack.empty = false;
    }
    rack.setSize (w, juce::jmax (y, viewport.getHeight()));
    rack.repaint();
}

void FxPage::paint (juce::Graphics& g)
{
    // Dark rack well behind the strips
    const auto well = rackArea.toFloat();
    const auto shape = Theme::chamfered (well, 12.0f);
    g.setColour (juce::Colour (0xff04070b));
    g.fillPath (shape);
    g.setColour (Theme::accent.withAlpha (0.2f));
    g.strokePath (shape, juce::PathStrokeType (1.0f));

    // Rails down both sides with glowing mounting points, like a hardware rack
    for (int y = rackArea.getY() + 20; y < rackArea.getBottom() - 10; y += 70)
        for (float x : { (float) rackArea.getX() - 12.0f, (float) rackArea.getRight() + 8.0f })
        {
            g.setColour (Theme::accent.withAlpha (0.15f));
            g.fillEllipse (x - 2.0f, (float) y - 2.0f, 8.0f, 8.0f);
            g.setColour (Theme::accent.withAlpha (0.7f));
            g.fillEllipse (x + 1.0f, (float) y + 1.0f, 2.0f, 2.0f);
        }
}

void FxPage::resized()
{
    auto r = getLocalBounds();
    listArea = r.removeFromLeft (150);
    r.removeFromLeft (24);
    rackArea = r.withTrimmedRight (20);
    viewport.setBounds (rackArea.reduced (2));

    auto list = listArea;
    const int rowH = juce::jmin (52, (list.getHeight() - 6 * (items.size() - 1)) / items.size());
    for (auto* item : items)
    {
        item->setBounds (list.removeFromTop (rowH));
        list.removeFromTop (6);
    }

    layoutRack();
}

//==============================================================================
OutputMeter::OutputMeter (std::atomic<float>* p) : peaks (p)
{
    setTooltip ("Output level (click to clear the clip lights)");
    startTimerHz (30);
}

void OutputMeter::timerCallback()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        const float peak = peaks[ch].exchange (0.0f);
        level[ch] = juce::jmax (peak, level[ch] * 0.86f);          // ~20 dB/s fall
        if (peak >= hold[ch]) { hold[ch] = peak; holdFrames[ch] = 45; }
        else if (--holdFrames[ch] <= 0) hold[ch] *= 0.9f;
        if (peak >= 1.0f) clipped[ch] = true;
    }
    repaint();
}

void OutputMeter::paint (juce::Graphics& g)
{
    // -48..+3 dB, segmented like hardware LED ladders
    auto toY = [] (float gain, juce::Rectangle<float> bar)
    {
        const float db = juce::jlimit (-48.0f, 3.0f, juce::Decibels::gainToDecibels (gain, -100.0f));
        return bar.getBottom() - (db + 48.0f) / 51.0f * bar.getHeight();
    };

    auto b = getLocalBounds().toFloat();
    const float barW = (b.getWidth() - 3.0f) / 2.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto col = b.withX (b.getX() + (float) ch * (barW + 3.0f)).withWidth (barW);
        const auto clipLed = col.removeFromTop (4.0f);
        col.removeFromTop (2.0f);

        g.setColour (Theme::graphBg);
        g.fillRect (col);

        // Segment colour by level: green, yellow from -12 dB, orange from -6 dB, red above 0 dB
        auto segmentColour = [] (float db)
        {
            return db >= 0.0f ? juce::Colour (0xffff3b3b) : db >= -6.0f ? juce::Colour (0xffffa62b)
                 : db >= -12.0f ? juce::Colour (0xffe8e24a) : juce::Colour (0xff39e07a);
        };
        const float yLevel = toY (level[ch], col);
        for (float y = col.getBottom() - 2.0f; y > col.getY(); y -= 3.0f)
        {
            const float db = (col.getBottom() - y) / col.getHeight() * 51.0f - 48.0f;
            const bool lit = y >= yLevel;
            g.setColour (segmentColour (db).withAlpha (lit ? 1.0f : 0.12f));
            g.fillRect (col.getX(), y - 2.0f, col.getWidth(), 2.0f);
        }

        // Peak hold line
        if (hold[ch] > 0.001f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (col.getX(), toY (hold[ch], col) - 1.0f, col.getWidth(), 1.5f);
        }

        g.setColour (clipped[ch] ? juce::Colour (0xffff3b3b) : juce::Colour (0xff3a1418));
        g.fillRect (clipLed);
    }
}

//==============================================================================
ModWheel::ModWheel (std::atomic<float>& v) : value (v)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip ("Mod wheel: drag up/down (double-click to reset). Follows your controller's mod wheel.");
    startTimerHz (30);
}

void ModWheel::timerCallback()
{
    // Follow the hardware wheel
    if (std::abs (value.load() - shown) > 0.001f)
        repaint();
}

void ModWheel::mouseDown (const juce::MouseEvent&)
{
    dragStart = value.load();
}

void ModWheel::mouseDrag (const juce::MouseEvent& e)
{
    value = juce::jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / (float) getHeight());
    repaint();
}

void ModWheel::paint (juce::Graphics& g)
{
    shown = value.load();
    auto b = getLocalBounds().toFloat();
    auto bar = b.removeFromRight (6.0f);
    b.removeFromRight (4.0f);

    // Slot
    g.setColour (Theme::graphBg);
    g.fillRect (b);
    g.setColour (Theme::accent.withAlpha (0.35f));
    g.drawRect (b, 1.0f);

    // Wheel surface: ribs scroll with the value, shaded like a cylinder
    const auto wheel = b.reduced (4.0f, 3.0f);
    juce::ColourGradient shade (juce::Colour (0xff05080b), 0.0f, wheel.getY(), juce::Colour (0xff05080b), 0.0f, wheel.getBottom(), false);
    shade.addColour (0.5, juce::Colour (0xff27323e));
    g.setGradientFill (shade);
    g.fillRect (wheel);

    const float spacing = 7.0f, offset = std::fmod (shown * 180.0f, spacing);
    for (float y = wheel.getY() + offset; y < wheel.getBottom(); y += spacing)
    {
        const float t = (y - wheel.getY()) / wheel.getHeight();             // 0..1 down the wheel
        const float light = std::sin (t * juce::MathConstants<float>::pi);   // brightest in the middle
        g.setColour (juce::Colour (0xff9fb3c8).withAlpha (0.08f + 0.35f * light));
        g.drawHorizontalLine ((int) y, wheel.getX() + 2.0f, wheel.getRight() - 2.0f);
    }

    // Position marker on the wheel
    const float markerY = wheel.getBottom() - shown * wheel.getHeight();
    g.setColour (Theme::accent.withAlpha (0.25f));
    g.fillRect (juce::Rectangle<float> (wheel.getX(), markerY - 3.0f, wheel.getWidth(), 6.0f));
    g.setColour (Theme::accent);
    g.fillRect (juce::Rectangle<float> (wheel.getX(), markerY - 1.0f, wheel.getWidth(), 2.0f));

    // Glowing level bar
    g.setColour (Theme::graphBg);
    g.fillRect (bar);
    const auto fill = bar.withTop (bar.getBottom() - shown * bar.getHeight());
    g.setColour (Theme::accent.withAlpha (0.25f));
    g.fillRect (fill.expanded (2.0f, 0.0f));
    g.setColour (Theme::accent);
    g.fillRect (fill);
}

//==============================================================================
VelvetPadEditor::VelvetPadEditor (VelvetPadProcessor& proc)
    : AudioProcessorEditor (proc),
      sub (proc.apvts), air (proc.apvts), oscA (proc.apvts, 0), oscB (proc.apvts, 1),
      filter (proc.apvts), env (proc.apvts), lfo (proc.apvts), pump (proc.apvts), fxPage (proc.apvts), visPage (proc.scope),
      master (proc.apvts, "master", "Master"),
      outputMeter (proc.outputPeak),
      modWheel (proc.modWheel),
      wheelCutoff (proc.apvts, "wheelCutoff", "> Cutoff"),
      wheelVibrato (proc.apvts, "wheelVibrato", "> Vibrato"),
      keyboard (proc.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> { &sub, &air, &oscA, &oscB, &filter, &env, &lfo, &pump, &fxPage, &visPage, &master, &outputMeter, &keyboard, &modWheel, &wheelCutoff, &wheelVibrato, &oscTab, &fxTab, &visTab })
        addAndMakeVisible (c);

    for (auto* tab : { &oscTab, &fxTab, &visTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (2);
    }
    oscTab.onClick = [this] { showPage (0); };
    fxTab.onClick = [this] { showPage (1); };
    visTab.onClick = [this] { showPage (2); };
    oscTab.setToggleState (true, juce::dontSendNotification);
    showPage (0);

    sub.setAccent (SectionColours::sub);
    air.setAccent (SectionColours::noise);
    oscA.setAccent (SectionColours::oscA);
    oscB.setAccent (SectionColours::oscB);
    filter.setAccent (SectionColours::filter);
    env.setAccent (SectionColours::env);
    lfo.setAccent (SectionColours::lfo);
    pump.setAccent (SectionColours::pump);

    env.setMonitor (&proc.monitor);
    lfo.setMonitor (&proc.monitor);

    keyboard.setAvailableRange (24, 96);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffb9c6d4));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff090d12));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff4a5868));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::accent.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::accent.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));

    // Show knob values in a bubble while hovering or dragging
    std::function<void (juce::Component&)> enablePopups = [&] (juce::Component& c)
    {
        if (auto* sl = dynamic_cast<juce::Slider*> (&c); sl != nullptr && sl->getProperties()["knob"])
            sl->setPopupDisplayEnabled (true, true, this, 1200);
        for (auto* child : c.getChildren())
            enablePopups (*child);
    };
    enablePopups (*this);

    setSize (1000, 760);
}

VelvetPadEditor::~VelvetPadEditor()
{
    setLookAndFeel (nullptr);
}

void VelvetPadEditor::showPage (int page)
{
    for (auto* c : std::initializer_list<juce::Component*> { &sub, &air, &oscA, &oscB, &filter, &env, &lfo, &pump })
        c->setVisible (page == 0);
    fxPage.setVisible (page == 1);
    visPage.setVisible (page == 2);
}

void VelvetPadEditor::paint (juce::Graphics& g)
{
    // Background: deep gradient, grid texture, soft cyan glow from the top
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1119), 0.0f, 0.0f, Theme::background, 0.0f, (float) getHeight(), false));
    g.fillAll();
    Theme::drawTexture (g, getLocalBounds().toFloat(), 0.7f);
    const float midX = (float) getWidth() * 0.5f;
    g.setGradientFill (juce::ColourGradient (Theme::accent.withAlpha (0.08f), midX, 0.0f,
                                             juce::Colours::transparentBlack, midX, 260.0f, true));
    g.fillRect (getLocalBounds());

    // Top bar with a glowing rule underneath
    auto top = getLocalBounds().removeFromTop (56).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff111821), 0.0f, 0.0f, juce::Colour (0xff0a0f15), 0.0f, top.getBottom(), false));
    g.fillRect (top);
    Theme::drawTexture (g, top);
    juce::Path rule;
    rule.startNewSubPath (0.0f, 55.5f);
    rule.lineTo (top.getRight(), 55.5f);
    Theme::strokeGlow (g, rule, Theme::accent.withAlpha (0.55f), 1.0f);

    // Title: glow pass, then a cyan > magenta gradient
    // Title as a vector path so it can take a gradient fill
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (Theme::font (28.0f, true).withExtraKerningFactor (0.12f), "VELVET PAD", 18.0f, 38.0f);
    juce::Path title;
    glyphs.createPath (title);
    const auto tb = title.getBounds();
    g.setColour (Theme::accent.withAlpha (0.18f));
    g.strokePath (title, juce::PathStrokeType (3.0f));
    g.setGradientFill (juce::ColourGradient (Theme::accent, tb.getX(), 0.0f, Theme::accent2, tb.getRight(), 0.0f, false));
    g.fillPath (title);

    g.setColour (Theme::dimText);
    g.setFont (Theme::font (12.0f));
    g.drawText ("WIDE PAD SYNTHESIZER  //  V1", 206, -6, 220, 56, juce::Justification::centredLeft);
    // Build stamp: shows at a glance which build the host has loaded
    g.setColour (Theme::dimText.withAlpha (0.6f));
    g.setFont (Theme::font (10.0f));
    g.drawText (juce::String ("BUILD ") + __DATE__ + "  " + __TIME__, 206, 10, 220, 56, juce::Justification::centredLeft);
    g.drawText ("drag the graphs   |   double-click to reset", 600, 0, 270, 56, juce::Justification::centredRight);

    // Keyboard deck with the mod wheel section on its left
    auto deck = getLocalBounds().removeFromBottom (122).reduced (8, 2).toFloat();
    Theme::drawPanelFrame (g, deck, 0.0f, Theme::accent);
    g.setColour (Theme::accent);
    g.setFont (Theme::font (11.0f, true));
    g.drawText ("MOD WHEEL", juce::Rectangle<float> (deck.getX() + 12.0f, deck.getY() + 4.0f, 180.0f, 16.0f),
                juce::Justification::centredLeft);
}

void VelvetPadEditor::resized()
{
    master.setBounds (getWidth() - 96, 2, 60, 54);
    outputMeter.setBounds (getWidth() - 32, 6, 20, 46);
    oscTab.setBounds (400, 14, 70, 30);
    fxTab.setBounds (474, 14, 70, 30);
    visTab.setBounds (548, 14, 70, 30);

    const int gap = 8;
    auto area = getLocalBounds().withTrimmedTop (56 + gap).reduced (gap, 0);

    auto keys = getLocalBounds().removeFromBottom (122).reduced (gap, 8);

    // Mod wheel section left of the keyboard: wheel + its two destination knobs
    auto wheelArea = keys.removeFromLeft (196);
    keys.removeFromLeft (10);
    wheelArea.removeFromTop (16);   // room for the "MOD WHEEL" label
    modWheel.setBounds (wheelArea.removeFromLeft (46));
    wheelArea.removeFromLeft (6);
    wheelCutoff.setBounds (wheelArea.removeFromLeft (wheelArea.getWidth() / 2));
    wheelVibrato.setBounds (wheelArea);

    keyboard.setBounds (keys);
    keyboard.setKeyWidth ((float) keys.getWidth() / 43.0f); // 43 white keys across the range

    fxPage.setBounds (area.withHeight (330 + gap + 222));
    visPage.setBounds (fxPage.getBounds());

    auto upper = area.removeFromTop (330);
    area.removeFromTop (gap);
    auto lower = area.removeFromTop (222);

    auto col = upper.removeFromLeft (128);
    sub.setBounds (col.removeFromTop (160));
    col.removeFromTop (gap);
    air.setBounds (col);
    upper.removeFromLeft (gap);

    const int oscW = 262;
    oscA.setBounds (upper.removeFromLeft (oscW));
    upper.removeFromLeft (gap);
    oscB.setBounds (upper.removeFromLeft (oscW));
    upper.removeFromLeft (gap);
    filter.setBounds (upper);

    env.setBounds (lower.removeFromLeft (400));
    lower.removeFromLeft (gap);
    lfo.setBounds (lower.removeFromLeft (400));
    lower.removeFromLeft (gap);
    pump.setBounds (lower);
}
