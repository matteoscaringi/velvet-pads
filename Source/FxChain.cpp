#include "FxChain.h"

namespace
{
// Delay times in beats, matching the "delaySync" choices
constexpr double delayBeats[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
constexpr double maxDelaySeconds = 3.0;

// Reverb line and diffuser lengths (ms) per mode, all mutually prime-ish to avoid ringing
constexpr float lineMs[3][8] = {
    { 31.7f, 37.3f, 41.1f, 47.3f, 53.9f, 61.1f, 67.7f, 77.3f },   // hall
    { 13.1f, 17.3f, 19.9f, 23.3f, 26.9f, 29.3f, 33.7f, 37.9f },   // plate
    {  8.9f, 10.7f, 13.3f, 15.1f, 17.9f, 19.7f, 22.3f, 25.1f },   // room
};
constexpr float diffuserMs[3][4] = { { 4.71f, 3.59f, 2.73f, 1.87f }, { 3.1f, 2.4f, 1.9f, 1.3f }, { 2.3f, 1.7f, 1.3f, 0.9f } };
constexpr float diffuserGain[3] = { 0.7f, 0.75f, 0.6f };

// In-place fast Walsh-Hadamard transform of 8 values, normalised (energy preserving)
void hadamard8 (float* x)
{
    for (int len = 1; len < 8; len <<= 1)
        for (int i = 0; i < 8; i += len << 1)
            for (int j = i; j < i + len; ++j)
            {
                const float a = x[j], b = x[j + len];
                x[j] = a + b;
                x[j + len] = a - b;
            }
    for (int i = 0; i < 8; ++i)
        x[i] *= 0.35355339f;
}
} // namespace

//==============================================================================
void FdnReverb::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& d : preDelay) d.resize ((int) (sr * 0.3) + 4);
    for (auto& l : lines)    l.resize ((int) (sr * 0.2) + 4);   // longest line (hall, max size) plus modulation
    for (auto& ch : diffusers)
        for (auto& d : ch) d.resize ((int) (sr * 0.02) + 4);
    reset();
}

void FdnReverb::reset()
{
    for (auto& d : preDelay) std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
    for (auto& l : lines)    std::fill (l.buffer.begin(), l.buffer.end(), 0.0f);
    for (auto& ch : diffusers)
        for (auto& d : ch) std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
    std::fill (std::begin (damp), std::end (damp), 0.0f);
    for (int i = 0; i < numLines; ++i)
        spinPhase[i] = (float) i / (float) numLines;
    lowCut.reset();
    highCut.reset();
}

void FdnReverb::process (const float* inL, const float* inR, float* wetL, float* wetR, int n, const Settings& s)
{
    const int mode = juce::jlimit (0, 2, s.mode);
    const float scale = 0.5f + 0.8f * s.size;
    const float msToSamples = (float) sr * 0.001f;

    float len[numLines], gain[numLines];
    for (int i = 0; i < numLines; ++i)
    {
        len[i] = lineMs[mode][i] * scale * msToSamples;
        // Gain per pass so the tail falls 60 dB in decaySeconds. The 1.16 compensates for the small
        // extra loss from damping and modulated (interpolated) reads, measured against real RT60.
        gain[i] = std::pow (10.0f, -3.0f * len[i] / (juce::jmax (0.1f, s.decaySeconds) * 1.16f * (float) sr));
    }
    float diffLen[numDiffusers];
    for (int i = 0; i < numDiffusers; ++i)
        diffLen[i] = juce::jmax (1.0f, diffuserMs[mode][i] * scale * msToSamples);
    const float dg = diffuserGain[mode];

    const float pre = juce::jmax (1.0f, s.preDelayMs * msToSamples);
    const float dampA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * s.highCut / (float) sr);
    const float spinInc = s.spinHz / (float) sr;
    const float spinAmount = s.spinDepth * 0.012f;   // up to +/-1.2% of each line's length
    lowCut.set (s.lowCut, 0.707f, sr);
    highCut.set (s.highCut, 0.707f, sr);

    for (int i = 0; i < n; ++i)
    {
        // Pre-delay, then input diffusion (Schroeder allpasses) per channel
        float x[2] = { inL[i], inR[i] };
        for (int ch = 0; ch < 2; ++ch)
        {
            preDelay[ch].push (x[ch]);
            float v = preDelay[ch].read (pre);
            for (int d = 0; d < numDiffusers; ++d)
            {
                const float delayed = diffusers[ch][d].read (diffLen[d]);
                const float w = v + dg * delayed;
                diffusers[ch][d].push (w);
                v = delayed - dg * w;
            }
            x[ch] = v;
        }

        // Read the network with slowly modulated delay times, damp each line
        float y[numLines];
        for (int l = 0; l < numLines; ++l)
        {
            const float mod = 1.0f + spinAmount * std::sin (juce::MathConstants<float>::twoPi * spinPhase[l]);
            spinPhase[l] += spinInc * (1.0f + 0.13f * (float) l);   // each line spins at a slightly different rate
            if (spinPhase[l] >= 1.0f) spinPhase[l] -= 1.0f;
            damp[l] += dampA * (lines[l].read (len[l] * mod) - damp[l]);
            y[l] = damp[l];
        }

        float fb[numLines];
        for (int l = 0; l < numLines; ++l)
            fb[l] = y[l] * gain[l];
        hadamard8 (fb);

        // Left feeds the even lines, right the odd ones
        for (int l = 0; l < numLines; ++l)
            lines[l].push (fb[l] + x[l & 1] * 0.5f);

        float outL = (y[0] + y[2] + y[4] + y[6]) * 0.5f;
        float outR = (y[1] + y[3] + y[5] + y[7]) * 0.5f;
        outL = highCut.process (0, lowCut.process (0, outL, 2), 0);
        outR = highCut.process (1, lowCut.process (1, outR, 2), 0);

        const float mid = 0.5f * (outL + outR), side = 0.5f * (outL - outR) * s.width;
        wetL[i] = mid + side;
        wetR[i] = mid - side;
    }
}

