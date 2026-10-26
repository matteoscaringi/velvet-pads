#include "PluginEditor.h"

//==============================================================================
VelvetLookAndFeel::VelvetLookAndFeel()
{
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

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (2.5f, radius * 0.13f);
    const float arcR = radius - lineW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Theme::track);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    // Brushed-metal knob cap
    const float capR = arcR - lineW * 1.1f;
    const auto cap = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre);
    g.setColour (Theme::edge);
    g.fillEllipse (cap.expanded (1.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b747e), centre.x, cap.getY(),
                                             juce::Colour (0xff22272c), centre.x, cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a525b), centre.x, cap.getY(),
                                             juce::Colour (0xff2c3238), centre.x, cap.getBottom(), false));
    g.fillEllipse (cap.reduced (capR * 0.12f));

    const juce::Point<float> tip (centre.x + std::sin (angle) * capR * 0.82f, centre.y - std::cos (angle) * capR * 0.82f);
    const juce::Point<float> base (centre.x + std::sin (angle) * capR * 0.35f, centre.y - std::cos (angle) * capR * 0.35f);
    g.setColour (Theme::text);
    g.drawLine ({ base, tip }, 2.2f);
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
        g.setColour (Theme::headerTop.brighter (0.15f));
        g.fillRect (r);
        g.setColour (b.findColour (juce::TextButton::buttonOnColourId));
        g.fillRect (r.removeFromBottom (2.0f));
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
    g.setColour (b.getToggleState() ? Theme::text : Theme::dimText);
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
    g.setColour (Theme::text.withAlpha (0.8f));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (name.toUpperCase(), getLocalBounds().removeFromBottom (14), juce::Justification::centred);
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
        g.setColour (Theme::panelBottom.withAlpha (0.6f));
        g.fillRect (getLocalBounds().withTrimmedTop (headerHeight).reduced (1));
    }
}

void Panel::paint (juce::Graphics& g)
{
    Theme::drawPanelFrame (g, getLocalBounds().toFloat(), (float) headerHeight);

    // Section colour stripe down the left edge (dimmed while the section is off)
    const bool on = powerAttachment == nullptr || power.getToggleState();
    g.setColour (accent.withAlpha (on ? 0.9f : 0.35f));
    g.fillRect (getLocalBounds().toFloat().withWidth (3.0f).reduced (0.0f, 1.0f));

    if (title.isNotEmpty())
    {
        g.setColour (on ? Theme::text : Theme::dimText);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
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

RackStrip::RackStrip (APVTS& s, const juce::String& t, juce::Colour c, const juce::String& powerId,
                      std::initializer_list<std::pair<const char*, const char*>> knobIds,
                      std::initializer_list<const char*> selectorIds, std::unique_ptr<juce::Component> g)
    : title (t), colour (c), graph (std::move (g))
{
    for (auto [id, name] : knobIds)
    {
        auto* k = knobs.add (new Knob (s, id, name));
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, c);
        addAndMakeVisible (k);
    }
    for (auto* id : selectorIds)
    {
        auto* sel = selectors.add (new Selector (s, id));
        sel->setAccent (c);
        addAndMakeVisible (sel);
    }
    if (auto* gb = dynamic_cast<GraphBase*> (graph.get()))
        gb->setAccent (c);
    if (graph != nullptr)
        addAndMakeVisible (*graph);
    power.setColour (juce::ToggleButton::tickColourId, c);

    power.getProperties().set ("powerIcon", true);
    power.setTooltip ("Turn " + t + " off (removes it from the rack)");
    addAndMakeVisible (power);
    powerAttachment = std::make_unique<APVTS::ButtonAttachment> (s, powerId, power);
}

void RackStrip::setSelected (bool shouldBeSelected)
{
    selected = shouldBeSelected;
    repaint();
}

void RackStrip::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (Theme::panelTop, 0.0f, 0.0f, Theme::panelBottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 1.5f);
    Theme::drawTexture (g, b);
    g.setColour (selected ? colour.withAlpha (0.8f) : Theme::edge);
    g.drawRoundedRectangle (b, 1.5f, selected ? 1.5f : 1.0f);

    // Colour stripe down the left edge identifies the module at a glance
    g.setColour (colour.withAlpha (selected ? 1.0f : 0.6f));
    g.fillRect (b.withWidth (3.0f).reduced (0.0f, 1.0f));

    // Title tag: solid colour when selected, tinted otherwise
    const auto tag = juce::Rectangle<float> (10.0f, 10.0f, 132.0f, 28.0f);
    g.setColour (selected ? colour : colour.withAlpha (0.16f));
    g.fillRoundedRectangle (tag, 1.5f);
    g.setColour (selected ? Theme::edge : colour);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawText (title, tag.reduced (10.0f, 0.0f), juce::Justification::centredLeft);
}

