#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

//==============================================================================
// Snapshot of all sound parameters, refreshed by the processor once per block.
struct OscSettings
{
    int wave = 0, octave = 0, semi = 0, voices = 1;
    float fine = 0.0f, level = 0.0f, detune = 0.0f, width = 0.0f;
};

struct SynthSettings
{
    OscSettings osc[2];
    bool oscOn[2] { true, true }, subOn = true, noiseOn = true, filterOn = true, lfoOn = true;

    int subWave = 0, subOctave = 0;
    float subLevel = 0.0f, noiseLevel = 0.0f;

    int filterType = 1;
    float cutoff = 3000.0f, reso = 0.0f, envAmount = 0.0f, keyTrack = 0.0f, drive = 0.0f;

    juce::ADSR::Parameters filterEnv, ampEnv;
    float velSens = 0.5f;

    int lfoShape = 0;
    float lfoRate = 5.0f, lfoDelay = 0.0f, lfoPitch = 0.0f, lfoCutoff = 0.0f, lfoAmp = 0.0f;

    // Performance controllers, written by the synth from MIDI
    float modWheel = 0.0f, pitchBendSemis = 0.0f;
};

//==============================================================================
struct StringSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
// Zero-delay-feedback state variable filter (Cytomic / Simper topology).
struct SVF
{
    float ic1 = 0.0f, ic2 = 0.0f;

    void reset() { ic1 = ic2 = 0.0f; }

    inline void process (float v0, float a1, float a2, float a3, float k, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = v0 - k * v1 - v2;
    }
};

struct SVFCoeffs
{
    float a1 = 0, a2 = 0, a3 = 0, k = 2;

    void set (float g, float kIn) noexcept
    {
        k  = kIn;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

//==============================================================================
class StringVoice : public juce::SynthesiserVoice
{
public:
    StringVoice (const SynthSettings& s) : settings (s)
    {
        rng = 0x9E3779B9u ^ (uint32_t) reinterpret_cast<uintptr_t> (this);
    }

    void prepare (double sr)
    {
        sampleRate = sr;
        ampEnv.setSampleRate (sr);
        filterEnv.setSampleRate (sr);
    }

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<StringSound*> (s) != nullptr; }

    void startNote (int midiNote, float vel, juce::SynthesiserSound*, int) override
    {
        noteNumber = midiNote;
        velocity = vel;

        if (! ampEnv.isActive())
        {
            for (auto& f : filters) f.reset();
            // Random start phases give every note its own ensemble shimmer
            for (auto& o : phases)
                for (auto& p : o)
                    p = random01();
            subPhase = 0.0;
        }

        ampEnv.setParameters (settings.ampEnv);
        filterEnv.setParameters (settings.filterEnv);
        ampEnv.noteOn();
        filterEnv.noteOn();

        lfoTime = 0.0f;
        lfoPhase = 0.0f;   // retrigger: every note starts the LFO at the same point the graph shows
        releaseTime = -1.0f;
        startOrder = ++startCounter;
        shValue = random01() * 2.0f - 1.0f;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            releaseTime = 0.0f;
            filterEnv.noteOff();
        }
        else
        {
            ampEnv.reset();
            filterEnv.reset();
            clearCurrentNote();
        }
    }

    // Read by the processor to drive the editor's playhead dots
    uint64_t getStartOrder() const     { return startOrder; }
    float getNoteTime() const          { return lfoTime; }
    float getReleaseTime() const       { return releaseTime; }
    float getAmpEnv() const            { return lastAmpEnv; }
    float getFilterEnv() const         { return lastFilterEnv; }
    float getLfo() const               { return lastLfo; }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        const auto& s = settings;
        ampEnv.setParameters (s.ampEnv);
        filterEnv.setParameters (s.filterEnv);

        const double baseFreq = juce::MidiMessage::getMidiNoteInHertz (noteNumber);
        const float invSr = (float) (1.0 / sampleRate);

