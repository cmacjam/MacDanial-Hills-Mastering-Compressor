#include "EngineA.h"

namespace od2a
{
//==============================================================================
// Biquad
void Biquad::set (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept
{
    b0 = (float) (nb0 / na0);
    b1 = (float) (nb1 / na0);
    b2 = (float) (nb2 / na0);
    a1 = (float) (na1 / na0);
    a2 = (float) (na2 / na0);
}

void Biquad::setHighPass (double fs, double f, double q) noexcept
{
    const double w0 = juce::MathConstants<double>::twoPi * f / fs;
    const double c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
    set ((1.0 + c) * 0.5, -(1.0 + c), (1.0 + c) * 0.5, 1.0 + al, -2.0 * c, 1.0 - al);
}

void Biquad::setPeak (double fs, double f, double q, double gainDb) noexcept
{
    const double A  = std::pow (10.0, gainDb / 40.0);
    const double w0 = juce::MathConstants<double>::twoPi * f / fs;
    const double c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
    set (1.0 + al * A, -2.0 * c, 1.0 - al * A, 1.0 + al / A, -2.0 * c, 1.0 - al / A);
}

void Biquad::setLowShelf (double fs, double f, double q, double gainDb) noexcept
{
    const double A  = std::pow (10.0, gainDb / 40.0), sA = std::sqrt (A);
    const double w0 = juce::MathConstants<double>::twoPi * f / fs;
    const double c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
    set (A * ((A + 1.0) - (A - 1.0) * c + 2.0 * sA * al),
         2.0 * A * ((A - 1.0) - (A + 1.0) * c),
         A * ((A + 1.0) - (A - 1.0) * c - 2.0 * sA * al),
         (A + 1.0) + (A - 1.0) * c + 2.0 * sA * al,
         -2.0 * ((A - 1.0) + (A + 1.0) * c),
         (A + 1.0) + (A - 1.0) * c - 2.0 * sA * al);
}

void Biquad::setHighShelf (double fs, double f, double q, double gainDb) noexcept
{
    const double A  = std::pow (10.0, gainDb / 40.0), sA = std::sqrt (A);
    const double w0 = juce::MathConstants<double>::twoPi * f / fs;
    const double c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
    set (A * ((A + 1.0) + (A - 1.0) * c + 2.0 * sA * al),
         -2.0 * A * ((A - 1.0) + (A + 1.0) * c),
         A * ((A + 1.0) + (A - 1.0) * c - 2.0 * sA * al),
         (A + 1.0) - (A - 1.0) * c + 2.0 * sA * al,
         2.0 * ((A - 1.0) - (A + 1.0) * c),
         (A + 1.0) - (A - 1.0) * c - 2.0 * sA * al);
}

//==============================================================================
// Transformer voicings
//   Nickel : least distortion, natural low end, gentle lift around 10 kHz ("air").
//   Iron   : Class-A output stage - mostly even-order harmonics, ~110 Hz lift, clear highs.
//   Steel  : most (still mild) distortion and transient softening, ~40 Hz lift, open top.
void Transformer::configure (int type, double fs) noexcept
{
    double fluxHz = 120.0;

    switch (type)
    {
        case nickel:
            lo.setLowShelf  (fs, 30.0,    0.70,  0.0);   // natural, flat low end
            hi.setPeak      (fs, 10000.0, 0.50, +0.8);   // the 10 kHz sheen
            drive = 0.10f; fluxDrive = 1.0f; fluxAmt = 0.03f; even = 0.000f; fluxHz = 150.0;
            break;

        case iron:
            lo.setPeak      (fs, 110.0,   0.70, +0.8);   // ~110 Hz weight
            hi.setHighShelf (fs, 14000.0, 0.60,  0.0);   // clear highs
            drive = 0.14f; fluxDrive = 1.3f; fluxAmt = 0.08f; even = 0.018f; fluxHz = 120.0;  // class-A: even-order
            break;

        default: // steel
            lo.setPeak      (fs, 40.0,    0.90, +1.6);   // big, tight 40 Hz
            hi.setHighShelf (fs, 12000.0, 0.60, +0.2);   // open, detailed top
            drive = 0.34f; fluxDrive = 1.6f; fluxAmt = 0.22f; even = 0.006f; fluxHz = 100.0;
            break;
    }

    fluxCoeff = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * fluxHz / fs));
}

