#pragma once
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <atomic>
#include <cmath>

// Unit C: two-channel VCA bus compressor (1U, sits under Models A and B).
namespace od2c
{
inline constexpr float kRatio[3]     = { 2.0f, 4.0f, 10.0f };
inline constexpr float kAttackMs[6]  = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
inline constexpr float kReleaseS[5]  = { 0.1f, 0.3f, 0.6f, 1.2f, 0.0f };    // 4 = AUTO
inline constexpr int   kAutoRelease  = 4;
inline constexpr float kThresholdRef = -10.0f;  // knob 0 = -10 dBFS (knob range -15 .. +15)

struct ChannelSettings
{
    bool  in = false;
    float threshold = 0.0f;   // knob value, -15 .. +15
    int   ratio = 0, attack = 3, release = kAutoRelease;
    float makeupDb = 0.0f;    // 0 .. +15
};

struct Settings
{
    bool power = true;
    bool stereo = true;       // linked detection; left controls drive both channels
    bool forceBypass = false; // master bypass
    ChannelSettings ch[2];
};

class BusComp
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>&, const Settings&) noexcept;

    std::atomic<float> grDb[2] { { 0.0f }, { 0.0f } };

private:
    struct State
    {
        float peak = 0.0f, env = 0.0f, envSlow = 0.0f;
        juce::SmoothedValue<float> makeup, amount;
    };
    State st[2];
    double fs = 44100.0;
    int numChannels = 2;
    float peakRel = 0.999f, autoFastRel = 0.0f, autoSlowRel = 0.0f, autoSlowAtt = 0.0f;
    bool snap = true;
};
} // namespace od2c
