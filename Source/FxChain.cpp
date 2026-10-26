#include "FxChain.h"

namespace
{
// Delay times in beats, matching the "delaySync" choices
constexpr double delayBeats[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
constexpr double maxDelaySeconds = 3.0;

float onePoleCoeff (float cutoffHz, double sr)
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoffHz / (float) sr);
}
} // namespace

FxChain::FxChain (juce::AudioProcessorValueTreeState& s)
{
    auto get = [&s] (const char* id) { return s.getRawParameterValue (id); };
    p.distOn = get ("distOn");             p.distType = get ("distType");         p.distDrive = get ("distDrive");
    p.distTone = get ("distTone");         p.distMix = get ("distMix");
    p.phaserOn = get ("phaserOn");         p.phaserRate = get ("phaserRate");     p.phaserDepth = get ("phaserDepth");
    p.phaserFeedback = get ("phaserFeedback"); p.phaserMix = get ("phaserMix");
    p.chorusOn = get ("chorusOn");         p.chorusRate = get ("chorusRate");     p.chorusDepth = get ("chorusDepth");
    p.chorusMix = get ("chorusMix");
    p.delayOn = get ("delayOn");           p.delaySync = get ("delaySync");       p.delayMode = get ("delayMode");
    p.delayFeedback = get ("delayFeedback"); p.delayTone = get ("delayTone");     p.delayMix = get ("delayMix");
    p.compOn = get ("compOn");             p.compThreshold = get ("compThreshold"); p.compRatio = get ("compRatio");
    p.compAttack = get ("compAttack");     p.compRelease = get ("compRelease");   p.compGain = get ("compGain");
    p.reverbOn = get ("reverbOn");         p.reverbSize = get ("reverbSize");     p.reverbDamp = get ("reverbDamp");
    p.reverbWidth = get ("reverbWidth");   p.reverbMix = get ("reverbMix");
    p.eqOn = get ("eqOn");                 p.eqLow = get ("eqLow");               p.eqMid = get ("eqMid");
    p.eqMidFreq = get ("eqMidFreq");       p.eqHigh = get ("eqHigh");
}

void FxChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };

    dry.setSize (2, maxBlockSize);
    phaser.prepare (spec);
    phaser.setCentreFrequency (1300.0f);
    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);

    delayBuffer.setSize (2, (int) (sr * maxDelaySeconds) + 2);
    delayBuffer.clear();
    delayWrite = 0;
    delaySamples.reset (sr, 0.15);
    delaySamples.setCurrentAndTargetValue ((float) (sr * 0.25));

    compressor.prepare (spec);
    reverb.prepare (spec);

    for (auto* band : { &eqLow, &eqMid, &eqHigh })
    {
        *band->state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (sr, 200.0, 0.707f, 1.0f);
        band->prepare (spec);
    }
    std::fill (std::begin (lastEq), std::end (lastEq), -999.0f);
}

void FxChain::process (juce::AudioBuffer<float>& buffer, double bpm)
{
    if (buffer.getNumChannels() < 2)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    const bool on[7] = { isOn (p.distOn), isOn (p.phaserOn), isOn (p.chorusOn), isOn (p.delayOn),
                         isOn (p.compOn), isOn (p.reverbOn), isOn (p.eqOn) };

    if (on[1] && ! wasOn[1]) phaser.reset();
    if (on[2] && ! wasOn[2]) chorus.reset();
    if (on[3] && ! wasOn[3]) { delayBuffer.clear(); delayLp[0] = delayLp[1] = 0.0f; }
    if (on[4] && ! wasOn[4]) compressor.reset();
    if (on[5] && ! wasOn[5]) reverb.reset();
    std::copy (std::begin (on), std::end (on), std::begin (wasOn));

    if (on[0])
        processDistortion (buffer);

    if (on[1])
    {
        phaser.setRate (p.phaserRate->load());
        phaser.setDepth (p.phaserDepth->load());
        phaser.setFeedback (p.phaserFeedback->load());
        phaser.setMix (p.phaserMix->load());
        phaser.process (ctx);
    }

    if (on[2])
    {
        chorus.setRate (p.chorusRate->load());
        chorus.setDepth (p.chorusDepth->load());
        chorus.setMix (p.chorusMix->load());
        chorus.process (ctx);
    }

    if (on[3])
        processDelay (buffer, bpm);

    if (on[4])
    {
        compressor.setThreshold (p.compThreshold->load());
        compressor.setRatio (p.compRatio->load());
        compressor.setAttack (p.compAttack->load());
        compressor.setRelease (p.compRelease->load());
        compressor.process (ctx);
        buffer.applyGain (juce::Decibels::decibelsToGain (p.compGain->load()));
    }

    if (on[5])
    {
        const float wet = p.reverbMix->load();
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = 0.5f + 0.48f * p.reverbSize->load();
        rp.damping = p.reverbDamp->load();
        rp.width = p.reverbWidth->load();
        rp.wetLevel = wet * 0.6f;
        rp.dryLevel = 1.0f - wet * 0.4f;
        reverb.setParameters (rp);
        reverb.process (ctx);
    }

    if (on[6])
    {
        updateEq();
        eqLow.process (ctx);
        eqMid.process (ctx);
        eqHigh.process (ctx);
    }
}

