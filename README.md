# Velvet Pad

Polyphonic (16 voices) wide pad synth for tech house — VST3 + AU + Standalone, built with JUCE 8.

**Sound path:** 2 unison supersaw oscillators (up to 7 detuned voices each, spread wide) + sub + noise
→ drive → stereo filter (LP12 / LP24 / BP / HP) with its own envelope → low cut → amp envelope
→ FX rack → tempo-synced **Pump** (1/4, 1/8, 1/16; sidechain duck or gate) → master.

**FX tab** (Serum-style, each module has its own on/off square):
Distortion (Soft / Hard / Tube / Fold / Crush, auto-gained) → Phaser → Chorus → tempo-synced Delay
(stereo or ping-pong) → Compressor → Reverb → 3-band EQ.

**VIS tab:** goniometer (stereo field with trails and auto-gain), L/R peak/RMS meters with 2 s peak hold,
phase correlation and a stereo-width readout (side/mid energy).

Every panel's square switches that section on/off. Envelope, filter, LFO, oscillator, distortion and EQ graphs are
draggable, and the envelope and LFO graphs show a playhead for the most recent note.

The default patch was tuned against a 126 BPM tech house pad stem: high-passed at 160 Hz, dark resonant
LP24 around 4.5 kHz, octave-up saw layer, wide chorus and a light 1/16 gate.

## Build

```bash
.tools/bin/cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
.tools/bin/cmake --build build --config Release -j8
```

Plugins are copied automatically to `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`.

## Source

- `Source/Params.h` — all parameters and their defaults (the default patch is a lush warm string pad)
- `Source/SynthVoice.h` — oscillators (PolyBLEP), filter, envelopes, LFO
- `Source/PluginProcessor.*` — voice management, pump, state save/restore
- `Source/FxChain.*` — the FX rack
- `Source/Visualizer.*`, `Source/ScopeFifo.h` — the VIS tab and its lock-free audio feed
- `Source/PluginEditor.*` — GUI
# velvet-pads
