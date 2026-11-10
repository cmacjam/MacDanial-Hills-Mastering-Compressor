#include "UnitPanelC.h"

using namespace juce;
using namespace od2;

namespace
{
    constexpr float kKnobY = 80.0f, kKnobR = 31.0f;
    const float kLeftX[5]  = { 178.0f, 268.0f, 358.0f, 448.0f, 538.0f };
    const float kRightX[5] = { 948.0f, 1038.0f, 1128.0f, 1218.0f, 1308.0f };
}

//==============================================================================
void GrMeter::setGr (float grDb, float dt)
{
    const float old = shown;
    shown += (1.0f - std::exp (-dt / 0.08f)) * (jlimit (0.0f, 22.0f, grDb) - shown);
    if (std::abs (shown - old) > 0.01f) repaint();
}

void GrMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // bezel
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (b.translated (2.0f, 3.0f).reduced (1.0f), 6.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2b2b2d), b.getX(), b.getY(), Colour (0xff080808), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (1.0f), 6.0f);

    auto face = b.reduced (b.getWidth() * 0.07f, b.getHeight() * 0.08f).withTrimmedBottom (b.getHeight() * 0.12f);
    g.setColour (Colour (0xff121214));
    g.fillRoundedRectangle (face, 4.0f);

    const Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f);
    const float R = face.getHeight() * 0.95f;
    auto angleFor = [] (float gr) { return degreesToRadians (42.0f - 84.0f * gr / 20.0f); };

    g.setColour (Colour (0xffe8e4d8));
    Path arc;
    arc.addCentredArc (pivot.x, pivot.y, R, R, 0.0f, angleFor (20.0f), angleFor (0.0f), true);
    g.strokePath (arc, PathStrokeType (1.2f));
    g.setFont (PanelLookAndFeel::panelFont (face.getHeight() * 0.13f, true));
    for (int db = 0; db <= 20; db += 2)
    {
        const float a = angleFor ((float) db);
        const bool major = db % 4 == 0;
        g.drawLine (Line<float> (pivot.getPointOnCircumference (R, a), pivot.getPointOnCircumference (R + (major ? 7.0f : 4.0f), a)), 1.2f);
        if (major)
        {
            const auto lp = pivot.getPointOnCircumference (R + 16.0f, a);
            g.drawText (String (db), Rectangle<float> (lp.x - 14.0f, lp.y - 7.0f, 28.0f, 14.0f), Justification::centred, false);
        }
    }
    g.setFont (PanelLookAndFeel::panelFont (face.getHeight() * 0.11f, true));
    g.drawText ("dB", Rectangle<float> (face.getX(), face.getY() + face.getHeight() * 0.38f, face.getWidth(), 14.0f), Justification::centred, false);
    g.setFont (PanelLookAndFeel::panelFont (face.getHeight() * 0.09f, true));
    g.drawText ("COMPRESSION", Rectangle<float> (face.getX(), face.getY() + face.getHeight() * 0.52f, face.getWidth(), 12.0f), Justification::centred, false);

    // needle
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (face.toNearestInt());
        const auto tip = pivot.getPointOnCircumference (R * 1.04f, angleFor (shown));
        g.setColour (Colour (0xfff4f1e8));
        g.drawLine (Line<float> (pivot, tip), 1.6f);
    }
    // zero-adjust screw
    const Point<float> sc (b.getCentreX(), b.getBottom() - b.getHeight() * 0.1f);
    PanelLookAndFeel::drawScrew (g, sc, b.getHeight() * 0.035f, 1.57f);
}

