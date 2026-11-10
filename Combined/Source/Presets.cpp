#include "Presets.h"

namespace
{
    // Values use the parameter's own units (choice/int = index or position).
    // "*" in an ID expands to both L and R, so presets also work in Dual Mono.
    struct Factory { const char* name; std::vector<std::pair<const char*, float>> values; };

    const std::vector<Factory>& factory()
    {
        static const std::vector<Factory> list {
            { "Default", {} },

            // Model B per the hardware manual's mastering recipe: ~1 dB optical,
            // ~2 dB discrete, 1.2:1, 30 ms attack, .1 s recover, sidechain filter in.
            { "Mastering Glue", {
                { "B_*_OPT_IN", 1 }, { "B_*_OPT_THR", 10 }, { "B_*_OPT_GAIN", 9 },
                { "B_*_DSC_IN", 1 }, { "B_*_DSC_THR", 13 }, { "B_*_DSC_RATIO", 0 }, { "B_*_DSC_ATTACK", 5 }, { "B_*_DSC_RECOVER", 0 },
                { "B_*_DSC_GAIN", 11 }, { "B_*_SC_FILTER", 1 }, { "B_*_XFMR", 2 }, { "A_BYPASS", 1 } } },

            { "Transparent Master (Nickel)", {
                { "A_BYPASS", 1 },
                { "B_*_OPT_IN", 0 }, { "B_*_DSC_IN", 1 }, { "B_*_DSC_THR", 9 }, { "B_*_DSC_RATIO", 0 },
                { "B_*_DSC_ATTACK", 5 }, { "B_*_DSC_RECOVER", 5 }, { "B_*_DSC_GAIN", 10 }, { "B_*_XFMR", 0 } } },

            { "Gentle Opto Leveler", {
                { "A_BYPASS", 1 },
                { "B_*_OPT_IN", 1 }, { "B_*_OPT_THR", 13 }, { "B_*_OPT_GAIN", 11 }, { "B_*_DSC_IN", 0 }, { "B_*_XFMR", 0 } } },

            { "Vocal Smoother", {
                { "A_*_OPT_IN", 1 }, { "A_*_OPT_THR", 14 }, { "A_*_OPT_GAIN", 11 },
                { "A_*_DSC_IN", 1 }, { "A_*_DSC_THR", 12 }, { "A_*_DSC_RATIO", 1 }, { "A_*_DSC_ATTACK", 3 }, { "A_*_DSC_RECOVER", 5 },
                { "A_*_DSC_GAIN", 10 }, { "A_*_XFMR", 1 }, { "A_*_SC_FILTER", 1 }, { "B_BYPASS", 1 } } },

            { "Punchy Drum Bus", {
                { "A_*_OPT_IN", 0 }, { "A_*_DSC_IN", 1 }, { "A_*_DSC_THR", 15 }, { "A_*_DSC_RATIO", 3 }, { "A_*_DSC_ATTACK", 4 },
                { "A_*_DSC_RECOVER", 0 }, { "A_*_DSC_GAIN", 12 }, { "A_*_XFMR", 2 }, { "A_*_SC_FILTER", 1 },
                { "B_BYPASS", 1 },
                { "C_*_IN", 1 }, { "C_*_THR", -4 }, { "C_*_RATIO", 1 }, { "C_*_ATTACK", 3 }, { "C_*_RELEASE", 4 }, { "C_*_MAKEUP", 2 } } },

            { "Class-A Polish", {
                { "A_BYPASS", 1 },
                { "B_*_OPT_IN", 0 }, { "B_*_DSC_IN", 1 }, { "B_*_DSC_THR", 11 }, { "B_*_DSC_RATIO", 0 }, { "B_*_DSC_ATTACK", 5 },
                { "B_*_DSC_RECOVER", 5 }, { "B_*_DSC_GAIN", 10 }, { "B_*_XFMR", 1 } } },

            { "Classic Bus Comp", {
                { "A_BYPASS", 1 }, { "B_BYPASS", 1 },
                { "C_*_IN", 1 }, { "C_*_THR", -6 }, { "C_*_RATIO", 1 }, { "C_*_ATTACK", 4 }, { "C_*_RELEASE", 4 }, { "C_*_MAKEUP", 3 } } },

            { "Steel Colour Only", {
                { "A_*_OPT_IN", 0 }, { "A_*_DSC_IN", 0 }, { "A_*_XFMR", 2 },
                { "B_*_OPT_IN", 0 }, { "B_*_DSC_IN", 0 }, { "B_*_XFMR", 2 } } },

            { "Flood Limiter", {
                { "A_BYPASS", 1 },
                { "B_*_OPT_IN", 0 }, { "B_*_DSC_IN", 1 }, { "B_*_DSC_THR", 14 }, { "B_*_DSC_RATIO", 5 },
                { "B_*_DSC_ATTACK", 0 }, { "B_*_DSC_RECOVER", 1 }, { "B_*_DSC_GAIN", 14 }, { "B_*_XFMR", 0 } } },

            { "Stack Everything", {
                { "A_*_OPT_IN", 1 }, { "A_*_OPT_THR", 10 }, { "A_*_DSC_THR", 9 }, { "A_*_XFMR", 1 },
                { "B_*_OPT_IN", 1 }, { "B_*_OPT_THR", 12 }, { "B_*_OPT_GAIN", 9 }, { "B_*_DSC_THR", 9 }, { "B_*_XFMR", 2 },
                { "C_*_IN", 1 }, { "C_*_THR", -2 }, { "C_*_RATIO", 0 }, { "C_*_ATTACK", 5 }, { "C_*_RELEASE", 4 }, { "C_*_MAKEUP", 1 } } },
        };
        return list;
    }
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

int PresetManager::getNumFactory() const { return (int) factory().size(); }

juce::String PresetManager::getFactoryName (int i) const
{
    return juce::isPositiveAndBelow (i, getNumFactory()) ? juce::String (factory()[(size_t) i].name) : juce::String();
}

void PresetManager::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void PresetManager::loadFactory (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumFactory()))
        return;

    // Start from defaults (UI-only settings such as zoom are kept).
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "MASTER_BYPASS")
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->getDefaultValue());
                rp->endChangeGesture();
            }

    for (const auto& [id, value] : factory()[(size_t) index].values)
    {
        const juce::String s (id);
        if (s.contains ("*"))
        {
            setParam (s.replace ("*", "L"), value);
            setParam (s.replace ("*", "R"), value);
        }
        else
            setParam (s, value);
    }
    currentFactory = index;
    currentName = factory()[(size_t) index].name;
}

juce::File PresetManager::getUserPresetFolder()
{
   #if JUCE_MAC
    auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile ("Library/Audio/Presets/Basemental/MacDanial Hills");
   #else
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Basemental/MacDanial Hills/Presets");
   #endif
    dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.mdhpreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool PresetManager::saveUser (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;

    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->setAttribute ("presetName", name);
        if (xml->writeTo (getUserPresetFolder().getChildFile (name + ".mdhpreset")))
        {
            currentName = name;
            currentFactory = -1;
            return true;
        }
    }
    return false;
}

bool PresetManager::loadUser (const juce::String& name)
{
    const auto file = getUserPresetFolder().getChildFile (name + ".mdhpreset");
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    const auto loaded = juce::ValueTree::fromXml (*xml);
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto child = loaded.getChildWithProperty ("id", rp->getParameterID());
            if (child.isValid() && rp->getParameterID() != "MASTER_BYPASS")
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
                rp->endChangeGesture();
            }
        }
    currentName = name;
    currentFactory = -1;
    return true;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    return getUserPresetFolder().getChildFile (name + ".mdhpreset").deleteFile();
}
