#pragma once
#include "PluginProcessor.h"
#include "UnitPanelA.h"
#include "UnitPanelB.h"

// Two rack units stacked: Model A on top, Model B below.
class MDHEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MDHEditor (MDHAudioProcessor&);
    ~MDHEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr float kUnitW = 1600.0f, kUnitH = 720.0f;

private:
    od2::PanelLookAndFeel lnf;
    UnitPanelA unitA;
    UnitPanelB unitB;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDHEditor)
};