//==============================================================================
FxChain::FxChain (juce::AudioProcessorValueTreeState& s) : state (s)
{
    p.distOn = raw ("distOn");               p.distType = raw ("distType");           p.distDrive = raw ("distDrive");
    p.distFilterMode = raw ("distFilterMode"); p.distFilterType = raw ("distFilterType");
    p.distFilterFreq = raw ("distFilterFreq"); p.distFilterQ = raw ("distFilterQ");   p.distMix = raw ("distMix");
    p.phaserOn = raw ("phaserOn");           p.phaserRate = raw ("phaserRate");       p.phaserDepth = raw ("phaserDepth");
    p.phaserFreq = raw ("phaserFreq");       p.phaserFeedback = raw ("phaserFeedback"); p.phaserMix = raw ("phaserMix");
    p.chorusOn = raw ("chorusOn");           p.chorusRate = raw ("chorusRate");       p.chorusDepth = raw ("chorusDepth");
    p.chorusDelay = raw ("chorusDelay");     p.chorusFeedback = raw ("chorusFeedback"); p.chorusMix = raw ("chorusMix");
    p.delayOn = raw ("delayOn");             p.delaySync = raw ("delaySync");         p.delayMode = raw ("delayMode");
    p.delayFeedback = raw ("delayFeedback"); p.delayLowCut = raw ("delayLowCut");     p.delayHighCut = raw ("delayHighCut");
    p.delayMix = raw ("delayMix");
    p.compOn = raw ("compOn");               p.compThreshold = raw ("compThreshold"); p.compRatio = raw ("compRatio");
    p.compAttack = raw ("compAttack");       p.compRelease = raw ("compRelease");     p.compGain = raw ("compGain");
    p.compMix = raw ("compMix");
    p.reverbOn = raw ("reverbOn");           p.reverbMode = raw ("reverbMode");       p.reverbPreDelay = raw ("reverbPreDelay");
    p.reverbSize = raw ("reverbSize");       p.reverbDecay = raw ("reverbDecay");     p.reverbLowCut = raw ("reverbLowCut");
    p.reverbHighCut = raw ("reverbHighCut"); p.reverbSpin = raw ("reverbSpin");       p.reverbSpinDepth = raw ("reverbSpinDepth");
    p.reverbWidth = raw ("reverbWidth");     p.reverbMix = raw ("reverbMix");
    p.eqOn = raw ("eqOn");                   p.eqLow = raw ("eqLow");                 p.eqMid = raw ("eqMid");
    p.eqMidFreq = raw ("eqMidFreq");         p.eqMidQ = raw ("eqMidQ");               p.eqHigh = raw ("eqHigh");
}

void FxChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };

    dry.setSize (2, maxBlockSize);
    wet.setSize (2, maxBlockSize);
    distFilter.reset();

    phaser.prepare (spec);
    chorus.prepare (spec);

    delayBuffer.setSize (2, (int) (sr * maxDelaySeconds) + 2);
    delayBuffer.clear();
    delayWrite = 0;
    delayLowCutFilter.reset();
    delayHighCutFilter.reset();
    delaySamples.reset (sr, 0.15);
    delaySamples.setCurrentAndTargetValue ((float) (sr * 0.25));

    compressor.prepare (spec);
    reverb.prepare (sr);

    for (auto* band : { &eqLow, &eqMid, &eqHigh })
    {
        *band->state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (sr, 200.0, 0.707f, 1.0f);
        band->prepare (spec);
    }
    std::fill (std::begin (lastEq), std::end (lastEq), -999.0f);
}

