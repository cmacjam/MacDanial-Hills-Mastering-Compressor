#pragma once
#include "PluginProcessor.h"
#include "UnitPanelA.h"
#include "UnitPanelB.h"
#include "UnitPanelC.h"
#include "PopOutWindow.h"

// Three rack units stacked (Model A, Model B, bus compressor C) under a toolbar with
// presets, zoom, stretch and pop-out controls.
class MDHEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MDHEditor (MDHAudioProcessor&);
    ~MDHEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr float kUnitW = 1600.0f;
    static constexpr int   kToolbarH = 38;
    static constexpr int   kBaseWidthAt100 = 1000;   // "100 %" zoom = 1000 px wide

private:
    struct Unit
    {
        juce::String name;
        juce::Component* panel;
        float baseH;
        std::unique_ptr<PopOutWindow> window;
        juce::TextButton popButton;
    };

    // Keeps the height in step with the width (unless stretching is on).
    struct Constrainer final : public juce::ComponentBoundsConstrainer
    {
        std::function<int (int)> heightForWidth, widthForHeight;
        bool locked = true;
        void checkBounds (juce::Rectangle<int>& b, const juce::Rectangle<int>& prev, const juce::Rectangle<int>& limits,
                          bool top, bool left, bool bottom, bool right) override;
    };

    void buildToolbar();
    void refreshPresetList();
    void loadPresetFromMenu();
    void savePresetDialog();
    void deleteCurrentPreset();
    void stepPreset (int delta);
    void setZoom (float zoom);
    void setStretch (bool);
    void togglePopOut (Unit&);
    void dock (Unit&);
    void updateConstrainer();
    float dockedBaseHeight() const;
    int heightForWidth (int w) const;
    void storeUiState();

    MDHAudioProcessor& proc;
    od2::PanelLookAndFeel lnf;

    UnitPanelA panelA;
    UnitPanelB panelB;
    UnitPanelC panelC;
    std::array<Unit, 3> units;

    juce::ComboBox presetBox, zoomBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" }, deleteBtn { "Delete" }, stretchBtn { "Stretch" };
    juce::Label allPoppedLabel;
    Constrainer constrainer;
    bool stretch = false;
    bool ignoreCallbacks = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MDHEditor)
};
