#include "PluginEditor.h"

using namespace juce;

namespace
{
    const float kZooms[]     = { 0.6f, 0.75f, 0.9f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    constexpr int kUserIdBase = 1000;
}

//==============================================================================
void MDHEditor::Constrainer::checkBounds (Rectangle<int>& b, const Rectangle<int>& prev, const Rectangle<int>& limits,
                                          bool top, bool left, bool bottom, bool right)
{
    ComponentBoundsConstrainer::checkBounds (b, prev, limits, top, left, bottom, right);
    if (! locked || heightForWidth == nullptr) return;

    const bool verticalOnly = (top || bottom) && ! (left || right);
    if (verticalOnly && widthForHeight != nullptr)
        b.setWidth (jlimit (getMinimumWidth(), getMaximumWidth(), widthForHeight (b.getHeight())));
    b.setHeight (heightForWidth (b.getWidth()));
}

//==============================================================================
MDHEditor::MDHEditor (MDHAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      panelA (p.apvts, p.engineA, "A_"),
      panelB (p.apvts, p.engineB, "B_"),
      panelC (p.apvts, p.engineC, "C_")
{
    setLookAndFeel (&lnf);

    units[0].name = "Model A";        units[0].panel = &panelA; units[0].baseH = UnitPanelA::kBaseH;
    units[1].name = "Model B";        units[1].panel = &panelB; units[1].baseH = UnitPanelB::kBaseH;
    units[2].name = "Bus Compressor"; units[2].panel = &panelC; units[2].baseH = UnitPanelC::kBaseH;

    for (auto& u : units)
        addAndMakeVisible (*u.panel);

    allPoppedLabel.setText ("All units are popped out. Close a window to dock it back here.", dontSendNotification);
    allPoppedLabel.setJustificationType (Justification::centred);
    allPoppedLabel.setColour (Label::textColourId, Colours::lightgrey);
    addChildComponent (allPoppedLabel);

    buildToolbar();

    constrainer.heightForWidth = [this] (int w) { return heightForWidth (w); };
    constrainer.widthForHeight = [this] (int h)
    {
        const float hb = dockedBaseHeight();
        return hb > 0.0f ? roundToInt ((float) (h - kToolbarH) * kUnitW / hb) : getWidth();
    };
    constrainer.setSizeLimits (560, kToolbarH + 60, 3200, 4000);
    setConstrainer (&constrainer);
    setResizable (true, true);

    // restore UI state
    stretch = proc.uiStretch.load();
    stretchBtn.setToggleState (stretch, dontSendNotification);
    updateConstrainer();

    const int w = jlimit (560, 3200, proc.uiWidth.load());
    const int h = proc.uiHeight.load() > 0 ? proc.uiHeight.load() : heightForWidth (w);
    setSize (w, stretch ? h : heightForWidth (w));
}

MDHEditor::~MDHEditor()
{
    for (auto& u : units)
        u.window.reset();
    setLookAndFeel (nullptr);
}

//==============================================================================
void MDHEditor::buildToolbar()
{
    auto styleButton = [this] (TextButton& b)
    {
        b.setColour (TextButton::buttonColourId, Colour (0xff2a2a2d));
        b.setColour (TextButton::buttonOnColourId, Colour (0xffb5643a));
        b.setColour (TextButton::textColourOffId, Colour (0xffe8e4d8));
        b.setColour (TextButton::textColourOnId, Colours::white);
        addAndMakeVisible (b);
    };
    auto styleCombo = [this] (ComboBox& c)
    {
        c.setColour (ComboBox::backgroundColourId, Colour (0xff1a1a1c));
        c.setColour (ComboBox::textColourId, Colour (0xffe8e4d8));
        c.setColour (ComboBox::outlineColourId, Colour (0xff3a3a3d));
        c.setColour (ComboBox::arrowColourId, Colour (0xffe8e4d8));
        addAndMakeVisible (c);
    };

    styleCombo (presetBox);
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this] { if (! ignoreCallbacks) loadPresetFromMenu(); };
    refreshPresetList();

    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &deleteBtn, &stretchBtn })
        styleButton (*b);
    prevBtn.onClick   = [this] { stepPreset (-1); };
    nextBtn.onClick   = [this] { stepPreset (+1); };
    saveBtn.onClick   = [this] { savePresetDialog(); };
    deleteBtn.onClick = [this] { deleteCurrentPreset(); };
    prevBtn.setTooltip ("Previous preset");
    nextBtn.setTooltip ("Next preset");

    styleCombo (zoomBox);
    for (int i = 0; i < (int) std::size (kZooms); ++i)
        zoomBox.addItem ("Zoom " + String (roundToInt (kZooms[i] * 100.0f)) + " %", i + 1);
    zoomBox.setTextWhenNothingSelected ("Zoom");
    zoomBox.onChange = [this]
    {
        if (ignoreCallbacks) return;
        const int i = zoomBox.getSelectedId() - 1;
        if (isPositiveAndBelow (i, (int) std::size (kZooms))) setZoom (kZooms[i]);
    };

    stretchBtn.setClickingTogglesState (true);
    stretchBtn.setTooltip ("Stretch: resize width and height freely");
    stretchBtn.onClick = [this] { setStretch (stretchBtn.getToggleState()); };

    for (auto& u : units)
    {
        u.popButton.setButtonText ("Pop out " + (u.name == "Bus Compressor" ? String ("C") : u.name.getLastCharacters (1)));
        u.popButton.setTooltip ("Open " + u.name + " in its own window");
        u.popButton.setClickingTogglesState (false);
        styleButton (u.popButton);
        u.popButton.onClick = [this, &u] { togglePopOut (u); };
    }
}