void FxChain::reset()
{
    distFilter.reset();
    std::fill (std::begin (dcX), std::end (dcX), 0.0f);
    std::fill (std::begin (dcY), std::end (dcY), 0.0f);
    distAutoGain = 1.0f;
    phaser.reset();
    chorus.reset();
    delayBuffer.clear();
    delayLowCutFilter.reset();
    delayHighCutFilter.reset();
    compressor.reset();
    reverb.reset();
    eqLow.reset();
    eqMid.reset();
    eqHigh.reset();
}

void FxChain::process (juce::AudioBuffer<float>& buffer, double bpm)
{
    if (buffer.getNumChannels() < 2)
        return;

    // Hosts may send a bigger block than announced in prepare(): split it so the
    // scratch buffers (dry / wet) are never written past their end
    const int maxChunk = dry.getNumSamples();
    if (maxChunk <= 0)
        return;   // not prepared yet (or prepared with a zero block size): pass audio through untouched
    if (buffer.getNumSamples() > maxChunk)
    {
        for (int start = 0; start < buffer.getNumSamples(); start += maxChunk)
        {
            const int len = juce::jmin (maxChunk, buffer.getNumSamples() - start);
            float* chans[2] = { buffer.getWritePointer (0, start), buffer.getWritePointer (1, start) };
            juce::AudioBuffer<float> chunk (chans, 2, len);
            process (chunk, bpm);
        }
        return;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    const int n = buffer.getNumSamples();

    const bool on[7] = { isOn (p.distOn), isOn (p.phaserOn), isOn (p.chorusOn), isOn (p.delayOn),
                         isOn (p.compOn), isOn (p.reverbOn), isOn (p.eqOn) };

    if (on[0] && ! wasOn[0]) distFilter.reset();
    if (on[1] && ! wasOn[1]) phaser.reset();
    if (on[2] && ! wasOn[2]) chorus.reset();
    if (on[3] && ! wasOn[3]) { delayBuffer.clear(); delayLowCutFilter.reset(); delayHighCutFilter.reset(); }
    if (on[4] && ! wasOn[4]) compressor.reset();
    if (on[5] && ! wasOn[5]) reverb.reset();
    std::copy (std::begin (on), std::end (on), std::begin (wasOn));

    if (on[0])
        processDistortion (buffer);

    if (on[1])
    {
        phaser.setRate (get (p.phaserRate));
        phaser.setDepth (get (p.phaserDepth));
        phaser.setCentreFrequency (get (p.phaserFreq));
        phaser.setFeedback (get (p.phaserFeedback));
        phaser.setMix (get (p.phaserMix));
        phaser.process (ctx);
    }

    if (on[2])
    {
        chorus.setRate (get (p.chorusRate));
        chorus.setDepth (get (p.chorusDepth));
        chorus.setCentreDelay (get (p.chorusDelay));
        chorus.setFeedback (get (p.chorusFeedback));
        chorus.setMix (get (p.chorusMix));
        chorus.process (ctx);
    }

    if (on[3])
        processDelay (buffer, bpm);

    if (on[4])
    {
        // Parallel compression: Mix blends the squashed signal with the untouched one
        for (int ch = 0; ch < 2; ++ch)
            dry.copyFrom (ch, 0, buffer, ch, 0, n);

        compressor.setThreshold (get (p.compThreshold));
        compressor.setRatio (get (p.compRatio));
        compressor.setAttack (get (p.compAttack));
        compressor.setRelease (get (p.compRelease));
        compressor.process (ctx);

        const float makeup = juce::Decibels::decibelsToGain (get (p.compGain));
        const float mix = get (p.compMix);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            const auto* d = dry.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
                x[i] = d[i] + mix * (x[i] * makeup - d[i]);
        }
    }

    if (on[5])
        processReverb (buffer);

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
    const int type = (int) get (p.distType);
    const float drive = get (p.distDrive);
    const float mix = get (p.distMix);
    const int filterMode = (int) get (p.distFilterMode);       // 0 off, 1 pre, 2 post
    const int filterType = (int) get (p.distFilterType);
    distFilter.set (get (p.distFilterFreq), get (p.distFilterQ), sampleRate);

    for (int ch = 0; ch < 2; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, n);

    double inPower = 0.0, outPower = 0.0;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            inPower += x[i] * x[i];
            float v = x[i];
            if (filterMode == 1) v = distFilter.process (ch, v, filterType);
            v = Dist::shape (type, v, drive);

            // DC blocker (the tube curve is asymmetric)
            const float dc = v - dcX[ch] + 0.995f * dcY[ch];
            dcX[ch] = v;
            dcY[ch] = dc;
            v = dc;

            if (filterMode == 2) v = distFilter.process (ch, v, filterType);
            x[i] = v;
            outPower += v * v;
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
    const auto beats = delayBeats[juce::jlimit (0, 5, (int) get (p.delaySync))];
    delaySamples.setTargetValue ((float) juce::jlimit (16.0, (double) size - 4.0, beats * 60.0 / bpm * sampleRate));

    const bool pingPong = (int) get (p.delayMode) == 1;
    const float feedback = get (p.delayFeedback);
    const float mix = get (p.delayMix);
    delayLowCutFilter.set (get (p.delayLowCut), 0.707f, sampleRate);
    delayHighCutFilter.set (get (p.delayHighCut), 0.707f, sampleRate);

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);
    auto* bl = delayBuffer.getWritePointer (0);
    auto* br = delayBuffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        // Fractional read so tempo changes glide instead of clicking. Integer index maths so
        // rounding can never index past the end of the buffer.
        const float d = delaySamples.getNextValue();
        const int whole = (int) d;
        const float frac = d - (float) whole;
        int i0 = delayWrite - whole;
        if (i0 < 0) i0 += size;
        int i1 = i0 - 1;
        if (i1 < 0) i1 += size;

        // Every repeat passes the low and high cut, so echoes thin out and darken
        float yl = bl[i0] + frac * (bl[i1] - bl[i0]);
        float yr = br[i0] + frac * (br[i1] - br[i0]);
        yl = delayHighCutFilter.process (0, delayLowCutFilter.process (0, yl, 2), 0);
        yr = delayHighCutFilter.process (1, delayLowCutFilter.process (1, yr, 2), 0);

        if (pingPong)
        {
            // Mono input enters on the left, repeats bounce L > R > L
            bl[delayWrite] = 0.5f * (l[i] + r[i]) + feedback * yr;
            br[delayWrite] = feedback * yl;
        }
        else
        {
            bl[delayWrite] = l[i] + feedback * yl;
            br[delayWrite] = r[i] + feedback * yr;
        }

        l[i] = l[i] * (1.0f - 0.3f * mix) + yl * mix;
        r[i] = r[i] * (1.0f - 0.3f * mix) + yr * mix;

        if (++delayWrite >= size)
            delayWrite = 0;
    }
}

