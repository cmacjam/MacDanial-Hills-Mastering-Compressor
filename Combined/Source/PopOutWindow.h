#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Floating window that temporarily hosts one unit panel (not owned).
class PopOutWindow final : public juce::DocumentWindow
{
public:
    PopOutWindow (const juce::String& title, juce::Component& panel, float baseW, float baseH,
                  juce::Rectangle<int> startBounds, std::function<void()> onCloseRequested);
    ~PopOutWindow() override;

    void closeButtonPressed() override;
    void setStretch (bool shouldStretch);

private:
    float aspect;
    std::function<void()> onClose;
};