        // Per-block unison setup: detuned increments and equal-power pan gains
        int numUni[2];
        for (int o = 0; o < 2; ++o)
        {
            const auto& os = s.osc[o];
            const int n = juce::jlimit (1, maxUnison, os.voices);
            numUni[o] = s.oscOn[o] ? n : 0;
            const double semis = os.octave * 12 + os.semi + os.fine / 100.0 + s.pitchBendSemis;
            const float norm = os.level / std::sqrt ((float) n);

            for (int u = 0; u < n; ++u)
            {
                const float spread = n == 1 ? 0.0f : 2.0f * (float) u / (float) (n - 1) - 1.0f;
                const double cents = spread * os.detune * 55.0;
                incs[o][u] = baseFreq * std::pow (2.0, (semis + cents / 100.0) / 12.0) / sampleRate;

                // Square-root curve pushes the outer unison voices towards the edges for wide pads
                const float pan = std::copysign (std::sqrt (std::abs (spread)), spread) * os.width;
                gainL[o][u] = norm * std::sqrt (0.5f * (1.0f - pan));
                gainR[o][u] = norm * std::sqrt (0.5f * (1.0f + pan));
            }
        }

        const double subInc = baseFreq * std::pow (2.0, s.pitchBendSemis / 12.0)
                              * (s.subOctave == 0 ? 0.5 : 0.25) / sampleRate;
        const float subGain = s.subOn ? s.subLevel * 0.7f : 0.0f;
        const float noiseGain = s.noiseOn ? s.noiseLevel * 0.25f : 0.0f;

        const float velGain = 1.0f - s.velSens * (1.0f - velocity);
        const float lfoInc = s.lfoRate * invSr;
        const float vibratoSemis = s.lfoPitch + s.modWheel * 0.6f;
        const float keyTrackOct = s.keyTrack * (float) (noteNumber - 60) / 12.0f;
        const float maxCutoff = (float) sampleRate * 0.45f;
        const float k = 2.0f - 1.96f * s.reso;
        const float piOverSr = juce::MathConstants<float>::pi * invSr;
        const float drivePre = 1.0f + s.drive * 5.0f;
        const float driveMakeup = 1.0f / std::sqrt (drivePre);