//==============================================================================
// Engine
void Engine::prepare (double sr, int maxBlock, int nCh)
{
    sampleRate   = sr;
    numChannels  = std::clamp (nCh, 1, 2);
    maxBlockSize = std::max (1, maxBlock);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, (size_t) kOversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,    // max quality
        true);   // integer latency, so the host can compensate exactly
    oversampler->initProcessing ((size_t) maxBlockSize);

    osRate = sr * (double) (1 << kOversamplingOrder);
    const double fs = osRate;
    auto coeff = [fs] (double tauSeconds) { return (float) (1.0 - std::exp (-1.0 / (tauSeconds * fs))); };

    optoRmsCoeff     = coeff (0.004);   // photocell "light" integration
    optoAttCoeff     = coeff (0.010);   // slow-ish opto attack
    optoFastRelCoeff = coeff (0.060);   // 80 % of reduction releases quickly
    dualFastRelCoeff = coeff (0.100);
    peakRelMul       = (float) std::exp (-1.0 / (0.008 * fs));   // discrete peak detector release
    invFs            = (float) (1.0 / fs);
    dcCoeff          = (float) (1.0 - juce::MathConstants<double>::twoPi * 5.0 / fs);

    for (auto& st : state)
    {
        st.scOpto.setHighPass (fs, 90.0, 0.7071);
        st.scDisc.setHighPass (fs, 90.0, 0.7071);

        for (int k = 0; k < 3; ++k)
        {
            st.xf[k].configure (k, fs);
            st.xfW[k].reset (fs, 0.030);
        }

        st.optoGain.reset (fs, 0.050);
        st.discGain.reset (fs, 0.050);
        st.optoAmt .reset (fs, 0.020);
        st.discAmt .reset (fs, 0.020);
    }

    bypassAmt.reset (fs, 0.020);
    reset();
}

void Engine::reset() noexcept
{
    if (oversampler != nullptr)
        oversampler->reset();

    for (auto& st : state)
    {
        st.scOpto.reset();
        st.scDisc.reset();
        for (auto& x : st.xf) x.reset();
        st.optoMs = st.optoFast = st.optoSlow = 0.0f;
        st.discEnv = st.discFast = st.discSlow = st.discPeak = 0.0f;
        st.dcX = st.dcY = 0.0f;
    }

    for (int c = 0; c < 2; ++c)
    {
        optoGrDb[c].store (0.0f);
        discGrDb[c].store (0.0f);
        outMeanSq[c].store (0.0f);
    }
    outPeak.store (0.0f);
    needsSnap = true;
}

int Engine::getLatencySamples() const noexcept
{
    return oversampler != nullptr ? (int) std::lround (oversampler->getLatencyInSamples()) : 0;
}

void Engine::updateTargets (const ChannelSettings* cs, bool bypass, int nCh) noexcept
{
    const float fs = (float) osRate;

    for (int ch = 0; ch < nCh; ++ch)
    {
        auto& st = state[(size_t) ch];
        const auto& c = cs[ch];

        st.optoGain.setTargetValue (dbToGain (makeupDb (c.optoGain)));
        st.discGain.setTargetValue (dbToGain (makeupDb (c.discGain)));
        st.optoAmt .setTargetValue (c.optoIn ? 1.0f : 0.0f);
        st.discAmt .setTargetValue (c.discIn ? 1.0f : 0.0f);

        const int xf = std::clamp (c.transformer, 0, 2);
        for (int k = 0; k < 3; ++k)
            st.xfW[k].setTargetValue (k == xf ? 1.0f : 0.0f);

        st.optoThr = thresholdDb (c.optoThr);
        st.discThr = thresholdDb (c.discThr);

        const int r = std::clamp (c.ratio, 0, kNumSteps - 1);
        st.discSlope = 1.0f - 1.0f / kRatio[r];
        st.discKnee  = (r == kFloodIndex) ? 1.0f : 4.0f;

        const int a = std::clamp (c.attack, 0, kNumSteps - 1);
        st.discAtt = 1.0f - std::exp (-1.0f / (kAttackMs[a] * 0.001f * fs));

        const int rec = std::clamp (c.recover, 0, kNumSteps - 1);
        st.dual    = (rec == kDualIndex);
        st.discRel = st.dual ? 0.0f : 1.0f - std::exp (-1.0f / (kRecoverS[rec] * fs));
        st.scOn    = c.scFilter;

        if (needsSnap)
        {
            for (auto* sv : { &st.optoGain, &st.discGain, &st.optoAmt, &st.discAmt, &st.xfW[0], &st.xfW[1], &st.xfW[2] })
                sv->setCurrentAndTargetValue (sv->getTargetValue());

            for (int k = 0; k < 3; ++k)
                st.xfActive[k] = (k == xf);
        }
    }

    bypassAmt.setTargetValue (bypass ? 1.0f : 0.0f);
    if (needsSnap)
        bypassAmt.setCurrentAndTargetValue (bypassAmt.getTargetValue());

    needsSnap = false;
}

