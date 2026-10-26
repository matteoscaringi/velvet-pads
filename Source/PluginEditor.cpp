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
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (slider.isMouseOverOrDragging() ? Theme::accent.withAlpha (0.6f) : Theme::edge);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
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
        g.setColour (Theme::accent);
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
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.isMouseOver (true) ? Theme::accent.withAlpha (0.6f) : Theme::edge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
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
        g.setColour (Theme::accent);
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
    const auto sq = b.getLocalBounds().toFloat().withSizeKeepingCentre (9.0f, 9.0f);
    if (b.getToggleState())
    {
        g.setColour (Theme::accent.withAlpha (0.25f));
        g.fillRect (sq.expanded (2.0f));
        g.setColour (Theme::accent);
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

    if (title.isNotEmpty())
    {
        const bool on = powerAttachment == nullptr || power.getToggleState();
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
}

void FilterPanel::resized()
{
    Panel::resized();
    auto r = content();
    type.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);
    layoutRow (r.removeFromBottom (56), { &cutoff, &reso, &drive, &envAmount, &keyTrack, &lowCut });
    r.removeFromBottom (6);
    graph.setBounds (r);
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

FxModule::FxModule (APVTS& s, const juce::String& t, const juce::String& powerId,
                    std::initializer_list<std::pair<const char*, const char*>> knobIds,
                    std::initializer_list<const char*> selectorIds, std::unique_ptr<juce::Component> g)
    : Panel (t, &s, powerId), graph (std::move (g))
{
    for (auto [id, name] : knobIds)
        addAndMakeVisible (knobs.add (new Knob (s, id, name)));
    for (auto* id : selectorIds)
        addAndMakeVisible (selectors.add (new Selector (s, id)));
    if (graph != nullptr)
        addAndMakeVisible (*graph);
}

void FxModule::resized()
{
    Panel::resized();
    auto r = content();

    if (graph != nullptr)
    {
        graph->setBounds (r.removeFromLeft (juce::jmin (r.getWidth() * 45 / 100, r.getHeight() * 2)));
        r.removeFromLeft (8);
    }

    if (! selectors.isEmpty())
    {
        auto row = r.removeFromTop (22);
        const int w = (row.getWidth() - 6 * (selectors.size() - 1)) / selectors.size();
        for (auto* sel : selectors)
        {
            sel->setBounds (row.removeFromLeft (w));
            row.removeFromLeft (6);
        }
        r.removeFromTop (6);
    }

    const int w = r.getWidth() / juce::jmax (1, knobs.size());
    for (auto* k : knobs)
        k->setBounds (r.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (r.getHeight(), 64)));
}

FxPage::FxPage (APVTS& s)
    : distortion (s, "DISTORTION", "distOn", { { "distDrive", "Drive" }, { "distTone", "Tone" }, { "distMix", "Mix" } },
                  { "distType" }, std::make_unique<DistGraph> (s)),
      phaser (s, "PHASER", "phaserOn", { { "phaserRate", "Rate" }, { "phaserDepth", "Depth" },
                                         { "phaserFeedback", "Feedback" }, { "phaserMix", "Mix" } }),
      chorus (s, "CHORUS", "chorusOn", { { "chorusRate", "Rate" }, { "chorusDepth", "Depth" }, { "chorusMix", "Mix" } }),
      delay (s, "DELAY", "delayOn", { { "delayFeedback", "Feedback" }, { "delayTone", "Tone" }, { "delayMix", "Mix" } },
             { "delaySync", "delayMode" }),
      compressor (s, "COMPRESSOR", "compOn", { { "compThreshold", "Thresh" }, { "compRatio", "Ratio" },
                                               { "compAttack", "Attack" }, { "compRelease", "Release" }, { "compGain", "Gain" } }),
      reverb (s, "REVERB", "reverbOn", { { "reverbSize", "Size" }, { "reverbDamp", "Damp" },
                                         { "reverbWidth", "Width" }, { "reverbMix", "Mix" } }),
      eq (s, "EQ", "eqOn", { { "eqLow", "Low" }, { "eqMid", "Mid" }, { "eqMidFreq", "Freq" }, { "eqHigh", "High" } },
          {}, std::make_unique<EqGraph> (s))
{
    for (auto* m : { &distortion, &phaser, &chorus, &delay, &compressor, &reverb, &eq })
        addAndMakeVisible (m);
}

void FxPage::resized()
{
    const int gap = 8;
    auto r = getLocalBounds();
    const int rowH = (r.getHeight() - 2 * gap) / 3;
    const int half = (r.getWidth() - gap) / 2;

    auto row1 = r.removeFromTop (rowH);
    r.removeFromTop (gap);
    distortion.setBounds (row1.removeFromLeft (half));
    row1.removeFromLeft (gap);
    const int third = (row1.getWidth() - gap) / 2;
    phaser.setBounds (row1.removeFromLeft (third));
    row1.removeFromLeft (gap);
    chorus.setBounds (row1);

    auto row2 = r.removeFromTop (rowH);
    r.removeFromTop (gap);
    delay.setBounds (row2.removeFromLeft (half));
    row2.removeFromLeft (gap);
    reverb.setBounds (row2);

    compressor.setBounds (r.removeFromLeft (half));
    r.removeFromLeft (gap);
    eq.setBounds (r);
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

    auto top = getLocalBounds().removeFromTop (56).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a3036), 0.0f, 0.0f, juce::Colour (0xff1b1f24), 0.0f, top.getBottom(), false));
    g.fillRect (top);
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
