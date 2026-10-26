#pragma once

#include <atomic>

// What the most recently played voice is doing, published by the audio thread
// after every block so the editor can draw playhead dots on the env/LFO graphs.
struct VoiceMonitor
{
    std::atomic<bool> active { false };
    std::atomic<float> ampEnv { 0.0f }, filterEnv { 0.0f };
    std::atomic<float> noteTime { 0.0f };       // seconds since note-on
    std::atomic<float> releaseTime { -1.0f };   // seconds since note-off, -1 while held
    std::atomic<float> lfoValue { 0.0f };       // LFO output including its fade-in
};
