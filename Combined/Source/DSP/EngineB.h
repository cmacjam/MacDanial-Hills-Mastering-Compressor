#pragma once
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

namespace od2b
{
//==============================================================================
// Hardware step tables (from the front-panel legends)
inline constexpr int   kNumSteps          = 6;
inline constexpr float kRatio[kNumSteps]    = { 1.2f, 2.0f, 3.0f, 4.0f, 6.0f, 20.0f }; // 5 = FLOOD (20:1)
inline constexpr float kAttackMs[kNumSteps] = { 0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 30.0f };
inline constexpr float kRecoverS[kNumSteps] = { 0.1f, 0.25f, 0.5f, 0.8f, 1.2f, 0.0f }; // 5 = DUAL
inline constexpr int   kFloodIndex        = 5;
inline constexpr int   kDualIndex         = 5;

// Calibration: threshold step 1 = 0 dBFS (minimum compression), step 24 = -28.75 dBFS.
inline float thresholdDb (int step) noexcept { return -1.25f * (float) (std::clamp (step, 1, 24) - 1); }
// Gain: 24-position switch, unity at position 7 (as on the hardware), fine 0.5 dB steps
// near unity and coarser 1.25 dB steps further out:  pos 0 = -5.75 dB ... pos 24 = +18.25 dB.
inline float makeupDb (int step) noexcept
{
    const int d = std::clamp (step, 0, 24) - 7;
    const int a = std::abs (d);
    const float db = a <= 4 ? 0.5f * (float) a : 2.0f + 1.25f * (float) (a - 4);
    return d < 0 ? -db : db;
}

inline float dbToGain (float db) noexcept    { return std::exp (db * 0.115129255f); }
inline float gainToDb (float g) noexcept     { return 8.68588964f * std::log (std::max (g, 1.0e-6f)); }
inline float powerToDb (float p) noexcept    { return 4.34294482f * std::log (std::max (p, 1.0e-12f)); }

inline float fastTanh (float x) noexcept
{
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Soft-knee gain-reduction curve. Returns positive dB of reduction.
inline float softKneeGr (float overDb, float slope, float kneeDb) noexcept
{
    const float half = 0.5f * kneeDb;
    if (overDb <= -half) return 0.0f;
    if (overDb <   half) { const float t = overDb + half; return slope * t * t / (2.0f * kneeDb); }
    return slope * overDb;
}

//==============================================================================
// Allocation-free TDF-II biquad (RBJ cookbook). Safe to retune on the audio thread.
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setHighPass  (double fs, double f, double q) noexcept;
    void setPeak      (double fs, double f, double q, double gainDb) noexcept;
    void setLowShelf  (double fs, double f, double q, double gainDb) noexcept;
    void setHighShelf (double fs, double f, double q, double gainDb) noexcept;

private:
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept;
};

//==============================================================================
// Output transformer model: EQ signature + low-frequency core (flux) saturation
// + general soft saturation + a touch of even-order asymmetry.
struct Transformer
{
    enum Type { nickel = 0, iron = 1, steel = 2, inputLundahl = 3 };

    void configure (int type, double fs) noexcept;
    void reset() noexcept { lo.reset(); hi.reset(); flux = 0.0f; }

    inline float process (float x) noexcept
    {
        x = hi.process (lo.process (x));

        // Core saturation is strongest at low frequencies (flux ~ integral of voltage).
        flux += fluxCoeff * (x - flux);
        const float f = flux * fluxDrive;
        x += fluxAmt * (fastTanh (f) - f) / fluxDrive;

        float y = fastTanh (x * drive) / drive;
        y += even * y * y;                     // 2nd harmonic (DC removed downstream)
        return y;
    }

    Biquad lo, hi;
    float fluxCoeff = 0.0f, flux = 0.0f, fluxDrive = 1.0f, fluxAmt = 0.0f;
    float drive = 0.1f, even = 0.0f;
};

//==============================================================================
struct ChannelSettings
{
    bool optoIn = true;   int optoThr = 6;  int optoGain = 12;
    bool discIn = true;   int discThr = 6;  int ratio = 1;  int attack = 4;  int recover = 1;  int discGain = 12;
    bool scFilter = false;
    int  transformer = Transformer::nickel;
};

struct Settings
{
    bool hardwireBypass = false;
    bool stereo         = true;     // Stereo: left settings drive both channels, detectors linked
    ChannelSettings ch[2];
};

//==============================================================================
class Engine
{
public:
    static constexpr int kOversamplingOrder = 2;   // 2^2 = 4x

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

    int getLatencySamples() const noexcept;
    int getNumChannels() const noexcept { return numChannels; }

    // ---- Lock-free metering (written on audio thread, read by the UI timer) ----
    std::atomic<float> optoGrDb[2]  { { 0.0f }, { 0.0f } };   // positive dB of reduction
    std::atomic<float> discGrDb[2]  { { 0.0f }, { 0.0f } };
    std::atomic<float> outMeanSq[2] { { 0.0f }, { 0.0f } };   // block mean-square of output
    std::atomic<float> outPeak      { 0.0f };                  // max |out| since last UI read

private:
    struct ChannelState
    {
        Biquad scOpto, scDisc;
        float optoMs = 0.0f, optoFast = 0.0f, optoSlow = 0.0f;
        float discEnv = 0.0f, discFast = 0.0f, discSlow = 0.0f, discPeak = 0.0f;
        Transformer xf[3];
        Transformer inXf;          // Class-A: Lundahl-style input transformer
        bool  xfActive[3] { true, false, false };
        float dcX = 0.0f, dcY = 0.0f;

        juce::SmoothedValue<float> optoGain, discGain, optoAmt, discAmt;
        juce::SmoothedValue<float> xfW[3];

        // per-block settings
        float optoThr = 0.0f, discThr = 0.0f, discSlope = 0.5f, discKnee = 4.0f;
        float discAtt = 0.0f, discRel = 0.0f;
        bool  dual = false, scOn = false;
    };

    void updateTargets (const ChannelSettings* cs, bool bypass, int nCh) noexcept;
    void processOversampled (juce::dsp::AudioBlock<float>& block) noexcept;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::array<ChannelState, 2> state;
    juce::SmoothedValue<float> bypassAmt;

    double sampleRate = 44100.0, osRate = 176400.0;
    int    numChannels = 2, maxBlockSize = 512;
    bool   linked = true, needsSnap = true;

    float optoRmsCoeff = 0.0f, optoAttCoeff = 0.0f, optoFastRelCoeff = 0.0f;
    float dualFastRelCoeff = 0.0f, peakRelMul = 0.999f, invFs = 0.0f, dcCoeff = 0.999f;
    float blockOptoGr[2] {}, blockDiscGr[2] {};
};

} // namespace od2b
