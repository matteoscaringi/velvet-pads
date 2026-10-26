#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{
inline juce::String pct (float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
inline juce::String secs (float v, int) { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms"
                                                          : juce::String (v, 2) + " s"; }
inline juce::String hz (float v, int)   { return v < 1000.0f ? juce::String (v, v < 10.0f ? 2 : 0) + " Hz"
                                                             : juce::String (v / 1000.0f, 1) + " kHz"; }

inline juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&] (const String& id, const String& name, NormalisableRange<float> range, float def,
                         std::function<String (float, int)> fmt)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
    };
    auto addInt = [&] (const String& id, const String& name, int lo, int hi, int def)
    {
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def));
    };
    auto addChoice = [&] (const String& id, const String& name, const StringArray& choices, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
    };

    const StringArray waves { "Saw", "Square", "Triangle", "Sine" };

    // Section power switches (the squares in each panel header). Sub, noise, LFO and pump
    // start off so a new instance is a clean two-oscillator patch.
    for (auto [id, name, on] : { std::tuple { "osc1On", "Osc 1 On", true }, { "osc2On", "Osc 2 On", true },
                                 { "subOn", "Sub On", false }, { "noiseOn", "Noise On", false },
                                 { "filterOn", "Filter On", true }, { "lfoOn", "LFO On", false },
                                 { "pumpOn", "Pump On", false },
                                 // Filter routing: which sources go through the filter (the rest bypass it)
                                 { "filterA", "Filter Osc A", true }, { "filterB", "Filter Osc B", true },
                                 { "filterN", "Filter Noise", true }, { "filterS", "Filter Sub", false },
                                 // FX rack: chorus + reverb on (part of the pad sound), the rest ready to switch on
                                 { "distOn", "Distortion On", false }, { "phaserOn", "Phaser On", false },
                                 { "chorusOn", "Chorus On", true }, { "delayOn", "Delay On", false },
                                 { "compOn", "Compressor On", false }, { "reverbOn", "Reverb On", true },
                                 { "eqOn", "EQ On", false } })
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, on));

    // Oscillators: both start in tune at the played pitch, single voice, unison controls at 50%
    for (int i = 1; i <= 2; ++i)
    {
        const String p = "osc" + String (i);
        const String n = "Osc " + String (i) + " ";
        addChoice (p + "Wave",   n + "Wave", waves, 0);
        addInt    (p + "Octave", n + "Octave", -2, 2, 0);
        addInt    (p + "Semi",   n + "Semi", -12, 12, 0);
        addFloat  (p + "Fine",   n + "Fine", { -100.0f, 100.0f }, 0.0f,
                   [] (float v, int) { return String (roundToInt (v)) + " ct"; });
        addInt    (p + "Voices", n + "Voices", 1, 7, 1);
        addFloat  (p + "Detune", n + "Detune", { 0.0f, 1.0f }, 0.5f, pct);
        addFloat  (p + "Width",  n + "Width",  { 0.0f, 1.0f }, 0.5f, pct);
        addFloat  (p + "Level",  n + "Level",  { 0.0f, 1.0f }, i == 1 ? 0.7f : 0.7f, pct);
    }

    addChoice ("subWave",    "Sub Wave", { "Sine", "Square" }, 0);
    addChoice ("subOctave",  "Sub Octave", { "-1 Oct", "-2 Oct" }, 0);
    addFloat  ("subLevel",   "Sub Level", { 0.0f, 1.0f }, 0.25f, pct);
    addFloat  ("noiseLevel", "Noise", { 0.0f, 1.0f }, 0.03f, pct);

    addChoice ("filterType", "Filter Type", { "LP 12", "LP 24", "BP", "HP" }, 1);
    addFloat  ("cutoff",     "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 4500.0f, hz);
    addFloat  ("reso",       "Resonance", { 0.0f, 1.0f }, 0.4f, pct);
    addFloat  ("filterEnv",  "Filter Env Amount", { -1.0f, 1.0f }, 0.12f, pct);
    addFloat  ("keyTrack",   "Key Track", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat  ("drive",      "Drive", { 0.0f, 1.0f }, 0.4f, pct);
    addFloat  ("lowCut",     "Low Cut", skewed (20.0f, 1000.0f, 150.0f), 160.0f, hz);

    const auto timeRange = skewed (0.001f, 10.0f, 0.8f);
    addFloat ("fAttack",  "Filter Attack",  timeRange, 0.8f, secs);
    addFloat ("fDecay",   "Filter Decay",   timeRange, 2.0f, secs);
    addFloat ("fSustain", "Filter Sustain", { 0.0f, 1.0f }, 0.6f, pct);
    addFloat ("fRelease", "Filter Release", timeRange, 1.5f, secs);

    addFloat ("aAttack",  "Amp Attack",  timeRange, 0.25f, secs);
    addFloat ("aDecay",   "Amp Decay",   timeRange, 1.5f, secs);
    addFloat ("aSustain", "Amp Sustain", { 0.0f, 1.0f }, 0.9f, pct);
    addFloat ("aRelease", "Amp Release", timeRange, 1.2f, secs);
    addFloat ("velSens",  "Velocity",    { 0.0f, 1.0f }, 0.3f, pct);

    // Slow filter drift instead of string vibrato
    addChoice ("lfoShape",  "LFO Shape", { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0);
    addFloat  ("lfoRate",   "LFO Rate", skewed (0.05f, 20.0f, 3.0f), 0.25f, hz);
    addFloat  ("lfoDelay",  "LFO Delay", skewed (0.0f, 5.0f, 1.0f), 0.0f, secs);
    addFloat  ("lfoPitch",  "LFO > Pitch", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat  ("lfoCutoff", "LFO > Cutoff", { 0.0f, 1.0f }, 0.15f, pct);
    addFloat  ("lfoAmp",    "LFO > Amp", { 0.0f, 1.0f }, 0.0f, pct);

    // Tempo-synced volume movement: classic sidechain duck, or a gate that accents each step
    addChoice ("pumpRate",  "Pump Rate", { "1/4", "1/8", "1/16" }, 2);
    addChoice ("pumpShape", "Pump Shape", { "Sidechain", "Gate" }, 1);
    addFloat  ("pumpDepth", "Pump Depth", { 0.0f, 1.0f }, 0.3f, pct);

    auto ms = [] (float v, int) { return String (roundToInt (v)) + " ms"; };
    auto db = [] (float v, int) { return String (v, 1) + " dB"; };

    addChoice ("distType",  "Distortion Type", { "Soft", "Hard", "Tube", "Fold", "Crush" }, 0);
    addFloat  ("distDrive", "Distortion Drive", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat  ("distTone",  "Distortion Tone", { 0.0f, 1.0f }, 0.7f, pct);
    addFloat  ("distMix",   "Distortion Mix", { 0.0f, 1.0f }, 1.0f, pct);

    addFloat ("phaserRate",     "Phaser Rate", skewed (0.02f, 10.0f, 1.0f), 0.3f, hz);
    addFloat ("phaserDepth",    "Phaser Depth", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat ("phaserFeedback", "Phaser Feedback", { 0.0f, 0.9f }, 0.3f, pct);
    addFloat ("phaserMix",      "Phaser Mix", { 0.0f, 1.0f }, 0.5f, pct);

    addFloat ("chorusRate",  "Chorus Rate", skewed (0.05f, 5.0f, 0.8f), 0.45f, hz);
    addFloat ("chorusDepth", "Chorus Depth", { 0.0f, 1.0f }, 0.4f, pct);
    addFloat ("chorusMix",   "Chorus Mix", { 0.0f, 1.0f }, 0.52f, pct);

    addChoice ("delaySync",     "Delay Time", { "1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" }, 2);
    addChoice ("delayMode",     "Delay Mode", { "Stereo", "Ping-Pong" }, 1);
    addFloat  ("delayFeedback", "Delay Feedback", { 0.0f, 0.95f }, 0.35f, pct);
    addFloat  ("delayTone",     "Delay Tone", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat  ("delayMix",      "Delay Mix", { 0.0f, 1.0f }, 0.25f, pct);

    addFloat ("compThreshold", "Comp Threshold", { -48.0f, 0.0f }, -18.0f, db);
    addFloat ("compRatio",     "Comp Ratio", skewed (1.0f, 20.0f, 4.0f), 4.0f,
              [] (float v, int) { return String (v, 1) + ":1"; });
    addFloat ("compAttack",    "Comp Attack", skewed (0.1f, 100.0f, 10.0f), 10.0f, ms);
    addFloat ("compRelease",   "Comp Release", skewed (10.0f, 1000.0f, 150.0f), 120.0f, ms);
    addFloat ("compGain",      "Comp Gain", { 0.0f, 24.0f }, 6.0f, db);

    addFloat ("reverbSize",  "Reverb Size", { 0.0f, 1.0f }, 0.8f, pct);
    addFloat ("reverbDamp",  "Reverb Damping", { 0.0f, 1.0f }, 0.45f, pct);
    addFloat ("reverbWidth", "Reverb Width", { 0.0f, 1.0f }, 1.0f, pct);
    addFloat ("reverbMix",   "Reverb Mix", { 0.0f, 1.0f }, 0.3f, pct);

    addFloat ("eqLow",     "EQ Low", { -12.0f, 12.0f }, 0.0f, db);
    addFloat ("eqMid",     "EQ Mid", { -12.0f, 12.0f }, 0.0f, db);
    addFloat ("eqMidFreq", "EQ Mid Freq", skewed (200.0f, 8000.0f, 1200.0f), 1000.0f, hz);
    addFloat ("eqHigh",    "EQ High", { -12.0f, 12.0f }, 0.0f, db);
    addFloat ("master",     "Master", { -36.0f, 6.0f }, -3.0f,
              [] (float v, int) { return String (v, 1) + " dB"; });

    return layout;
}
} // namespace Params