void RackStrip::resized()
{
    auto r = getLocalBounds().reduced (10);

    auto right = r.removeFromRight (34);
    power.setBounds (right.removeFromTop (28));
    r.removeFromRight (8);

    auto left = r.removeFromLeft (132);
    left.removeFromTop (28 + 6);
    for (auto* sel : selectors)
    {
        sel->setBounds (left.removeFromTop (22));
        left.removeFromTop (4);
    }
    r.removeFromLeft (12);

    if (graph != nullptr)
    {
        graph->setBounds (r.removeFromLeft (juce::jmin (220, r.getWidth() / 3)));
        r.removeFromLeft (10);
    }

    const int w = r.getWidth() / juce::jmax (1, knobs.size());
    for (auto* k : knobs)
        k->setBounds (r.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (r.getHeight(), 64)));
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
    g.setGradientFill (juce::ColourGradient (selected ? Theme::headerTop.brighter (0.1f) : Theme::headerTop, 0.0f, 0.0f,
                                             selected ? Theme::headerBot.brighter (0.1f) : Theme::headerBot, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 1.5f);
    Theme::drawTexture (g, b, 0.8f);
    g.setColour (selected ? colour.withAlpha (0.9f) : Theme::edge);
    g.drawRoundedRectangle (b, 1.5f, 1.0f);

    g.setColour (isOn() ? colour : colour.withAlpha (0.35f));
    g.fillRect (b.withWidth (3.0f).reduced (0.0f, 1.0f));

    g.setColour (isOn() ? Theme::text : Theme::dimText);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
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
    strips.add (new RackStrip (s, "DISTORTION", FxColours::distortion, "distOn", { { "distDrive", "Drive" }, { "distTone", "Tone" }, { "distMix", "Mix" } },
                               { "distType" }, std::make_unique<DistGraph> (s)));
    strips.add (new RackStrip (s, "PHASER", FxColours::phaser, "phaserOn", { { "phaserRate", "Rate" }, { "phaserDepth", "Depth" },
                                                          { "phaserFeedback", "Feed" }, { "phaserMix", "Mix" } }));
    strips.add (new RackStrip (s, "CHORUS", FxColours::chorus, "chorusOn", { { "chorusRate", "Rate" }, { "chorusDepth", "Depth" }, { "chorusMix", "Mix" } }));
    strips.add (new RackStrip (s, "DELAY", FxColours::delay, "delayOn", { { "delayFeedback", "Feedback" }, { "delayTone", "Tone" }, { "delayMix", "Mix" } },
                               { "delaySync", "delayMode" }));
    strips.add (new RackStrip (s, "COMPRESSOR", FxColours::compressor, "compOn", { { "compThreshold", "Thresh" }, { "compRatio", "Ratio" },
                                                            { "compAttack", "Attack" }, { "compRelease", "Release" }, { "compGain", "Gain" } }));
    strips.add (new RackStrip (s, "REVERB", FxColours::reverb, "reverbOn", { { "reverbSize", "Size" }, { "reverbDamp", "Damp" },
                                                          { "reverbWidth", "Width" }, { "reverbMix", "Mix" } }));
    strips.add (new RackStrip (s, "EQ", FxColours::eq, "eqOn", { { "eqLow", "Low" }, { "eqMid", "Mid" }, { "eqMidFreq", "Freq" }, { "eqHigh", "High" } },
                               {}, std::make_unique<EqGraph> (s)));

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
    int y = 8;
    rack.empty = true;
    for (int i = 0; i < strips.size(); ++i)
    {
        const bool on = items[i]->isOn();
        strips[i]->setVisible (on);
        if (! on)
            continue;
        strips[i]->setBounds (8, y, w - 16, strips[i]->preferredHeight());
        y += strips[i]->preferredHeight() + 8;
        rack.empty = false;
    }
    rack.setSize (w, juce::jmax (y, viewport.getHeight()));
    rack.repaint();
}

