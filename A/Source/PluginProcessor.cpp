#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace od2;

//==============================================================================
OD2AudioProcessor::OD2AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OD2", createParameterLayout())
{
    auto raw = [this] (const juce::String& id)
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };

    for (int c = 0; c < 2; ++c)
    {
        auto& cp   = chParams[(size_t) c];
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

    bypassRaw     = raw (ids::hardwireBypass);
    stereoRaw     = raw (ids::stereo);
    hardwireParam = apvts.getParameter (ids::hardwireBypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout OD2AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using PID = juce::ParameterID;

    layout.add (std::make_unique<juce::AudioParameterBool> (PID { ids::hardwireBypass, 1 }, "Hardwire Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (PID { ids::stereo, 1 },         "Stereo (off = Dual Mono)", true));

    const juce::StringArray ratios   { "1.2:1", "2:1", "3:1", "4:1", "6:1", "Flood" };
    const juce::StringArray attacks  { "0.1 ms", "0.5 ms", "1 ms", "5 ms", "10 ms", "30 ms" };
    const juce::StringArray recovers { "0.1 s", "0.25 s", "0.5 s", "0.8 s", "1.2 s", "Dual" };
    const juce::StringArray xfmrs    { "Nickel", "Iron", "Steel" };
    const juce::StringArray meters   { "Optical GR", "Discrete GR", "Output" };

    for (int c = 0; c < 2; ++c)
    {
        const juce::String side = (c == 0 ? "L " : "R ");
        auto pid = [c] (const char* base) { return PID { ids::ch (c, base), 1 }; };

        layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::optIn),     side + "Optical In", true));
        layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::optThr),    side + "Optical Threshold", 1, 24, 6));
        layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::optGain),   side + "Optical Gain", 0, 24, 7));
        layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::dscIn),     side + "Discrete In", true));
        layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscThr),    side + "Discrete Threshold", 1, 24, 6));
        layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRatio),  side + "Discrete Ratio", ratios, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscAttack), side + "Discrete Attack", attacks, 5));
        layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::dscRecov),  side + "Discrete Recover", recovers, 0));
        layout.add (std::make_unique<juce::AudioParameterInt>    (pid (ids::dscGain),   side + "Discrete Gain", 0, 24, 7));
        layout.add (std::make_unique<juce::AudioParameterBool>   (pid (ids::scFilter),  side + "Sidechain Filter", true));
        layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::xfmr),      side + "Transformer", xfmrs, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (pid (ids::meterSel),  side + "Meter Select", meters, c == 0 ? 0 : 1));
    }

    return layout;
}

//==============================================================================
void OD2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples (engine.getLatencySamples());
}

void OD2AudioProcessor::releaseResources() {}

bool OD2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void OD2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto isOn  = [] (const std::atomic<float>* p) { return p->load (std::memory_order_relaxed) > 0.5f; };
    auto asInt = [] (const std::atomic<float>* p) { return (int) std::lround (p->load (std::memory_order_relaxed)); };

    Settings s;
    s.hardwireBypass = isOn (bypassRaw);
    s.stereo         = isOn (stereoRaw);

    for (int c = 0; c < 2; ++c)
    {
        const auto& cp = chParams[(size_t) c];
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

    engine.process (buffer, s);
}

//==============================================================================
void OD2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OD2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
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

juce::AudioProcessorEditor* OD2AudioProcessor::createEditor()
{
    return new OD2Editor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OD2AudioProcessor();
}
