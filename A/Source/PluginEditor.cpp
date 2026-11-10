#include "PluginEditor.h"

using namespace juce;
using namespace od2;

namespace
{
    constexpr float kVuRefDbfs = -14.0f;   // Output meter: 0 VU = -14 dBFS RMS
}

//==============================================================================
OD2Editor::OD2Editor (OD2AudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    buildChannel (0);
    buildChannel (1);

    // ---- centre section -------------------------------------------------------
    addToggle (stereoSw, ids::stereo, 800.0f, 450.0f, {}, {}, {}, {}, true);
    addToggle (hardwire, ids::hardwireBypass, 800.0f, 545.0f, "HARDWIRE", "BYPASS", "IN", "OUT", true);

    monoLed.setColour (colours::ledBlue);
    stereoLed.setColour (colours::ledRed);
    for (auto* l : { &monoLed, &stereoLed }) addAndMakeVisible (*l);
    place (monoLed,   { 722.0f, 437.0f, 26.0f, 26.0f });
    place (stereoLed, { 852.0f, 437.0f, 26.0f, 26.0f });
    textSpecs.push_back ({ "DUAL MONO", 735.0f, 420.0f, 12.0f, true });
    textSpecs.push_back ({ "STEREO",    865.0f, 420.0f, 12.0f, true });

    addAndMakeVisible (eye);
    place (eye, { 764.0f, 592.0f, 72.0f, 72.0f });

    rawStereo = proc.apvts.getRawParameterValue (ids::stereo);

    // ---- window ---------------------------------------------------------------
    setResizable (true, true);
    setResizeLimits (800, 360, 1600, 720);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    setSize (1200, 540);

    lastTickMs = Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

OD2Editor::~OD2Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
StringArray OD2Editor::range (int from, int to)
{
    StringArray s;
    for (int i = from; i <= to; ++i) s.add (String (i));
    return s;
}

void OD2Editor::place (Component& c, Rectangle<float> r)
{
    placements.emplace_back (&c, r);
}

void OD2Editor::addKnob (Slider& s, const String& id, float cx, float cy, float r,
                         const StringArray& labels, const String& title)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (kRotStart, kRotEnd, true);
    s.setPopupDisplayEnabled (true, true, this);
    s.setMouseDragSensitivity (labels.size() <= 8 ? 140 : 260);
    addAndMakeVisible (s);
    sliderAtts.push_back (std::make_unique<SliderAtt> (proc.apvts, id, s));

    if (auto* param = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    knobSpecs.push_back ({ cx, cy, r, labels, title });
    place (s, { cx - r * 1.05f, cy - r * 1.05f, r * 2.1f, r * 2.1f });
}

void OD2Editor::addToggle (ToggleButton& b, const String& id, float cx, float cy,
                           const String& top, const String& bottom, const String& left, const String& right,
                           bool onIsRight)
{
    b.getProperties().set ("onIsRight", onIsRight);
    addAndMakeVisible (b);
    buttonAtts.push_back (std::make_unique<ButtonAtt> (proc.apvts, id, b));
    toggleSpecs.push_back ({ cx, cy, top, bottom, left, right });
    place (b, { cx - 30.0f, cy - 19.0f, 60.0f, 38.0f });
}

void OD2Editor::buildChannel (int ch)
{
    auto& c = chans[(size_t) ch];
    auto mx = [ch] (float x) { return ch == 0 ? x : kBaseW - x; };
    auto id = [ch] (const char* base) { return ids::ch (ch, base); };

    // ---- knobs (positions follow the hardware faceplate, mirrored for the right channel)
    addKnob (c.optThr,  id (ids::optThr),    mx (185.0f), 115.0f, 58.0f, range (1, 24), "OPTICAL THRESHOLD");
    addKnob (c.optGain, id (ids::optGain),   mx (340.0f), 290.0f, 58.0f, range (0, 24), "OPTICAL GAIN");
    addKnob (c.ratio,   id (ids::dscRatio),  mx (185.0f), 430.0f, 58.0f, { "1.2", "2", "3", "4", "6", "FLOOD" }, "DISCRETE RATIO");
    addKnob (c.attack,  id (ids::dscAttack), mx (355.0f), 455.0f, 48.0f, { ".1", ".5", "1", "5", "10", "30" }, "DISCRETE ATTACK");
    addKnob (c.dscThr,  id (ids::dscThr),    mx (190.0f), 625.0f, 58.0f, range (1, 24), "DISCRETE THRESHOLD");
    addKnob (c.recover, id (ids::dscRecov),  mx (355.0f), 625.0f, 48.0f, { ".1", ".25", ".5", ".8", "1.2", "DUAL" }, "DISCRETE RECOVER");
    addKnob (c.dscGain, id (ids::dscGain),   mx (525.0f), 600.0f, 72.0f, range (0, 24), "DISCRETE GAIN");

    // ---- bat toggles
    addToggle (c.optIn,    id (ids::optIn),    mx (188.0f), 270.0f, "OPTICAL",   "BYPASS", "IN", "OUT", false);
    addToggle (c.scFilter, id (ids::scFilter), mx (482.0f), 440.0f, "SIDECHAIN", "FILTER", "IN", "OUT", false);
    addToggle (c.dscIn,    id (ids::dscIn),    mx (632.0f), 455.0f, "DISCRETE",  "BYPASS", "IN", "OUT", false);

    // ---- stage LEDs (lit = section IN)
    c.optLed.setColour (colours::ledRed);
    c.dscLed.setColour (colours::ledRed);
    addAndMakeVisible (c.optLed);
    addAndMakeVisible (c.dscLed);
    place (c.optLed, { mx (240.0f) - 13.0f, 222.0f - 13.0f, 26.0f, 26.0f });
    place (c.dscLed, { mx (684.0f) - 13.0f, 428.0f - 13.0f, 26.0f, 26.0f });

    // ---- transformer select (3-way toggle with blue LEDs)
    if (auto* xp = proc.apvts.getParameter (id (ids::xfmr)))
    {
        c.xfmr = std::make_unique<LedSelector> (*xp, LedSelector::Style::verticalToggle, colours::ledBlue, ch == 1);
        addAndMakeVisible (*c.xfmr);
        const float x0 = ch == 0 ? 660.0f : 865.0f;
        place (*c.xfmr, { x0, 535.0f, 75.0f, 120.0f });
        textSpecs.push_back ({ "NICKEL", x0 + 37.5f, 526.0f, 12.0f, true });
        textSpecs.push_back ({ "STEEL",  x0 + 37.5f, 666.0f, 12.0f, true });
        textSpecs.push_back ({ "IRON",   ch == 0 ? x0 - 12.0f : x0 + 87.0f, 595.0f, 10.0f, true });
    }

    // ---- meter select (mini knob with LEDs on an arc)
    if (auto* mp = proc.apvts.getParameter (id (ids::meterSel)))
    {
        c.meterSel = std::make_unique<LedSelector> (*mp, LedSelector::Style::miniKnob, colours::ledBlue, false);
        addAndMakeVisible (*c.meterSel);
        const float kx = mx (345.0f);
        place (*c.meterSel, { kx - 55.0f, 50.0f, 110.0f, 90.0f });   // knob centre = (kx, 113)
        const float spread = degreesToRadians (60.0f);
        arcTexts.push_back ({ "OPTICAL",  kx, 113.0f, 61.0f, -spread, 11.0f });
        arcTexts.push_back ({ "DISCRETE", kx, 113.0f, 61.0f,  0.0f,   11.0f });
        arcTexts.push_back ({ "OUTPUT",   kx, 113.0f, 61.0f,  spread, 11.0f });
    }

    // ---- VU meter + O / D / VU indicator LEDs beneath it
    c.vu.setBrand ("basemental audio");
    addAndMakeVisible (c.vu);
    place (c.vu, { ch == 0 ? 425.0f : 810.0f, 35.0f, 365.0f, 340.0f });

    const float xs[2][3] = { { 545.0f, 620.0f, 695.0f }, { 905.0f, 980.0f, 1055.0f } };
    const char* names[3] = { "O", "D", "VU" };
    for (int k = 0; k < 3; ++k)
    {
        auto& led = c.meterLeds[(size_t) k];
        led.setColour (k == 2 ? colours::ledBlue : colours::ledRed);
        addAndMakeVisible (led);
        place (led, { xs[ch][k] - 11.0f, 394.0f - 11.0f, 22.0f, 22.0f });
        textSpecs.push_back ({ names[k], xs[ch][k] - 20.0f, 394.0f, 10.0f, true });
    }

    rawOptIn[(size_t) ch] = proc.apvts.getRawParameterValue (id (ids::optIn));
    rawDscIn[(size_t) ch] = proc.apvts.getRawParameterValue (id (ids::dscIn));
    rawMeter[(size_t) ch] = proc.apvts.getRawParameterValue (id (ids::meterSel));
}

//==============================================================================
void OD2Editor::resized()
{
    const float s = (float) getWidth() / kBaseW;
    for (auto& [comp, r] : placements)
        comp->setBounds ((r * s).toNearestInt());

    renderBackground();
}

void OD2Editor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;

    constexpr float k = 2.0f;
    background = Image (Image::RGB, (int) (w * k), (int) (h * k), true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (k * (float) w / kBaseW));
    drawPanel (g);
}

