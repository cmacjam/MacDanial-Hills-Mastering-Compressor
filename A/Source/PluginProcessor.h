#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/OD2Engine.h"
#include "ParamIDs.h"

class OD2AudioProcessor final : public juce::AudioProcessor
{
public:
    OD2AudioProcessor();
    ~OD2AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
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

    // Host bypass button is mapped to the HARDWIRE BYPASS switch.
    juce::AudioProcessorParameter* getBypassParameter() const override { return hardwireParam; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    od2::Engine engine;

private:
    struct ChannelParams
    {
        std::atomic<float>* optIn   = nullptr; std::atomic<float>* optThr = nullptr; std::atomic<float>* optGain = nullptr;
        std::atomic<float>* dscIn   = nullptr; std::atomic<float>* dscThr = nullptr; std::atomic<float>* ratio   = nullptr;
        std::atomic<float>* attack  = nullptr; std::atomic<float>* recover = nullptr; std::atomic<float>* dscGain = nullptr;
        std::atomic<float>* scFilter = nullptr; std::atomic<float>* xfmr = nullptr;
    };

    std::array<ChannelParams, 2> chParams;
    std::atomic<float>* bypassRaw = nullptr;
    std::atomic<float>* stereoRaw = nullptr;
    juce::AudioProcessorParameter* hardwireParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OD2AudioProcessor)
};