void MDHEditor::refreshPresetList()
{
    const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
    presetBox.clear (dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < proc.presets.getNumFactory(); ++i)
        presetBox.addItem (proc.presets.getFactoryName (i), i + 1);

    const auto user = proc.presets.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < user.size(); ++i)
            presetBox.addItem (user[i], kUserIdBase + i);
    }

    const auto current = proc.presets.getCurrentName();
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == current)
            presetBox.setSelectedItemIndex (i, dontSendNotification);
}

void MDHEditor::loadPresetFromMenu()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    if (id >= kUserIdBase)
        proc.presets.loadUser (presetBox.getText());
    else
        proc.presets.loadFactory (id - 1);
}

void MDHEditor::stepPreset (int delta)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex();
    for (int tries = 0; tries < n; ++tries)
    {
        idx = (idx + delta + n) % n;
        if (presetBox.getItemId (idx) != 0) break;   // skip headings/separators
    }
    presetBox.setSelectedItemIndex (idx, sendNotificationSync);
}

void MDHEditor::savePresetDialog()
{
    auto* aw = new AlertWindow ("Save preset", "Name this preset:", MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", proc.presets.getCurrentName());
    aw->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    aw->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

    Component::SafePointer<MDHEditor> safe (this);
    aw->enterModalState (true, ModalCallbackFunction::create ([safe, aw] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            if (safe->proc.presets.saveUser (aw->getTextEditorContents ("name")))
                safe->refreshPresetList();
        }
    }), true);
}

void MDHEditor::deleteCurrentPreset()
{
    if (presetBox.getSelectedId() < kUserIdBase)
        return;   // factory presets can't be deleted

    const auto name = presetBox.getText();
    Component::SafePointer<MDHEditor> safe (this);
    AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon, "Delete preset",
                                  "Delete the user preset \"" + name + "\"?", "Delete", "Cancel", this,
                                  ModalCallbackFunction::create ([safe, name] (int r)
                                  {
                                      if (r == 1 && safe != nullptr && safe->proc.presets.deleteUser (name))
                                          safe->refreshPresetList();
                                  }));
}

//==============================================================================
float MDHEditor::dockedBaseHeight() const
{
    float h = 0.0f;
    for (auto& u : units)
        if (u.window == nullptr) h += u.baseH;
    return h;
}

int MDHEditor::heightForWidth (int w) const
{
    const float hb = dockedBaseHeight();
    return kToolbarH + (hb > 0.0f ? roundToInt ((float) w * hb / kUnitW) : 60);
}

void MDHEditor::updateConstrainer()
{
    constrainer.locked = ! stretch;
}

void MDHEditor::setZoom (float zoom)
{
    const int w = roundToInt ((float) kBaseWidthAt100 * zoom);
    if (stretch)
    {
        const float sy = (float) (getHeight() - kToolbarH) / (float) jmax (1, getWidth());
        setSize (w, kToolbarH + roundToInt ((float) w * sy));
    }
    else
        setSize (w, heightForWidth (w));
}

void MDHEditor::setStretch (bool s)
{
    stretch = s;
    updateConstrainer();
    for (auto& u : units)
        if (u.window != nullptr) u.window->setStretch (s);
    if (! stretch) setSize (getWidth(), heightForWidth (getWidth()));
    storeUiState();
}

void MDHEditor::togglePopOut (Unit& u)
{
    if (u.window != nullptr) { dock (u); return; }

    const auto screenPos = u.panel->getScreenBounds();
    removeChildComponent (u.panel);

    Component::SafePointer<MDHEditor> safe (this);
    Unit* unitPtr = &u;
    u.window = std::make_unique<PopOutWindow> ("MacDanial Hills - " + u.name, *u.panel, kUnitW, u.baseH,
        screenPos.translated (40, 40),
        [safe, unitPtr]
        {
            MessageManager::callAsync ([safe, unitPtr] { if (safe != nullptr) safe->dock (*unitPtr); });
        });
    u.window->setStretch (stretch);
    u.popButton.setButtonText ("Dock " + (u.name == "Bus Compressor" ? String ("C") : u.name.getLastCharacters (1)));

    setSize (getWidth(), stretch ? jmax (kToolbarH + 60, getHeight() - screenPos.getHeight()) : heightForWidth (getWidth()));
    resized();
}

