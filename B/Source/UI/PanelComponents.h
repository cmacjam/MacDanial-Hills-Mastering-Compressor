#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PanelLookAndFeel.h"

namespace od2
{
//==============================================================================
class Led final : public juce::Component
{
public:
    Led() { setInterceptsMouseClicks (false, false); }
    void setColour (juce::Colour c) { colour = c; repaint(); }
    void setOn (bool shouldBeOn)    { if (on != shouldBeOn) { on = shouldBeOn; repaint(); } }
    void paint (juce::Graphics&) override;

private:
    juce::Colour colour { colours::ledRed };
    bool on = false;
};

//==============================================================================
// Multi-position selector bound to a choice parameter.
//   verticalToggle : 3-way bat switch with an LED per position (Transformer Select)
//   miniKnob       : small bakelite knob with LEDs on an arc (Meter Select)
class LedSelector final : public juce::Component
{
public:
    enum class Style { verticalToggle, miniKnob };

    LedSelector (juce::RangedAudioParameter& param, Style style, juce::Colour ledColour, bool mirrored);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    int getIndex() const noexcept { return index; }

private:
    void select (int newIndex);
    juce::Point<float> ledCentre (int i) const;
    float knobAngleFor (int i) const;

    const Style        style;
    const juce::Colour ledColour;
    const bool         mirrored;
    const int          numItems;
    int                index = 0;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// Illuminated analog VU meter (bakelite bezel, cream face, red zone, needle).
class VuMeter final : public juce::Component
{
public:
    VuMeter();
    // vuDb: reading relative to 0 VU (GR modes pass -GR, output mode passes level - ref)
    void setReading (float vuDb, float dtSeconds);
    void setBrand (juce::String b) { brand = std::move (b); renderCaches(); }

    void paint (juce::Graphics&) override;
    void resized() override { renderCaches(); }

private:
    struct Geo { juce::Rectangle<float> face; juce::Point<float> pivot; float R; };
    Geo geometry() const;
    static float vuToPos (float db);
    static float posToAngle (float pos);
    void renderCaches();
    void drawStatic (juce::Graphics&);
    void drawOverlay (juce::Graphics&);

    juce::Image staticLayer, overlayLayer;
    juce::String brand { "basemental audio" };
    float pos = 0.0f;
};

//==============================================================================
// "Magic eye" indicator tube; the shadow wedge closes on output peaks.
class MagicEye final : public juce::Component
{
public:
    MagicEye() { setInterceptsMouseClicks (false, false); }
    void setLevel (float l) { if (std::abs (l - level) > 0.002f) { level = l; repaint(); } }
    void paint (juce::Graphics&) override;

private:
    float level = 0.0f;
};

} // namespace od2