        auto* left  = out.getWritePointer (0);
        auto* right = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            // --- LFO with delayed fade-in (natural, late-blooming string vibrato)
            float lfo = lfoValue (s.lfoShape);
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f)
            {
                lfoPhase -= 1.0f;
                shValue = random01() * 2.0f - 1.0f;
            }
            if (! s.lfoOn)
                lfo = 0.0f;
            else if (s.lfoDelay > 0.0f)
                lfo *= juce::jmin (1.0f, lfoTime / s.lfoDelay);
            lfoTime += invSr;
            if (releaseTime >= 0.0f)
                releaseTime += invSr;
            lastLfo = lfo;

            const double pitchMul = std::exp2 (lfo * vibratoSemis / 12.0f);

            // --- Oscillators
            float l = 0.0f, r = 0.0f;
            for (int o = 0; o < 2; ++o)
            {
                const int wave = s.osc[o].wave;
                for (int u = 0; u < numUni[o]; ++u)
                {
                    const double dt = incs[o][u] * pitchMul;
                    double& ph = phases[o][u];
                    const float v = oscillator (wave, (float) ph, (float) dt);
                    ph += dt;
                    if (ph >= 1.0) ph -= 1.0;
                    l += v * gainL[o][u];
                    r += v * gainR[o][u];
                }
            }

            const double subDt = subInc * pitchMul;
            const float sub = (s.subWave == 0 ? oscillator (3, (float) subPhase, (float) subDt)
                                              : oscillator (1, (float) subPhase, (float) subDt)) * subGain;
            subPhase += subDt;
            if (subPhase >= 1.0) subPhase -= 1.0;

            const float noise = (random01() * 2.0f - 1.0f) * noiseGain;
            l += sub + noise;
            r += sub + noise;

            // --- Filter
            const float fEnv = filterEnv.getNextSample();
            const float aEnv = ampEnv.getNextSample();
            lastFilterEnv = fEnv;
            lastAmpEnv = aEnv;

            const float octaves = s.envAmount * fEnv * 6.0f + lfo * s.lfoCutoff * 3.0f + keyTrackOct;
            const float fc = juce::jlimit (20.0f, maxCutoff, s.cutoff * std::exp2 (octaves));
            const float g = std::tan (fc * piOverSr);

            if (s.filterOn)
            {
                // Saturation before the filter: adds upper harmonics, the filter then tames them
                if (s.drive > 0.0f)
                {
                    l = std::tanh (l * drivePre) * driveMakeup;
                    r = std::tanh (r * drivePre) * driveMakeup;
                }

                SVFCoeffs c1;
                c1.set (g, k);
                l = runFilter (s.filterType, l, c1, g, filters[0], filters[1]);
                r = runFilter (s.filterType, r, c1, g, filters[2], filters[3]);
            }

            // --- Amp
            const float amp = aEnv * velGain * (1.0f - s.lfoAmp * (0.5f + 0.5f * lfo)) * outputGain;
            left[i] += l * amp;
            if (right != nullptr)
                right[i] += r * amp;

            if (! ampEnv.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    static constexpr int maxUnison = 7;
    static constexpr float outputGain = 0.22f;

    static inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static inline float oscillator (int wave, float t, float dt) noexcept
    {
        switch (wave)
        {
            case 0: return 2.0f * t - 1.0f - polyBlep (t, dt);                         // saw
            case 1: {                                                                    // square
                float t2 = t + 0.5f;
                if (t2 >= 1.0f) t2 -= 1.0f;
                return (t < 0.5f ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt);
            }
            case 2: return 1.0f - 4.0f * std::abs (t - 0.5f);                         // triangle
            default: return std::sin (juce::MathConstants<float>::twoPi * t);           // sine
        }
    }

    inline float lfoValue (int shape) const noexcept
    {
        switch (shape)
        {
            case 0:  return std::sin (juce::MathConstants<float>::twoPi * lfoPhase);
            case 1:  return 1.0f - 4.0f * std::abs (lfoPhase - 0.5f);
            case 2:  return 2.0f * lfoPhase - 1.0f;
            case 3:  return lfoPhase < 0.5f ? 1.0f : -1.0f;
            default: return shValue;
        }
    }

    static inline float runFilter (int type, float x, const SVFCoeffs& c1, float g, SVF& f1, SVF& f2) noexcept
    {
        float lp, bp, hp;
        f1.process (x, c1.a1, c1.a2, c1.a3, c1.k, lp, bp, hp);

        switch (type)
        {
            case 0: return lp;
            case 1: {
                // Second, non-resonant stage for a smooth 24 dB/oct slope
                SVFCoeffs c2;
                c2.set (g, 1.4142f);
                float lp2, bp2, hp2;
                f2.process (lp, c2.a1, c2.a2, c2.a3, c2.k, lp2, bp2, hp2);
                return lp2;
            }
            case 2: return bp * c1.k;
            default: return hp;
        }
    }

    inline float random01() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (float) (rng >> 8) * (1.0f / 16777216.0f);
    }

    const SynthSettings& settings;
    double sampleRate = 44100.0;

    int noteNumber = 60;
    float velocity = 1.0f;

    double phases[2][maxUnison] {};
    double incs[2][maxUnison] {};
    float gainL[2][maxUnison] {}, gainR[2][maxUnison] {};
    double subPhase = 0.0;

    SVF filters[4]; // L stage 1/2, R stage 1/2
    juce::ADSR ampEnv, filterEnv;

    float lfoPhase = 0.0f, lfoTime = 0.0f, shValue = 0.0f;
    uint32_t rng = 1;

    float releaseTime = -1.0f, lastAmpEnv = 0.0f, lastFilterEnv = 0.0f, lastLfo = 0.0f;
    uint64_t startOrder = 0;
    static inline std::atomic<uint64_t> startCounter { 0 };
};

//==============================================================================
// Tracks mod wheel and pitch bend globally so newly started voices pick them up.
class StringSynth : public juce::Synthesiser
{
public:
    explicit StringSynth (SynthSettings& s) : settings (s) {}

    void handleController (int channel, int number, int value) override
    {
        if (number == 1)
            settings.modWheel = (float) value / 127.0f;
        juce::Synthesiser::handleController (channel, number, value);
    }

    void handlePitchWheel (int channel, int value) override
    {
        settings.pitchBendSemis = (float) (value - 8192) / 8192.0f * 2.0f;
        juce::Synthesiser::handlePitchWheel (channel, value);
    }

private:
    SynthSettings& settings;
};