//==============================================================================
UnitPanelC::UnitPanelC (AudioProcessorValueTreeState& s, od2c::BusComp& c, String paramPrefix)
    : apvts (s), comp (c), prefix (std::move (paramPrefix))
{
    const StringArray thrScale { "-15", "", "", "0", "", "", "+15" };
    const StringArray ratios { "2", "4", "10" };
    const StringArray attacks { ".1", ".3", "1", "3", "10", "30" };
    const StringArray releases { ".1", ".3", ".6", "1.2", "AUTO" };
    const StringArray makeup { "0", "", "", "15" };

    for (int ch = 0; ch < 2; ++ch)
    {
        auto& c = chans[(size_t) ch];
        const float* xs = ch == 0 ? kLeftX : kRightX;
        const String side = ch == 0 ? "L_" : "R_";
        addKnob (c.thr,     prefix + side + "THR",     xs[0], thrScale, "THRESHOLD");
        addKnob (c.ratio,   prefix + side + "RATIO",   xs[1], ratios,   "RATIO");
        addKnob (c.attack,  prefix + side + "ATTACK",  xs[2], attacks,  "ATTACK");
        addKnob (c.release, prefix + side + "RELEASE", xs[3], releases, "RELEASE");
        addKnob (c.makeup,  prefix + side + "MAKEUP",  xs[4], makeup,   "MAKE-UP");

        addAndMakeVisible (c.meter);
        placements.emplace_back (&c.meter, ch == 0 ? Rectangle<float> (606.0f, 12.0f, 176.0f, 160.0f)
                                                   : Rectangle<float> (1392.0f, 12.0f, 176.0f, 160.0f));
    }

    addLamp (power,       prefix + "POWER",  "POWER",  {  58.0f,  20.0f, 66.0f, 54.0f });
    addLamp (chans[0].in, prefix + "L_IN",   "IN",     {  58.0f, 106.0f, 66.0f, 54.0f });
    addLamp (stereo,      prefix + "STEREO", "STEREO", { 820.0f,  20.0f, 66.0f, 54.0f });
    addLamp (chans[1].in, prefix + "R_IN",   "IN",     { 820.0f, 106.0f, 66.0f, 54.0f });

    for (const char* base : { "THR", "RATIO", "ATTACK", "RELEASE", "MAKEUP" })
    {
        LinkPair lp { apvts.getParameter (prefix + "L_" + base), apvts.getParameter (prefix + "R_" + base) };
        if (lp.l != nullptr && lp.r != nullptr) { lp.lastL = lp.l->getValue(); lp.lastR = lp.r->getValue(); links.push_back (lp); }
    }
    rawStereo  = apvts.getRawParameterValue (prefix + "STEREO");
    lastStereo = rawStereo != nullptr && rawStereo->load() > 0.5f;
    lastTickMs = Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

UnitPanelC::~UnitPanelC() { stopTimer(); }

void UnitPanelC::addKnob (Slider& s, const String& id, float cx, const StringArray& scale, const String& label)
{
    s.getProperties().set ("style", "cream");
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (kRotStart, kRotEnd, true);
    s.setPopupDisplayEnabled (true, true, getParentComponent() != nullptr ? getParentComponent() : this);
    addAndMakeVisible (s);
    sliderAtts.push_back (std::make_unique<SliderAtt> (apvts, id, s));
    if (auto* p = apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    knobSpecs.push_back ({ cx, scale, label });
    placements.emplace_back (&s, Rectangle<float> (cx - kKnobR, kKnobY - kKnobR, kKnobR * 2.0f, kKnobR * 2.0f));
}

void UnitPanelC::addLamp (ToggleButton& b, const String& id, const String& text, Rectangle<float> r)
{
    b.getProperties().set ("style", "lamp");
    b.setButtonText (text);
    addAndMakeVisible (b);
    buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, id, b));
    placements.emplace_back (&b, r);
}

void UnitPanelC::resized()
{
    // Independent X/Y scale so the panel can be zoomed or stretched.
    const float sx = (float) getWidth() / kBaseW, sy = (float) getHeight() / kBaseH;
    for (auto& [comp_, r] : placements)
        comp_->setBounds (Rectangle<float> (r.getX() * sx, r.getY() * sy, r.getWidth() * sx, r.getHeight() * sy).toNearestInt());
    renderBackground();
}

void UnitPanelC::paint (Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());
}