void OD2Editor::paint (Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());
}

void OD2Editor::drawPanel (Graphics& g)
{
    const Rectangle<float> panel (0.0f, 0.0f, kBaseW, kBaseH);

    // ---- anodised black faceplate with a faint brushed grain ----
    g.setGradientFill (ColourGradient (colours::panelLight, 0.0f, 0.0f, colours::panel, 0.0f, kBaseH, false));
    g.fillRect (panel);

    Random rng (0x0D2);
    for (int i = 0; i < 900; ++i)
    {
        const float y = rng.nextFloat() * kBaseH;
        const float x = rng.nextFloat() * kBaseW;
        g.setColour (Colours::white.withAlpha (0.012f + rng.nextFloat() * 0.015f));
        g.drawLine (x, y, x + 40.0f + rng.nextFloat() * 160.0f, y, 0.6f);
    }

    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawRect (panel.reduced (1.0f), 2.0f);
    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawRect (panel, 1.0f);

    // ---- rack screws ----
    for (auto pt : { Point<float> (45.0f, 170.0f), Point<float> (45.0f, 560.0f),
                     Point<float> (1555.0f, 170.0f), Point<float> (1555.0f, 560.0f) })
        PanelLookAndFeel::drawScrew (g, pt, 11.0f, 0.6f + pt.x * 0.001f);

    const auto legend = colours::legend;

    // ---- brand & model ----
    g.setColour (legend);
    g.setFont (PanelLookAndFeel::panelFont (22.0f).withExtraKerningFactor (0.12f));
    g.drawText ("BASEMENTAL AUDIO", Rectangle<float> (500.0f, 6.0f, 600.0f, 28.0f), Justification::centred, false);
    g.setFont (PanelLookAndFeel::panelFont (22.0f).withExtraKerningFactor (0.08f));
    g.drawText ("MASTERING COMPRESSOR", Rectangle<float> (500.0f, 680.0f, 600.0f, 30.0f), Justification::centred, false);
    g.setFont (PanelLookAndFeel::panelFont (11.0f));
    g.setColour (legend.withAlpha (0.7f));
    g.drawText ("OD-2", Rectangle<float> (700.0f, 668.0f, 200.0f, 14.0f), Justification::centred, false);

    // ---- knob dials: ring, ticks, numerals and curved title ----
    for (const auto& ks : knobSpecs)
    {
        const Point<float> c (ks.cx, ks.cy);
        const int n = ks.labels.size();
        const float ringR = ks.r * 1.12f;
        const float numR  = ks.r * (ks.r > 60.0f ? 1.30f : 1.33f);
        const float numSize = (n > 12) ? (ks.r > 60.0f ? 12.0f : 10.5f) : 12.0f;

        Path ring;
        ring.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, kRotStart, kRotEnd, true);
        g.setColour (legend.withAlpha (0.85f));
        g.strokePath (ring, PathStrokeType (1.3f));

        g.setFont (PanelLookAndFeel::panelFont (numSize, true));
        for (int i = 0; i < n; ++i)
        {
            const float a = kRotStart + (kRotEnd - kRotStart) * (float) i / (float) jmax (1, n - 1);
            const auto t1 = c.getPointOnCircumference (ringR, a);
            const auto t2 = c.getPointOnCircumference (ringR + 5.0f, a);
            g.drawLine (Line<float> (t1, t2), 1.3f);

            const auto lp = c.getPointOnCircumference (numR + (ks.labels[i].length() > 3 ? 8.0f : 0.0f), a);
            g.drawText (ks.labels[i], Rectangle<float> (lp.x - 22.0f, lp.y - numSize * 0.6f, 44.0f, numSize * 1.2f),
                        Justification::centred, false);
        }

        PanelLookAndFeel::drawTextOnArc (g, ks.title, PanelLookAndFeel::panelFont (ks.r > 60.0f ? 17.0f : 15.0f)
                                                          .withExtraKerningFactor (0.06f),
                                         c, ks.r * (ks.r > 60.0f ? 1.55f : 1.64f), 0.0f);
    }

    // ---- toggle legends ----
    g.setFont (PanelLookAndFeel::panelFont (12.5f, true));
    for (const auto& ts : toggleSpecs)
    {
        auto put = [&g] (const String& t, float x, float y)
        {
            if (t.isNotEmpty())
                g.drawText (t, Rectangle<float> (x - 50.0f, y - 8.0f, 100.0f, 16.0f), Justification::centred, false);
        };
        put (ts.top,    ts.cx,         ts.cy - 30.0f);
        put (ts.bottom, ts.cx,         ts.cy + 30.0f);
        put (ts.left,   ts.cx - 45.0f, ts.cy);
        put (ts.right,  ts.cx + 47.0f, ts.cy);
    }

    // ---- misc straight legends ----
    for (const auto& t : textSpecs)
    {
        g.setFont (PanelLookAndFeel::panelFont (t.size, t.bold));
        g.drawText (t.text, Rectangle<float> (t.x - 50.0f, t.y - t.size * 0.65f, 100.0f, t.size * 1.3f),
                    Justification::centred, false);
    }

    // ---- curved meter-select legends ----
    for (const auto& at : arcTexts)
        PanelLookAndFeel::drawTextOnArc (g, at.text, PanelLookAndFeel::panelFont (at.size).withExtraKerningFactor (0.05f),
                                         { at.cx, at.cy }, at.radius, at.angle);
}