void FxPage::paint (juce::Graphics& g)
{
    // Dark rack well behind the strips
    const auto well = rackArea.toFloat();
    g.setColour (juce::Colour (0xff101317));
    g.fillRoundedRectangle (well, 1.5f);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (well.reduced (0.5f), 1.5f, 1.0f);

    // Rail screws down both sides, like a hardware rack
    g.setColour (Theme::panelTop);
    for (int y = rackArea.getY() + 20; y < rackArea.getBottom() - 10; y += 70)
    {
        g.fillEllipse ((float) rackArea.getX() - 14.0f, (float) y, 7.0f, 7.0f);
        g.fillEllipse ((float) rackArea.getRight() + 7.0f, (float) y, 7.0f, 7.0f);
    }
}

void FxPage::resized()
{
    auto r = getLocalBounds();
    listArea = r.removeFromLeft (230);
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
VelvetPadEditor::VelvetPadEditor (VelvetPadProcessor& proc)
    : AudioProcessorEditor (proc),
      sub (proc.apvts), air (proc.apvts), oscA (proc.apvts, 0), oscB (proc.apvts, 1),
      filter (proc.apvts), env (proc.apvts), lfo (proc.apvts), pump (proc.apvts), fxPage (proc.apvts), visPage (proc.scope),
      master (proc.apvts, "master", "Master"),
      keyboard (proc.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> { &sub, &air, &oscA, &oscB, &filter, &env, &lfo, &pump, &fxPage, &visPage, &master, &keyboard, &oscTab, &fxTab, &visTab })
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
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffdde2e7));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1b1e22));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff7d858e));
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
    g.fillAll (Theme::background);
    Theme::drawTexture (g, getLocalBounds().toFloat(), 0.6f);

    auto top = getLocalBounds().removeFromTop (56).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a3036), 0.0f, 0.0f, juce::Colour (0xff1b1f24), 0.0f, top.getBottom(), false));
    g.fillRect (top);
    Theme::drawTexture (g, top);
    g.setColour (Theme::edge);
    g.drawHorizontalLine (55, 0.0f, top.getRight());

    g.setColour (Theme::gold);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("VELVET PAD", 18, 0, 260, 56, juce::Justification::centredLeft);

    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (11.5f));
    g.drawText ("WIDE PAD SYNTHESIZER", 196, 2, 200, 56, juce::Justification::centredLeft);
    g.drawText ("drag the graphs   |   double-click to reset", 630, 0, 270, 56, juce::Justification::centredRight);

    g.setColour (juce::Colour (0xff0e1013));
    g.fillRect (keyboard.getBounds().expanded (8, 6).toFloat());
}

void VelvetPadEditor::resized()
{
    master.setBounds (getWidth() - 74, 2, 60, 54);
    oscTab.setBounds (400, 14, 70, 30);
    fxTab.setBounds (474, 14, 70, 30);
    visTab.setBounds (548, 14, 70, 30);

    const int gap = 8;
    auto area = getLocalBounds().withTrimmedTop (56 + gap).reduced (gap, 0);

    auto keys = getLocalBounds().removeFromBottom (122).reduced (gap, 8);
    keyboard.setBounds (keys);
    keyboard.setKeyWidth ((float) keys.getWidth() / 43.0f); // 43 white keys from C1 to C7

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
