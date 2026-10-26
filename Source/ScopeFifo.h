#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// Lock-free hand-off of the plugin's final output from the audio thread to the
// VIS tab. Only active while the VIS tab is showing; when full, new audio is dropped.
class ScopeFifo
{
public:
    void push (const juce::AudioBuffer<float>& buffer)
    {
        if (! enabled.load (std::memory_order_relaxed))
            return;

        const int n = buffer.getNumSamples();
        const auto* l = buffer.getReadPointer (0);
        const auto* r = buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0);

        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        std::copy (l, l + n1, left.begin() + s1);
        std::copy (r, r + n1, right.begin() + s1);
        std::copy (l + n1, l + n1 + n2, left.begin() + s2);
        std::copy (r + n1, r + n1 + n2, right.begin() + s2);
        fifo.finishedWrite (n1 + n2);
    }

    // Message thread: copies up to maxSamples into l/r, returns how many
    int pull (float* l, float* r, int maxSamples)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (juce::jmin (maxSamples, fifo.getNumReady()), s1, n1, s2, n2);
        std::copy (left.begin() + s1, left.begin() + s1 + n1, l);
        std::copy (right.begin() + s1, right.begin() + s1 + n1, r);
        std::copy (left.begin() + s2, left.begin() + s2 + n2, l + n1);
        std::copy (right.begin() + s2, right.begin() + s2 + n2, r + n1);
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }

    void setEnabled (bool shouldBeEnabled)
    {
        if (! shouldBeEnabled)
            fifo.reset();
        enabled = shouldBeEnabled;
    }

private:
    static constexpr int capacity = 16384;
    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> left {}, right {};
    std::atomic<bool> enabled { false };
};