void MDHEditor::dock (Unit& u)
{
    if (u.window == nullptr) return;
    u.window.reset();
    addAndMakeVisible (*u.panel);
    u.popButton.setButtonText ("Pop out " + (u.name == "Bus Compressor" ? String ("C") : u.name.getLastCharacters (1)));

    if (stretch)
        setSize (getWidth(), getHeight() + roundToInt ((float) getWidth() * u.baseH / kUnitW));
    else
        setSize (getWidth(), heightForWidth (getWidth()));
    resized();
}

void MDHEditor::storeUiState()
{
    proc.uiWidth   = getWidth();
    proc.uiHeight  = getHeight();
    proc.uiStretch = stretch;
}

//==============================================================================
void MDHEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff0b0b0c));
    auto bar = getLocalBounds().removeFromTop (kToolbarH).toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff232326), 0.0f, 0.0f, Colour (0xff141416), 0.0f, bar.getBottom(), false));
    g.fillRect (bar);
    g.setColour (Colour (0xffe8e4d8));
    g.setFont (od2::PanelLookAndFeel::panelFont (13.0f, true).withExtraKerningFactor (0.08f));
    g.drawText ("MACDANIAL HILLS", bar.withTrimmedLeft (10.0f).withWidth (150.0f).withTrimmedBottom (13.0f).withTrimmedTop (3.0f),
                Justification::centredLeft, false);
    g.setColour (Colour (0xff8f8a80));
    g.setFont (od2::PanelLookAndFeel::panelFont (10.0f, false));
    g.drawText ("v" JucePlugin_VersionString, bar.withTrimmedLeft (10.0f).withWidth (150.0f).withTrimmedTop (22.0f),
                Justification::centredLeft, false);
    g.setColour (Colours::black);
    g.drawLine (0.0f, bar.getBottom() - 0.5f, (float) getWidth(), bar.getBottom() - 0.5f, 1.0f);
}

void MDHEditor::resized()
{
    // ---- toolbar ----
    auto bar = getLocalBounds().removeFromTop (kToolbarH).reduced (6, 6);
    bar.removeFromLeft (150);
    presetBox.setBounds (bar.removeFromLeft (jmin (230, jmax (120, getWidth() / 5))));
    bar.removeFromLeft (4);
    prevBtn.setBounds (bar.removeFromLeft (26));
    bar.removeFromLeft (2);
    nextBtn.setBounds (bar.removeFromLeft (26));
    bar.removeFromLeft (6);
    saveBtn.setBounds (bar.removeFromLeft (52));
    bar.removeFromLeft (4);
    deleteBtn.setBounds (bar.removeFromLeft (56));

    for (int i = (int) units.size() - 1; i >= 0; --i)
    {
        units[(size_t) i].popButton.setBounds (bar.removeFromRight (78));
        bar.removeFromRight (4);
    }
    bar.removeFromRight (6);
    stretchBtn.setBounds (bar.removeFromRight (70));
    bar.removeFromRight (6);
    zoomBox.setBounds (bar.removeFromRight (jmin (120, jmax (90, bar.getWidth()))));

    {   // reflect current zoom in the menu
        const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
        int match = 0;
        for (int i = 0; i < (int) std::size (kZooms); ++i)
            if (std::abs ((float) getWidth() - kZooms[i] * (float) kBaseWidthAt100) < 2.0f) match = i + 1;
        zoomBox.setSelectedId (match, dontSendNotification);
        if (match == 0) zoomBox.setText ("Zoom " + String (roundToInt (100.0f * (float) getWidth() / (float) kBaseWidthAt100)) + " %", dontSendNotification);
    }

    // ---- units ----
    auto area = getLocalBounds().withTrimmedTop (kToolbarH);
    const float hb = dockedBaseHeight();
    allPoppedLabel.setVisible (hb <= 0.0f);
    allPoppedLabel.setBounds (area);

    int y = area.getY();
    int docked = 0, numDocked = 0;
    for (auto& u : units) if (u.window == nullptr) ++numDocked;

    for (auto& u : units)
    {
        if (u.window != nullptr) continue;
        ++docked;
        // Stretch: share the available height in proportion; otherwise keep each unit's own aspect.
        int h = stretch ? roundToInt ((float) area.getHeight() * u.baseH / hb)
                        : roundToInt ((float) area.getWidth() * u.baseH / kUnitW);
        if (docked == numDocked) h = jmax (h, area.getBottom() - y);
        u.panel->setBounds (area.getX(), y, area.getWidth(), h);
        y += h;
    }

    storeUiState();
}
