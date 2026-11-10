#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Factory + user presets for the stacked plug-in.
// Factory presets also appear in the DAW's own preset/program menu.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    int getNumFactory() const;
    juce::String getFactoryName (int index) const;
    void loadFactory (int index);

    juce::StringArray getUserPresetNames() const;
    bool loadUser (const juce::String& name);
    bool saveUser (const juce::String& name);
    bool deleteUser (const juce::String& name);
    static juce::File getUserPresetFolder();

    juce::String getCurrentName() const { return currentName; }
    int getCurrentFactoryIndex() const   { return currentFactory; }

private:
    void setParam (const juce::String& id, float denormalised);
    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentName { "Default" };
    int currentFactory = 0;
};
