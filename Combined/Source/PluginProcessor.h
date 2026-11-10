#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/EngineA.h"
#include "DSP/EngineB.h"
#include "DSP/BusComp.h"
#include "ParamIDs.h"
#include "Presets.h"

// MacDanial Hills Mastering Compressor: Model A (standard) above Model B (Class-A)
// above unit C (1U bus compressor), wired in series: input -> A -> B -> C -> output.
class MDHAudioProcessor final : public juce::AudioProcessor
{
public:
    MDHAudioProcessor();
    ~MDHAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                     { return true; }

    const juce::String getName() const override         { return JucePlugin_Name; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0.0; }

    // Factory presets are exposed to the DAW's preset menu as programs.
    int getNumPrograms() override                       { return presets.getNumFactory(); }
    int getCurrentProgram() override                    { return juce::jmax (0, presets.getCurrentFactoryIndex()); }
    void setCurrentProgram (int index) override         { presets.loadFactory (index); }
    const juce::String getProgramName (int index) override { return presets.getFactoryName (index); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The DAW's bypass button drives a master bypass of both units.
    juce::AudioProcessorParameter* getBypassParameter() const override { return masterBypassParam; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    od2a::Engine engineA;
    od2b::Engine engineB;
    od2c::BusComp engineC;
    PresetManager presets { apvts };

    static constexpr const char* masterBypassId = "MASTER_BYPASS";

private:
    struct ChannelParams
    {
        std::atomic<float>* optIn = nullptr; std::atomic<float>* optThr = nullptr; std::atomic<float>* optGain = nullptr;
        std::atomic<float>* dscIn = nullptr; std::atomic<float>* dscThr = nullptr; std::atomic<float>* ratio = nullptr;
        std::atomic<float>* attack = nullptr; std::atomic<float>* recover = nullptr; std::atomic<float>* dscGain = nullptr;
        std::atomic<float>* scFilter = nullptr; std::atomic<float>* xfmr = nullptr;
    };
    struct UnitParams
    {
        std::array<ChannelParams, 2> ch;
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* stereo = nullptr;
    };

    void bindUnit (UnitParams&, const juce::String& prefix);
    template <typename SettingsT> void readUnit (const UnitParams&, SettingsT&, bool masterBypass) const;

    UnitParams unitA, unitB;
    struct BusParams
    {
        std::atomic<float>* power = nullptr; std::atomic<float>* stereo = nullptr;
        struct Ch { std::atomic<float>* in = nullptr; std::atomic<float>* thr = nullptr; std::atomic<float>* ratio = nullptr;
                    std::atomic<float>* attack = nullptr; std::atomic<float>* release = nullptr; std::atomic<float>* makeup = nullptr; };
        std::array<Ch, 2> ch;
    } unitC;
    std::atomic<float>* masterBypassRaw = nullptr;
    juce::AudioProcessorParameter* masterBypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDHAudioProcessor)
};