//==============================================================================
void OD2Editor::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = (float) jlimit (0.001, 0.1, (now - lastTickMs) * 0.001);
    lastTickMs = now;

    const bool stereo = rawStereo->load() > 0.5f;
    auto& eng = proc.engine;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto& c = chans[(size_t) ch];
        const int eff = stereo ? 0 : ch;   // in Stereo the left switches govern both sides

        c.optLed.setOn (rawOptIn[(size_t) eff]->load() > 0.5f);
        c.dscLed.setOn (rawDscIn[(size_t) eff]->load() > 0.5f);

        const int sel = jlimit (0, 2, (int) std::lround (rawMeter[(size_t) ch]->load()));
        for (int k = 0; k < 3; ++k)
            c.meterLeds[(size_t) k].setOn (k == sel);

        float vuDb = 0.0f;
        if (sel == 0)      vuDb = -eng.optoGrDb[ch].load (std::memory_order_relaxed);
        else if (sel == 1) vuDb = -eng.discGrDb[ch].load (std::memory_order_relaxed);
        else               vuDb = powerToDb (eng.outMeanSq[ch].load (std::memory_order_relaxed)) - kVuRefDbfs;

        c.vu.setReading (vuDb, dt);
    }

    monoLed.setOn (! stereo);
    stereoLed.setOn (stereo);

    // Right-hand controls are inactive in Stereo (meter select stays independent).
    if (firstTick || stereo != lastStereo)
    {
        auto& r = chans[1];
        for (Component* comp : std::initializer_list<Component*> {
                 &r.optThr, &r.optGain, &r.ratio, &r.attack, &r.recover, &r.dscThr, &r.dscGain,
                 &r.optIn, &r.dscIn, &r.scFilter, r.xfmr.get() })
        {
            if (comp != nullptr)
            {
                comp->setEnabled (! stereo);
                comp->setAlpha (stereo ? 0.55f : 1.0f);
            }
        }
        lastStereo = stereo;
        firstTick = false;
    }

    // Magic eye follows output peaks: fast attack, slow decay.
    const float peak   = eng.outPeak.exchange (0.0f, std::memory_order_relaxed);
    const float target = jlimit (0.0f, 1.0f, (gainToDb (peak) + 24.0f) / 24.0f);
    eyeLevel = target > eyeLevel ? target : eyeLevel + (target - eyeLevel) * (1.0f - std::exp (-dt / 0.35f));
    eye.setLevel (eyeLevel);
}
