#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace od2;

//==============================================================================
MDHAudioProcessor::MDHAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MDH", createParameterLayout())
{
    bindUnit (unitA, "A_");
    bindUnit (unitB, "B_");

    auto raw = [this] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); jassert (p != nullptr); return p; };
    unitC.power  = raw ("C_POWER");
    unitC.stereo = raw ("C_STEREO");
    for (int c = 0; c < 2; ++c)
    {
        const juce::String s = c == 0 ? "C_L_" : "C_R_";
        auto& ch = unitC.ch[(size_t) c];
        ch.in = raw (s + "IN");   ch.thr = raw (s + "THR");       ch.ratio = raw (s + "RATIO");
        ch.attack = raw (s + "ATTACK"); ch.release = raw (s + "RELEASE"); ch.makeup = raw (s + "MAKEUP");
    }
    masterBypassRaw   = apvts.getRawParameterValue (masterBypassId);
    masterBypassParam = apvts.getParameter (masterBypassId);
}

void MDHAudioProcessor::bindUnit (UnitParams& u, const juce::String& pre)
{
    auto raw = [this, &pre] (const juce::String& id)
    {
        auto* p = apvts.getRawParameterValue (pre + id);
        jassert (p != nullptr);
        return p;
    };

    for (int c = 0; c < 2; ++c)
    {
        auto& cp   = u.ch[(size_t) c];
        cp.optIn    = raw (ids::ch (c, ids::optIn));
        cp.optThr   = raw (ids::ch (c, ids::optThr));
        cp.optGain  = raw (ids::ch (c, ids::optGain));
        cp.dscIn    = raw (ids::ch (c, ids::dscIn));
        cp.dscThr   = raw (ids::ch (c, ids::dscThr));
        cp.ratio    = raw (ids::ch (c, ids::dscRatio));
        cp.attack   = raw (ids::ch (c, ids::dscAttack));
        cp.recover  = raw (ids::ch (c, ids::dscRecov));
        cp.dscGain  = raw (ids::ch (c, ids::dscGain));
        cp.scFilter = raw (ids::ch (c, ids::scFilter));
        cp.xfmr     = raw (ids::ch (c, ids::xfmr));
    }
    u.bypass = raw (ids::hardwireBypass);
    u.stereo = raw (ids::stereo);
}

juce::AudioProcessorValueTreeState::ParameterLayout MDHAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using PID = juce::ParameterID;

    layout.add (std::make_unique<juce::AudioParameterBool> (PID { masterBypassId, 1 }, "Master Bypass", false));

    const juce::StringArray ratios   { "1.2:1", "2:1", "3:1", "4:1", "6:1", "Flood" };
    const juce::StringArray attacks  { "0.1 ms", "0.5 ms", "1 ms", "5 ms", "10 ms", "30 ms" };
    const juce::StringArray recovers { "0.1 s", "0.25 s", "0.5 s", "0.8 s", "1.2 s", "Dual" };
    const juce::StringArray xfmrs    { "Nickel", "Iron", "Steel" };
    const juce::StringArray meters   { "Optical GR", "Discrete GR", "Output" };

    for (const juce::String unit : { "A", "B" })
    {
        const juce::String pre = unit + "_";
        const bool isA = unit == "A";

        // Model A defaults to stage OUT so the stack starts with B doing the work.
        layout.add (std::make_unique<juce::AudioParameterBool> (PID { pre + ids::hardwireBypass, 1 }, unit + " Hardwire Bypass", false));
        layout.add (std::make_unique<juce::AudioParameterBool> (PID { pre + ids::stereo, 1 },         unit + " Stereo (off = Dual Mono)", true));

        for (int c = 0; c < 2; ++c)
        {
            const juce::String name = unit + (c == 0 ? " L " : " R ");
            auto pid = [&pre, c] (const char* base) { return PID { pre + ids::ch (c, base), 1 }; };

            layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::optIn),     name + "Optical In", ! isA));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::optThr),    name + "Optical Threshold", 1, 24, 6));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::optGain),   name + "Optical Gain", 0, 24, 7));
            layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::dscIn),     name + "Discrete In", true));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscThr),    name + "Discrete Threshold", 1, 24, 6));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRatio),  name + "Discrete Ratio", ratios, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscAttack), name + "Discrete Attack", attacks, 5));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRecov),  name + "Discrete Recover", recovers, 0));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscGain),   name + "Discrete Gain", 0, 24, 7));
            layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::scFilter),  name + "Sidechain Filter", true));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::xfmr),      name + "Transformer", xfmrs, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::meterSel),  name + "Meter Select", meters, c == 0 ? 0 : 1));
        }
    }
    // ---- Unit C: bus compressor (starts powered, both channels OUT) ----
    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "C_POWER", 1 },  "C Power", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "C_STEREO", 1 }, "C Stereo Link", true));
    for (int c = 0; c < 2; ++c)
    {
        const juce::String s = c == 0 ? "C_L_" : "C_R_", n = c == 0 ? "C L " : "C R ";
        layout.add (std::make_unique<juce::AudioParameterBool>   (PID { s + "IN", 1 },      n + "In", false));
        layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { s + "THR", 1 },     n + "Threshold",
                        juce::NormalisableRange<float> (-15.0f, 15.0f, 0.5f), 0.0f));
        layout.add (std::make_unique<juce::AudioParameterChoice> (PID { s + "RATIO", 1 },   n + "Ratio", juce::StringArray { "2:1", "4:1", "10:1" }, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (PID { s + "ATTACK", 1 },  n + "Attack",
                        juce::StringArray { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 3));
        layout.add (std::make_unique<juce::AudioParameterChoice> (PID { s + "RELEASE", 1 }, n + "Release",
                        juce::StringArray { "0.1 s", "0.3 s", "0.6 s", "1.2 s", "Auto" }, 4));
        layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { s + "MAKEUP", 1 },  n + "Make-up",
                        juce::NormalisableRange<float> (0.0f, 15.0f, 0.5f), 0.0f));
    }

    return layout;
}

