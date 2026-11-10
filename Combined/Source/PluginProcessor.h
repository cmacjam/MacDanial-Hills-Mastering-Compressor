#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/EngineA.h"
#include "DSP/EngineB.h"
#include "ParamIDs.h"

// MacDanial Hills Mastering Compressor: Model A (standard) stacked above
// Model B (Class-A), wired in series: input -> A -> B -> output.
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

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The DAW's bypass button drives a master bypass of both units.
    juce::AudioProcessorParameter* getBypassParameter() const override { return masterBypassParam; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    od2a::Engine engineA;
    od2b::Engine engineB;

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
    std::atomic<float>* masterBypassRaw = nullptr;
    juce::AudioProcessorParameter* masterBypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDHAudioProcessor)
};
