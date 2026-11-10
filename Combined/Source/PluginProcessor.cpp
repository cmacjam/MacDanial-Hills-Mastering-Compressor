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
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::optGain),   name + "Optical Gain", 0, 24, 12));
            layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::dscIn),     name + "Discrete In", true));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscThr),    name + "Discrete Threshold", 1, 24, 6));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRatio),  name + "Discrete Ratio", ratios, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscAttack), name + "Discrete Attack", attacks, 5));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRecov),  name + "Discrete Recover", recovers, 0));
            layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscGain),   name + "Discrete Gain", 0, 24, 12));
            layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::scFilter),  name + "Sidechain Filter", true));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::xfmr),      name + "Transformer", xfmrs, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::meterSel),  name + "Meter Select", meters, c == 0 ? 0 : 1));
        }
    }
    return layout;
}

//==============================================================================
void MDHAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engineA.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engineB.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
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
    engineB.process (buffer, sB);   // bottom unit
}

//==============================================================================
void MDHAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MDHAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MDHAudioProcessor::createEditor()
{
    return new MDHEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MDHAudioProcessor();
}