//==============================================================================
void MDHAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engineA.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engineB.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engineC.prepare (sampleRate, getTotalNumOutputChannels());
    setLatencySamples (engineA.getLatencySamples() + engineB.getLatencySamples());
}

bool MDHAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

template <typename SettingsT>
void MDHAudioProcessor::readUnit (const UnitParams& u, SettingsT& s, bool masterBypass) const
{
    auto isOn  = [] (const std::atomic<float>* p) { return p->load (std::memory_order_relaxed) > 0.5f; };
    auto asInt = [] (const std::atomic<float>* p) { return (int) std::lround (p->load (std::memory_order_relaxed)); };

    s.hardwireBypass = masterBypass || isOn (u.bypass);
    s.stereo         = isOn (u.stereo);

    for (int c = 0; c < 2; ++c)
    {
        const auto& cp = u.ch[(size_t) c];
        auto& cs = s.ch[c];
        cs.optoIn      = isOn  (cp.optIn);
        cs.optoThr     = asInt (cp.optThr);
        cs.optoGain    = asInt (cp.optGain);
        cs.discIn      = isOn  (cp.dscIn);
        cs.discThr     = asInt (cp.dscThr);
        cs.ratio       = asInt (cp.ratio);
        cs.attack      = asInt (cp.attack);
        cs.recover     = asInt (cp.recover);
        cs.discGain    = asInt (cp.dscGain);
        cs.scFilter    = isOn  (cp.scFilter);
        cs.transformer = asInt (cp.xfmr);
    }
}

void MDHAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const bool master = masterBypassRaw->load (std::memory_order_relaxed) > 0.5f;

    od2a::Settings sA;
    od2b::Settings sB;
    readUnit (unitA, sA, master);
    readUnit (unitB, sB, master);

    engineA.process (buffer, sA);   // top unit
    engineB.process (buffer, sB);   // middle unit

    od2c::Settings sC;
    auto on = [] (const std::atomic<float>* p) { return p->load (std::memory_order_relaxed) > 0.5f; };
    sC.power       = on (unitC.power);
    sC.stereo      = on (unitC.stereo);
    sC.forceBypass = master;
    for (int c = 0; c < 2; ++c)
    {
        const auto& pc = unitC.ch[(size_t) c];
        auto& cs = sC.ch[c];
        cs.in        = on (pc.in);
        cs.threshold = pc.thr->load (std::memory_order_relaxed);
        cs.ratio     = (int) std::lround (pc.ratio->load (std::memory_order_relaxed));
        cs.attack    = (int) std::lround (pc.attack->load (std::memory_order_relaxed));
        cs.release   = (int) std::lround (pc.release->load (std::memory_order_relaxed));
        cs.makeupDb  = pc.makeup->load (std::memory_order_relaxed);
    }
    engineC.process (buffer, sC);   // bottom unit (1U bus compressor)
}

//==============================================================================
void MDHAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        xml->setAttribute ("uiWidth",   uiWidth.load());
        xml->setAttribute ("uiHeight",  uiHeight.load());
        xml->setAttribute ("uiStretch", uiStretch.load() ? 1 : 0);
        copyXmlToBinary (*xml, destData);
    }
}

void MDHAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            uiWidth   = xml->getIntAttribute ("uiWidth", uiWidth.load());
            uiHeight  = xml->getIntAttribute ("uiHeight", uiHeight.load());
            uiStretch = xml->getIntAttribute ("uiStretch", uiStretch.load() ? 1 : 0) != 0;
            xml->removeAttribute ("uiWidth");
            xml->removeAttribute ("uiHeight");
            xml->removeAttribute ("uiStretch");
            // Read the stored values from our private XML copy (never from the live state tree,
            // which JUCE's own timer may be writing on the message thread).
            std::vector<std::pair<juce::String, float>> stored;
            for (auto* e : xml->getChildIterator())
                if (e->hasAttribute ("id") && e->hasAttribute ("value"))
                    stored.emplace_back (e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("value"));

            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            // Push every stored value back into its parameter. Without this, on/off parameters
            // that a host left at an in-between value (e.g. 0.6) may not be reset, because the
            // state tree already holds the same snapped value.
            for (const auto& [id, value] : stored)
                if (auto* rp = apvts.getParameter (id))
                    rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        }
}

juce::AudioProcessorEditor* MDHAudioProcessor::createEditor()
{
    return new MDHEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MDHAudioProcessor();
}