void UnitPanelC::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    constexpr float k = 2.0f;
    background = Image (Image::RGB, (int) (w * k), (int) (h * k), true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (k * (float) w / kBaseW, k * (float) h / kBaseH));

    const Rectangle<float> panel (0.0f, 0.0f, kBaseW, kBaseH);
    g.setGradientFill (ColourGradient (Colour (0xff1c1c1f), 0.0f, 0.0f, Colour (0xff0d0d0f), 0.0f, kBaseH, false));
    g.fillRect (panel);
    Random rng (0xB05);
    for (int i = 0; i < 300; ++i)
    {
        const float y = rng.nextFloat() * kBaseH, x = rng.nextFloat() * kBaseW;
        g.setColour (Colours::white.withAlpha (0.01f + rng.nextFloat() * 0.015f));
        g.drawLine (x, y, x + 60.0f + rng.nextFloat() * 160.0f, y, 0.6f);
    }
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (0.0f, 1.0f, kBaseW, 1.0f, 1.5f);
    g.setColour (Colours::black);
    g.drawLine (0.0f, kBaseH - 1.0f, kBaseW, kBaseH - 1.0f, 2.0f);

    for (auto pt : { Point<float> (26.0f, 32.0f), Point<float> (26.0f, 152.0f), Point<float> (1574.0f, 32.0f), Point<float> (1574.0f, 152.0f) })
        PanelLookAndFeel::drawScrew (g, pt, 9.0f, 0.4f + pt.y * 0.01f);

    const auto legend = colours::legend;
    for (int side = 0; side < 2; ++side)
        for (size_t i = 0; i < 5; ++i)
        {
            const auto& ks = knobSpecs[(size_t) side * 5 + i];
            const Point<float> c (ks.cx, kKnobY);
            g.setColour (legend);
            const int n = ks.scale.size();
            g.setFont (PanelLookAndFeel::panelFont (11.0f, true));
            for (int j = 0; j < n; ++j)
            {
                const float a = kRotStart + (kRotEnd - kRotStart) * (float) j / (float) jmax (1, n - 1);
                g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (c.getPointOnCircumference (kKnobR + 6.0f, a)));
                if (ks.scale[j].isNotEmpty())
                {
                    const auto lp = c.getPointOnCircumference (kKnobR + (ks.scale[j].length() > 3 ? 22.0f : 16.0f), a);
                    g.drawText (ks.scale[j], Rectangle<float> (lp.x - 20.0f, lp.y - 7.0f, 40.0f, 14.0f), Justification::centred, false);
                }
            }
            g.setFont (PanelLookAndFeel::panelFont (12.5f, true));
            g.drawText (ks.label, Rectangle<float> (ks.cx - 50.0f, 138.0f, 100.0f, 16.0f), Justification::centred, false);
        }

    g.setColour (legend.withAlpha (0.75f));
    g.setFont (PanelLookAndFeel::panelFont (11.0f, true).withExtraKerningFactor (0.1f));
    g.drawText ("MACDANIAL HILLS  -  BUS COMPRESSOR", Rectangle<float> (700.0f, 166.0f, 320.0f, 14.0f), Justification::centred, false);
}

void UnitPanelC::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = (float) jlimit (0.001, 0.1, (now - lastTickMs) * 0.001);
    lastTickMs = now;

    for (int ch = 0; ch < 2; ++ch)
        chans[(size_t) ch].meter.setGr (comp.grDb[ch].load (std::memory_order_relaxed), dt);

    // Stereo: left/right knobs move together.
    const bool st = rawStereo != nullptr && rawStereo->load() > 0.5f;
    for (auto& lp : links)
    {
        const float l = lp.l->getValue(), r = lp.r->getValue();
        auto push = [] (RangedAudioParameter* p, float v) { p->beginChangeGesture(); p->setValueNotifyingHost (v); p->endChangeGesture(); };
        if (st)
        {
            if ((st && ! lastStereo) || l != lp.lastL) { if (r != l) push (lp.r, l); }
            else if (r != lp.lastR) push (lp.l, r);
        }
        lp.lastL = lp.l->getValue();
        lp.lastR = lp.r->getValue();
    }
    lastStereo = st;
}
