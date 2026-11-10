#include "BusComp.h"

namespace od2c
{
static inline float toDb (float g)   { return 8.68588964f * std::log (std::max (g, 1.0e-6f)); }
static inline float toGain (float d) { return std::exp (d * 0.115129255f); }
static inline float coeff (double tau, double fs) { return (float) (1.0 - std::exp (-1.0 / (tau * fs))); }

void BusComp::prepare (double sampleRate, int nCh)
{
    fs = sampleRate;
    numChannels = std::clamp (nCh, 1, 2);
    peakRel     = (float) std::exp (-1.0 / (0.005 * fs));
    autoFastRel = coeff (0.10, fs);
    autoSlowRel = coeff (1.20, fs);
    autoSlowAtt = coeff (0.15, fs);
    for (auto& s : st)
    {
        s.makeup.reset (fs, 0.05);
        s.amount.reset (fs, 0.02);
    }
    reset();
}

void BusComp::reset() noexcept
{
    for (auto& s : st) s.peak = s.env = s.envSlow = 0.0f;
    grDb[0].store (0.0f);
    grDb[1].store (0.0f);
    snap = true;
}

void BusComp::process (juce::AudioBuffer<float>& buffer, const Settings& in) noexcept
{
    const int nCh = std::min (numChannels, buffer.getNumChannels());
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    const ChannelSettings cs[2] = { in.ch[0], in.stereo ? in.ch[0] : in.ch[1] };
    const bool linked = in.stereo && nCh > 1;

    float thr[2], slope[2], att[2], rel[2];
    bool autoRel[2];
    for (int c = 0; c < nCh; ++c)
    {
        auto& s = st[c];
        const bool active = in.power && cs[c].in && ! in.forceBypass;
        s.amount.setTargetValue (active ? 1.0f : 0.0f);
        s.makeup.setTargetValue (toGain (std::clamp (cs[c].makeupDb, 0.0f, 15.0f)));
        if (snap) { s.amount.setCurrentAndTargetValue (s.amount.getTargetValue()); s.makeup.setCurrentAndTargetValue (s.makeup.getTargetValue()); }

        thr[c]     = kThresholdRef + std::clamp (cs[c].threshold, -15.0f, 15.0f);
        slope[c]   = 1.0f - 1.0f / kRatio[std::clamp (cs[c].ratio, 0, 2)];
        att[c]     = coeff (kAttackMs[std::clamp (cs[c].attack, 0, 5)] * 0.001, fs);
        const int r = std::clamp (cs[c].release, 0, 4);
        autoRel[c] = (r == kAutoRelease);
        rel[c]     = autoRel[c] ? autoFastRel : coeff (kReleaseS[r], fs);
    }
    snap = false;

    // Powered off / all channels out and settled: leave audio untouched.
    bool anyActive = false;
    for (int c = 0; c < nCh; ++c)
        anyActive |= st[c].amount.isSmoothing() || st[c].amount.getTargetValue() > 0.0f;

    float blockGr[2] = { 0.0f, 0.0f };

    if (anyActive)
    {
        float* d[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };

        for (int i = 0; i < n; ++i)
        {
            float lvl[2] {};
            for (int c = 0; c < nCh; ++c)
            {
                st[c].peak = std::max (std::abs (d[c][i]), st[c].peak * peakRel);
                lvl[c] = toDb (st[c].peak);
            }
            if (linked) lvl[0] = lvl[1] = std::max (lvl[0], lvl[1]);

            for (int c = 0; c < nCh; ++c)
            {
                auto& s = st[c];
                // 2 dB soft knee, feed-forward VCA
                const float over = lvl[c] - thr[c];
                float target = 0.0f;
                if (over > 1.0f)       target = slope[c] * over;
                else if (over > -1.0f) target = slope[c] * (over + 1.0f) * (over + 1.0f) / 4.0f;

                s.env += (target > s.env ? att[c] : rel[c]) * (target - s.env);
                float gr = s.env;

                if (autoRel[c])
                {   // program-dependent: sustained compression recovers slowly, transients quickly
                    s.envSlow += (target > s.envSlow ? autoSlowAtt : autoSlowRel) * (target - s.envSlow);
                    gr = std::max (gr, s.envSlow);
                }

                const float amt = s.amount.getNextValue();
                const float mk  = s.makeup.getNextValue();
                d[c][i] *= 1.0f + amt * (toGain (-gr) * mk - 1.0f);
                blockGr[c] = std::max (blockGr[c], gr * amt);
            }
        }
    }

    for (int c = 0; c < 2; ++c)
        grDb[c].store (blockGr[std::min (c, nCh - 1)], std::memory_order_relaxed);
}
} // namespace od2c
