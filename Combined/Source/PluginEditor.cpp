#include "PluginEditor.h"

MDHEditor::MDHEditor (MDHAudioProcessor& p)
    : AudioProcessorEditor (&p),
      unitA (p.apvts, p.engineA, "A_"),
      unitB (p.apvts, p.engineB, "B_")
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (unitA);
    addAndMakeVisible (unitB);

    setResizable (true, true);
    setResizeLimits (640, 576, 1600, 1440);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kUnitW / (double) (kUnitH * 2.0f));
    setSize (960, 864);
}

MDHEditor::~MDHEditor()
{
    setLookAndFeel (nullptr);
}

void MDHEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MDHEditor::resized()
{
    const int w = getWidth();
    const int unitH = juce::roundToInt ((float) w * kUnitH / kUnitW);
    unitA.setBounds (0, 0, w, unitH);
    unitB.setBounds (0, unitH, w, getHeight() - unitH);
}