void FxChain::processReverb (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    FdnReverb::Settings s;
    s.mode = (int) get (p.reverbMode);
    s.preDelayMs = get (p.reverbPreDelay);
    s.size = get (p.reverbSize);
    s.decaySeconds = get (p.reverbDecay);
    s.lowCut = get (p.reverbLowCut);
    s.highCut = get (p.reverbHighCut);
    s.spinHz = get (p.reverbSpin);
    s.spinDepth = get (p.reverbSpinDepth);
    s.width = get (p.reverbWidth);

    reverb.process (buffer.getReadPointer (0), buffer.getReadPointer (1),
                    wet.getWritePointer (0), wet.getWritePointer (1), n, s);

    const float mix = get (p.reverbMix);
    const float dryGain = 1.0f - 0.3f * mix, wetGain = 1.2f * mix;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);
        const auto* w = wet.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            x[i] = x[i] * dryGain + w[i] * wetGain;
    }
}

void FxChain::updateEq()
{
    const float now[5] = { get (p.eqLow), get (p.eqMid), get (p.eqMidFreq), get (p.eqMidQ), get (p.eqHigh) };
    if (std::equal (std::begin (now), std::end (now), std::begin (lastEq)))
        return;
    std::copy (std::begin (now), std::end (now), std::begin (lastEq));

    using AC = juce::dsp::IIR::ArrayCoefficients<float>;
    auto gain = [] (float db) { return juce::Decibels::decibelsToGain (db); };
    *eqLow.state  = AC::makeLowShelf (sampleRate, 200.0, 0.707f, gain (now[0]));
    *eqMid.state  = AC::makePeakFilter (sampleRate, now[2], now[3], gain (now[1]));
    *eqHigh.state = AC::makeHighShelf (sampleRate, 6000.0, 0.707f, gain (now[4]));
}
