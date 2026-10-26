#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"

VelvetPadProcessor::VelvetPadProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new StringVoice (settings));
    synth.addSound (new StringSound());

    auto get = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    const char* oscIds[] = { "Wave", "Octave", "Semi", "Fine", "Voices", "Detune", "Width", "Level" };
    for (int o = 0; o < 2; ++o)
        for (int i = 0; i < 8; ++i)
            p.osc[o][i] = get ("osc" + juce::String (o + 1) + oscIds[i]);

    p.subWave = get ("subWave");      p.subOctave = get ("subOctave");
    p.subLevel = get ("subLevel");    p.noiseLevel = get ("noiseLevel");
    p.filterType = get ("filterType"); p.cutoff = get ("cutoff");  p.reso = get ("reso");
    p.filterEnv = get ("filterEnv");  p.keyTrack = get ("keyTrack");
    p.fAttack = get ("fAttack");      p.fDecay = get ("fDecay");
    p.fSustain = get ("fSustain");    p.fRelease = get ("fRelease");
    p.aAttack = get ("aAttack");      p.aDecay = get ("aDecay");
    p.aSustain = get ("aSustain");    p.aRelease = get ("aRelease");
    p.velSens = get ("velSens");
    p.lfoShape = get ("lfoShape");    p.lfoRate = get ("lfoRate");   p.lfoDelay = get ("lfoDelay");
    p.lfoPitch = get ("lfoPitch");    p.lfoCutoff = get ("lfoCutoff"); p.lfoAmp = get ("lfoAmp");
    p.master = get ("master");
    p.osc1On = get ("osc1On");        p.osc2On = get ("osc2On");
    p.subOn = get ("subOn");          p.noiseOn = get ("noiseOn");
    p.filterOn = get ("filterOn");    p.lfoOn = get ("lfoOn");
    p.lowCut = get ("lowCut");        p.drive = get ("drive");
    p.filterA = get ("filterA");      p.filterB = get ("filterB");
    p.filterN = get ("filterN");      p.filterS = get ("filterS");
    p.wheelCutoff = get ("wheelCutoff"); p.wheelVibrato = get ("wheelVibrato");        p.pumpOn = get ("pumpOn");    p.pumpRate = get ("pumpRate");
    p.pumpShape = get ("pumpShape");  p.pumpDepth = get ("pumpDepth");
}

void VelvetPadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<StringVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate);

    fx.prepare (sampleRate, samplesPerBlock);

    masterGain.reset (sampleRate, 0.05);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.master->load()));
}

bool VelvetPadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void VelvetPadProcessor::updateSettings()
{
    for (int o = 0; o < 2; ++o)
    {
        auto** op = p.osc[o];
        auto& os = settings.osc[o];
        os.wave   = (int) op[0]->load();
        os.octave = (int) op[1]->load();
        os.semi   = (int) op[2]->load();
        os.fine   = op[3]->load();
        os.voices = (int) op[4]->load();
        os.detune = op[5]->load();
        os.width  = op[6]->load();
        os.level  = op[7]->load();
    }

    settings.subWave    = (int) p.subWave->load();
    settings.subOctave  = (int) p.subOctave->load();
    settings.subLevel   = p.subLevel->load();
    settings.noiseLevel = p.noiseLevel->load();

    settings.filterType = (int) p.filterType->load();
    settings.cutoff     = p.cutoff->load();
    settings.reso       = p.reso->load();
    settings.envAmount  = p.filterEnv->load();
    settings.keyTrack   = p.keyTrack->load();
    settings.drive      = p.drive->load();
    settings.lowCut     = p.lowCut->load();

    settings.filterEnv = { p.fAttack->load(), p.fDecay->load(), p.fSustain->load(), p.fRelease->load() };
    settings.ampEnv    = { p.aAttack->load(), p.aDecay->load(), p.aSustain->load(), p.aRelease->load() };
    settings.velSens   = p.velSens->load();

    settings.lfoShape  = (int) p.lfoShape->load();
    settings.lfoRate   = p.lfoRate->load();
    settings.lfoDelay  = p.lfoDelay->load();
    settings.lfoPitch  = p.lfoPitch->load();
    settings.lfoCutoff = p.lfoCutoff->load();
    settings.lfoAmp    = p.lfoAmp->load();

    settings.oscOn[0] = p.osc1On->load() > 0.5f;
    settings.oscOn[1] = p.osc2On->load() > 0.5f;
    settings.subOn    = p.subOn->load() > 0.5f;
    settings.noiseOn  = p.noiseOn->load() > 0.5f;
    settings.filterOn = p.filterOn->load() > 0.5f;
    settings.lfoOn    = p.lfoOn->load() > 0.5f;

    settings.filterOsc[0] = p.filterA->load() > 0.5f;
    settings.filterOsc[1] = p.filterB->load() > 0.5f;
    settings.filterNoise  = p.filterN->load() > 0.5f;
    settings.filterSub    = p.filterS->load() > 0.5f;

    settings.modWheel     = modWheel.load();
    settings.wheelCutoff  = p.wheelCutoff->load();
    settings.wheelVibrato = p.wheelVibrato->load();
}

void VelvetPadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    updateSettings();
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());


    double bpm = 126.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    fx.process (buffer, bpm);

    applyPump (buffer);
    publishMonitor();

    masterGain.setTargetValue (juce::Decibels::decibelsToGain (p.master->load()));
    masterGain.applyGain (buffer, buffer.getNumSamples());
    protectOutput (buffer);
    scope.push (buffer);

    // Master meters: keep the highest peak until the editor collects it
    for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
    {
        const float peak = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
        float prev = outputPeak[ch].load();
        while (peak > prev && ! outputPeak[ch].compare_exchange_weak (prev, peak)) {}
    }
}

void VelvetPadProcessor::applyPump (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    double bpm = 126.0;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto ppq = pos->getPpqPosition(); ppq && pos->getIsPlaying())
                pumpBeats = *ppq; // lock to the host's bar grid
        }

    const bool on = p.pumpOn->load() > 0.5f;
    const float depth = on ? p.pumpDepth->load() : 0.0f;
    const int rateIndex = (int) p.pumpRate->load();
    const double stepBeats = rateIndex == 0 ? 1.0 : (rateIndex == 1 ? 0.5 : 0.25);
    const bool gate = (int) p.pumpShape->load() == 1;
    const double beatsPerSample = bpm / 60.0 / currentSampleRate;
    const float smooth = 1.0f - std::exp (-1.0f / (0.003f * (float) currentSampleRate));

    for (int i = 0; i < n; ++i)
    {
        const double step = pumpBeats / stepBeats;
        const float ph = (float) (step - std::floor (step));

        // Sidechain: ducked at the step start, recovers by ~65% of the step.
        // Gate: loud at the step start, fades towards the end.
        const float duck = gate ? std::pow (ph, 1.3f)
                                : juce::square (juce::jmax (0.0f, 1.0f - ph / 0.65f));
        const float target = 1.0f - depth * duck;
        pumpGain += (target - pumpGain) * smooth;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[i] *= pumpGain;

        pumpBeats += beatsPerSample;
    }
}

void VelvetPadProcessor::protectOutput (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (2, buffer.getNumChannels());

    // 1. If anything upstream produced NaN/Inf, output silence and reset every state that could hold it
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* x = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (x[i]))
            {
                buffer.clear();
                fx.reset();
                synth.allNotesOff (0, false);
                limiterGain = 1.0f;
                return;
            }
    }

    // 2. Peak limiter at -0.3 dBFS: instant attack, 80 ms release. Inaudible at normal levels,
    //    but nothing (a huge resonance, extreme FX settings) can ever blast the speakers.
    constexpr float ceiling = 0.966f;
    const float release = 1.0f - std::exp (-1.0f / (0.08f * (float) currentSampleRate));
    auto* l = buffer.getWritePointer (0);
    auto* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float peak = juce::jmax (std::abs (l[i]), r != nullptr ? std::abs (r[i]) : 0.0f);
        const float target = peak > ceiling ? ceiling / peak : 1.0f;
        limiterGain = target < limiterGain ? target : limiterGain + (target - limiterGain) * release;
        l[i] *= limiterGain;
        if (r != nullptr) r[i] *= limiterGain;
    }
}

void VelvetPadProcessor::publishMonitor()
{
    const StringVoice* newest = nullptr;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<const StringVoice*> (synth.getVoice (i)); v != nullptr && v->isVoiceActive())
            if (newest == nullptr || v->getStartOrder() > newest->getStartOrder())
                newest = v;

    if (newest != nullptr)
    {
        monitor.ampEnv = newest->getAmpEnv();
        monitor.filterEnv = newest->getFilterEnv();
        monitor.noteTime = newest->getNoteTime();
        monitor.releaseTime = newest->getReleaseTime();
        monitor.lfoValue = newest->getLfo();
    }
    monitor.active = newest != nullptr;
}

juce::AudioProcessorEditor* VelvetPadProcessor::createEditor()
{
    return new VelvetPadEditor (*this);
}

void VelvetPadProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VelvetPadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VelvetPadProcessor();
}
