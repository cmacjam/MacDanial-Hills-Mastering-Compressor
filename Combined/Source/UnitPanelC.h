#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/BusComp.h"
#include "UI/PanelLookAndFeel.h"

// Gain-reduction needle meter (0 dB at right, 20 dB at left).
class GrMeter final : public juce::Component
{
public:
    GrMeter() { setInterceptsMouseClicks (false, false); }
    void setGr (float grDb, float dt);
    void paint (juce::Graphics&) override;
private:
    float shown = 0.0f;
};

// Unit C: 1U two-channel bus compressor.
class UnitPanelC final : public juce::Component, private juce::Timer
{
public:
    UnitPanelC (juce::AudioProcessorValueTreeState&, od2c::BusComp&, juce::String paramPrefix);
    ~UnitPanelC() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr float kBaseW = 1600.0f, kBaseH = 184.0f;

private:
    void timerCallback() override;
    void addKnob (juce::Slider&, const juce::String& id, float cx, const juce::StringArray& scale, const juce::String& label);
    void addLamp (juce::ToggleButton&, const juce::String& id, const juce::String& text, juce::Rectangle<float> r);
    void renderBackground();

    struct KnobSpec { float cx; juce::StringArray scale; juce::String label; };

    juce::AudioProcessorValueTreeState& apvts;
    od2c::BusComp& comp;
    const juce::String prefix;

    struct Channel { juce::Slider thr, ratio, attack, release, makeup; juce::ToggleButton in; GrMeter meter; };
    std::array<Channel, 2> chans;
    juce::ToggleButton power, stereo;

    std::vector<std::pair<juce::Component*, juce::Rectangle<float>>> placements;
    std::vector<KnobSpec> knobSpecs;
    juce::Image background;

    struct LinkPair { juce::RangedAudioParameter* l = nullptr; juce::RangedAudioParameter* r = nullptr; float lastL = 0, lastR = 0; };
    std::vector<LinkPair> links;
    std::atomic<float>* rawStereo = nullptr;
    bool lastStereo = true;
    double lastTickMs = 0.0;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;
};
