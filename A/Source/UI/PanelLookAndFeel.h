#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace od2
{
namespace colours
{
    inline const juce::Colour panel      { 0xff151619 };
    inline const juce::Colour panelLight { 0xff1f2024 };
    inline const juce::Colour legend     { 0xffeae6da };   // white screen-print
    inline const juce::Colour ledRed     { 0xffff2a2a };
    inline const juce::Colour ledBlue    { 0xff2fb8ff };
    inline const juce::Colour vuFace     { 0xfff0d78c };
    inline const juce::Colour vuRed      { 0xffd8261c };
}

// Rotary angles shared by knobs and the panel legends (clockwise from 12 o'clock).
inline constexpr float kRotStart = juce::MathConstants<float>::pi * 1.17f;
inline constexpr float kRotEnd   = juce::MathConstants<float>::pi * 2.83f;

class PanelLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // ---- Shared drawing primitives -------------------------------------------
    static void drawBakeliteKnob (juce::Graphics&, juce::Point<float> centre, float radius, float angle, bool enabled);
    static void drawToggleSwitch (juce::Graphics&, juce::Point<float> centre, float nutRadius,
                                  juce::Point<float> direction, float length);
    static void drawLed (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour colour, bool on);
    static void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius, float slotAngle);
    static void drawTextOnArc (juce::Graphics&, const juce::String& text, const juce::Font& font,
                               juce::Point<float> centre, float radius, float centreAngle);
    static juce::Font panelFont (float height, bool bold = true);
};

} // namespace od2
