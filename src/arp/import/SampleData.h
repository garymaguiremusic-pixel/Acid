#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

//==============================================================================
/**
    A decoded sample, ready for playback.

    The audio is stored with kPad frames of silence before and after it, so the
    4-point interpolator can read neighbours at the edges without bounds checks.

    Lifetime: SampleManager owns every SampleData. The audio thread only holds raw
    pointers. Each voice that plays a sample increments voiceRefs, and the audio
    thread sets 'retired' when it swaps a sample out. SampleManager deletes a
    sample on a non-audio thread once it is retired and no voice uses it, so the
    audio thread never frees memory.
*/
struct SampleData
{
    static constexpr int kPad = 4;

    juce::AudioBuffer<float> buffer;    // numChannels (1 or 2) x (numFrames + 2 * kPad)
    int numFrames = 0;
    double sampleRate = 44100.0;

    mutable std::atomic<int> voiceRefs { 0 };      // bookkeeping, so mutable on const samples
    mutable std::atomic<bool> retired { false };

    bool isEmpty() const noexcept               { return numFrames <= 0; }
    int getNumChannels() const noexcept         { return buffer.getNumChannels(); }

    /** Pointer to frame 0 of a channel (mono samples return channel 0 for both). */
    const float* getChannel (int channel) const noexcept
    {
        return buffer.getReadPointer (juce::jmin (channel, buffer.getNumChannels() - 1)) + kPad;
    }

    /** Allocates a zeroed, padded buffer for the given size. */
    void allocate (int channels, int frames, double rate)
    {
        numFrames = frames;
        sampleRate = rate;
        buffer.setSize (juce::jlimit (1, 2, channels), frames + 2 * kPad);
        buffer.clear();
    }

    /** Writable pointer to frame 0 of a channel, used while building the sample. */
    float* getWritePointer (int channel) noexcept  { return buffer.getWritePointer (channel) + kPad; }
};