void Engine::process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept
{
    if (oversampler == nullptr)
        return;

    const int nCh   = std::min (numChannels, buffer.getNumChannels());
    const int total = buffer.getNumSamples();
    if (nCh <= 0 || total <= 0)
        return;

    // In Stereo mode the left-hand controls govern both channels (as on the hardware).
    const ChannelSettings cs[2] = { s.ch[0], s.stereo ? s.ch[0] : s.ch[1] };
    linked = s.stereo && nCh > 1;
    updateTargets (cs, s.hardwireBypass, nCh);

    blockOptoGr[0] = blockOptoGr[1] = 0.0f;
    blockDiscGr[0] = blockDiscGr[1] = 0.0f;

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nCh, (size_t) total);

    for (int start = 0; start < total; start += maxBlockSize)
    {
        const int len = std::min (maxBlockSize, total - start);
        auto sub = block.getSubBlock ((size_t) start, (size_t) len);
        auto os  = oversampler->processSamplesUp (sub);
        processOversampled (os);
        oversampler->processSamplesDown (sub);
    }

    // ---- Metering ----
    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const int src = std::min (ch, nCh - 1);
        const float* d = buffer.getReadPointer (src);
        float sum = 0.0f;
        for (int i = 0; i < total; ++i)
        {
            sum += d[i] * d[i];
            peak = std::max (peak, std::abs (d[i]));
        }
        outMeanSq[ch].store (sum / (float) total, std::memory_order_relaxed);
        optoGrDb[ch] .store (blockOptoGr[src],    std::memory_order_relaxed);
        discGrDb[ch] .store (blockDiscGr[src],    std::memory_order_relaxed);
    }

    if (peak > outPeak.load (std::memory_order_relaxed))
        outPeak.store (peak, std::memory_order_relaxed);
}

