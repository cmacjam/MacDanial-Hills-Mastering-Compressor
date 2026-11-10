#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/EngineA.h"
#include "ParamIDs.h"
#include "UI/PanelLookAndFeel.h"
#include "UI/PanelComponents.h"

class UnitPanelA final : public juce::Component, private juce::Timer
{
public:
    UnitPanelA (juce::AudioProcessorValueTreeState&, od2a::Engine&, juce::String paramPrefix);
    ~UnitPanelA() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr float kBaseW = 1600.0f, kBaseH = 720.0f;   // faceplate design units

private:
    void timerCallback() override;

    // ---- layout / legend bookkeeping (all in base units) ----
    struct KnobSpec   { float cx, cy, r; juce::StringArray labels; juce::String title; };
    struct ToggleSpec { float cx, cy; juce::String top, bottom, left, right; };
    struct TextSpec   { juce::String text; float x, y, size; bool bold; };
    struct ArcText    { juce::String text; float cx, cy, radius, angle, size; };

    void buildChannel (int ch);
    void addKnob (juce::Slider&, const juce::String& paramId, float cx, float cy, float r,
                  const juce::StringArray& labels, const juce::String& title);
    void addToggle (juce::ToggleButton&, const juce::String& paramId, float cx, float cy,
                    const juce::String& top, const juce::String& bottom,
                    const juce::String& left, const juce::String& right, bool onIsRight);
    void place (juce::Component&, juce::Rectangle<float> baseBounds);
    void renderBackground();
    void drawPanel (juce::Graphics&);
    static juce::StringArray range (int from, int to);

    juce::AudioProcessorValueTreeState& apvts;
    od2a::Engine& engine;
    const juce::String prefix;
    od2::PanelLookAndFeel lnf;

    struct ChannelUI
    {
        juce::Slider optThr, optGain, ratio, attack, recover, dscThr, dscGain;
        juce::ToggleButton optIn, dscIn, scFilter;
        std::unique_ptr<od2::LedSelector> xfmr, meterSel;
        od2::Led optLed, dscLed;
        std::array<od2::Led, 3> meterLeds;
        od2::VuMeter vu;
    };
    std::array<ChannelUI, 2> chans;

    juce::ToggleButton hardwire, stereoSw;
    od2::Led monoLed, stereoLed;
    od2::MagicEye eye;

    std::vector<std::pair<juce::Component*, juce::Rectangle<float>>> placements;
    std::vector<KnobSpec>   knobSpecs;
    std::vector<ToggleSpec> toggleSpecs;
    std::vector<TextSpec>   textSpecs;
    std::vector<ArcText>    arcTexts;
    juce::Image background;

    // raw parameter pointers for the UI timer
    std::array<std::atomic<float>*, 2> rawOptIn {}, rawDscIn {}, rawMeter {};
    std::atomic<float>* rawStereo = nullptr;
    bool lastStereo = false, firstTick = true;
    float eyeLevel = 0.0f;
    double lastTickMs = 0.0;

    // Attachments last: destroyed first.
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnitPanelA)
};
