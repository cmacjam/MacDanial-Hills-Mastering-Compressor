#include "PopOutWindow.h"

PopOutWindow::PopOutWindow (const juce::String& title, juce::Component& panel, float baseW, float baseH,
                            juce::Rectangle<int> startBounds, std::function<void()> onCloseRequested)
    : DocumentWindow (title, juce::Colours::black, DocumentWindow::closeButton, true),
      aspect (baseW / baseH),
      onClose (std::move (onCloseRequested))
{
    setUsingNativeTitleBar (true);
    setContentNonOwned (&panel, false);
    setResizable (true, true);
    setResizeLimits (400, juce::roundToInt (400.0f / aspect), 3200, 3200);
    setStretch (false);
    setAlwaysOnTop (true);   // stays above the DAW while you work
    setBounds (startBounds);
    setVisible (true);
}

PopOutWindow::~PopOutWindow()
{
    clearContentComponent();   // the panel belongs to the editor, never delete it here
}

void PopOutWindow::setStretch (bool shouldStretch)
{
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (shouldStretch ? 0.0 : (double) aspect);
}

void PopOutWindow::closeButtonPressed()
{
    if (onClose) onClose();   // the editor docks the panel back asynchronously
}