void Engine::processOversampled (juce::dsp::AudioBlock<float>& os) noexcept
{
    const int nCh = (int) std::min (os.getNumChannels(), (size_t) numChannels);
    const int n   = (int) os.getNumSamples();
    float* ptr[2] = { os.getChannelPointer (0), nCh > 1 ? os.getChannelPointer (1) : nullptr };

    // Fully hardwire-bypassed: leave the (latency-matched) signal untouched.
    if (! bypassAmt.isSmoothing() && bypassAmt.getTargetValue() >= 1.0f)
        return;

    for (int i = 0; i < n; ++i)
    {
        const float byp = bypassAmt.getNextValue();
        float dry[2] {}, x[2] {}, lvl[2] {};

        // ---- 1. Input line amp + optical detector ----------------------------
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto& st = state[(size_t) ch];
            dry[ch] = ptr[ch][i];

            float v = fastTanh (dry[ch] * 0.15f) / 0.15f;  // class-A line amp, ~+16 dB headroom
            v += 0.003f * v * v;                            // class-A 2nd harmonic
            x[ch] = v;

            const float hp = st.scOpto.process (v);
            const float sc = st.scOn ? hp : v;
            st.optoMs += optoRmsCoeff * (sc * sc - st.optoMs);
            lvl[ch] = powerToDb (st.optoMs) + 3.0f;         // RMS -> sine-peak referenced
        }
        if (linked) lvl[0] = lvl[1] = std::max (lvl[0], lvl[1]);

        // ---- 2. Optical stage: fixed 2:1, soft knee, two-stage release ---------
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto& st = state[(size_t) ch];
            const float target = softKneeGr (lvl[ch] - st.optoThr, 0.5f, 10.0f);
            const float tf = 0.8f * target, ts = 0.2f * target;

            st.optoFast += (tf > st.optoFast ? optoAttCoeff : optoFastRelCoeff) * (tf - st.optoFast);

            if (ts > st.optoSlow)
                st.optoSlow += optoAttCoeff * (ts - st.optoSlow);
            else
            {   // program-dependent: deeper compression -> longer slow release (0.8 - 1.8 s)
                const float tau = 0.8f + 0.05f * std::min (st.optoFast + st.optoSlow, 20.0f);
                st.optoSlow += (invFs / tau) * (ts - st.optoSlow);
            }

            const float gr  = st.optoFast + st.optoSlow;
            const float amt = st.optoAmt.getNextValue();
            const float mk  = st.optoGain.getNextValue();
            x[ch] *= 1.0f + amt * (dbToGain (-gr) * mk - 1.0f);
            blockOptoGr[ch] = std::max (blockOptoGr[ch], gr * amt);
        }

        // ---- 3. Discrete (VCA) stage ------------------------------------------
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto& st = state[(size_t) ch];
            const float hp = st.scDisc.process (x[ch]);
            st.discPeak = std::max (std::abs (st.scOn ? hp : x[ch]), st.discPeak * peakRelMul);
            lvl[ch] = gainToDb (st.discPeak);
        }
        if (linked) lvl[0] = lvl[1] = std::max (lvl[0], lvl[1]);

        for (int ch = 0; ch < nCh; ++ch)
        {
            auto& st = state[(size_t) ch];
            const float target = softKneeGr (lvl[ch] - st.discThr, st.discSlope, st.discKnee);
            float gr;

            if (! st.dual)
            {
                st.discEnv += (target > st.discEnv ? st.discAtt : st.discRel) * (target - st.discEnv);
                gr = st.discEnv;
                st.discFast = 0.8f * gr;
                st.discSlow = 0.2f * gr;
            }
            else
            {   // DUAL: mimics the optical two-stage recovery
                const float tf = 0.8f * target, ts = 0.2f * target;
                st.discFast += (tf > st.discFast ? st.discAtt : dualFastRelCoeff) * (tf - st.discFast);

                if (ts > st.discSlow)
                    st.discSlow += st.discAtt * (ts - st.discSlow);
                else
                {
                    const float tau = 1.0f + 0.05f * std::min (st.discFast + st.discSlow, 20.0f);
                    st.discSlow += (invFs / tau) * (ts - st.discSlow);
                }
                gr = st.discFast + st.discSlow;
                st.discEnv = gr;
            }

            const float amt = st.discAmt.getNextValue();
            const float mk  = st.discGain.getNextValue();
            x[ch] *= 1.0f + amt * (dbToGain (-gr) * mk - 1.0f);
            blockDiscGr[ch] = std::max (blockDiscGr[ch], gr * amt);
        }

        // ---- 4. Output transformer matrix + DC block + hardwire crossfade ------
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto& st = state[(size_t) ch];
            float y = 0.0f;

            for (int k = 0; k < 3; ++k)
            {
                const float w = st.xfW[k].getNextValue();
                if (w > 1.0e-5f)
                {
                    if (! st.xfActive[k]) { st.xf[k].reset(); st.xfActive[k] = true; }
                    y += w * st.xf[k].process (x[ch]);
                }
                else
                {
                    st.xfActive[k] = false;
                }
            }

            const float dc = y - st.dcX + dcCoeff * st.dcY;
            st.dcX = y;
            st.dcY = dc;

            ptr[ch][i] = dc + byp * (dry[ch] - dc);
        }
    }
}

} // namespace od2a