void FxChain::processDistortion (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const int type = (int) p.distType->load();
    const float drive = p.distDrive->load();
    const float mix = p.distMix->load();
    const float toneA = onePoleCoeff (1000.0f * std::pow (20.0f, p.distTone->load()), sampleRate);

    for (int ch = 0; ch < 2; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, n);

    double inPower = 0.0, outPower = 0.0;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            inPower += x[i] * x[i];
            float y = Dist::shape (type, x[i], drive);

            // DC blocker (the tube curve is asymmetric), then the tone low-pass
            const float dc = y - dcX[ch] + 0.995f * dcY[ch];
            dcX[ch] = y;
            dcY[ch] = dc;
            toneState[ch] += toneA * (dc - toneState[ch]);

            x[i] = toneState[ch];
            outPower += x[i] * x[i];
        }
    }

    // Auto-gain: drive changes the character, not the loudness
    if (outPower > 1e-9 && inPower > 1e-9)
    {
        const float target = juce::jlimit (0.05f, 4.0f, (float) std::sqrt (inPower / outPower));
        distAutoGain += (target - distAutoGain) * 0.15f;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        const auto* d = dry.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            x[i] = d[i] + mix * (x[i] * distAutoGain - d[i]);
    }
}

void FxChain::processDelay (juce::AudioBuffer<float>& buffer, double bpm)
{
    const int n = buffer.getNumSamples();
    const int size = delayBuffer.getNumSamples();
    const auto beats = delayBeats[juce::jlimit (0, 5, (int) p.delaySync->load())];
    delaySamples.setTargetValue ((float) juce::jlimit (16.0, (double) size - 4.0, beats * 60.0 / bpm * sampleRate));

    const bool pingPong = (int) p.delayMode->load() == 1;
    const float feedback = p.delayFeedback->load();
    const float mix = p.delayMix->load();
    const float lpA = onePoleCoeff (800.0f * std::pow (2.0f, p.delayTone->load() * 4.0f), sampleRate);

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);
    auto* bl = delayBuffer.getWritePointer (0);
    auto* br = delayBuffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        // Fractional read so tempo changes glide instead of clicking
        const float d = delaySamples.getNextValue();
        float readPos = (float) delayWrite - d;
        if (readPos < 0.0f) readPos += (float) size;
        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        const float yl = bl[i0] + frac * (bl[i1] - bl[i0]);
        const float yr = br[i0] + frac * (br[i1] - br[i0]);

        // Darken every repeat
        delayLp[0] += lpA * (yl - delayLp[0]);
        delayLp[1] += lpA * (yr - delayLp[1]);

        if (pingPong)
        {
            // Mono input enters on the left, repeats bounce L > R > L
            bl[delayWrite] = 0.5f * (l[i] + r[i]) + feedback * delayLp[1];
            br[delayWrite] = feedback * delayLp[0];
        }
        else
        {
            bl[delayWrite] = l[i] + feedback * delayLp[0];
            br[delayWrite] = r[i] + feedback * delayLp[1];
        }

        l[i] = l[i] * (1.0f - 0.3f * mix) + delayLp[0] * mix;
        r[i] = r[i] * (1.0f - 0.3f * mix) + delayLp[1] * mix;

        if (++delayWrite >= size)
            delayWrite = 0;
    }
}

void FxChain::updateEq()
{
    const float now[4] = { p.eqLow->load(), p.eqMid->load(), p.eqMidFreq->load(), p.eqHigh->load() };
    if (std::equal (std::begin (now), std::end (now), std::begin (lastEq)))
        return;
    std::copy (std::begin (now), std::end (now), std::begin (lastEq));

    using AC = juce::dsp::IIR::ArrayCoefficients<float>;
    auto gain = [] (float db) { return juce::Decibels::decibelsToGain (db); };
    *eqLow.state  = AC::makeLowShelf (sampleRate, 200.0, 0.707f, gain (now[0]));
    *eqMid.state  = AC::makePeakFilter (sampleRate, now[2], 1.0f, gain (now[1]));
    *eqHigh.state = AC::makeHighShelf (sampleRate, 6000.0, 0.707f, gain (now[3]));
}
