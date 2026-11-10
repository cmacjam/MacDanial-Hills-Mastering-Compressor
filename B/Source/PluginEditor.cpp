#include "PluginEditor.h"

using namespace juce;
using namespace od2;

namespace
{
    constexpr float kVuRefDbfs = -14.0f;   // Output meter: 0 VU = -14 dBFS RMS

    // Engraved line: dark cut with a light lower edge.
    void engraveStroke (Graphics& g, const Path& p, float w = 2.0f)
    {
        g.setColour (colours::engraveHi);
        g.strokePath (p, PathStrokeType (w * 0.7f), AffineTransform::translation (0.8f, 1.0f));
        g.setColour (colours::engrave.withAlpha (0.75f));
        g.strokePath (p, PathStrokeType (w));
    }

    Path polygon (std::initializer_list<Point<float>> pts, float corner)
    {
        Path p;
        bool first = true;
        for (auto pt : pts) { if (first) p.startNewSubPath (pt); else p.lineTo (pt); first = false; }
        p.closeSubPath();
        return p.createPathWithRoundedCorners (corner);
    }
}

//==============================================================================
OD2Editor::OD2Editor (OD2AudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    buildChannel (0);
    buildChannel (1);

    // ---- centre section -------------------------------------------------------
    addToggle (hardwire, ids::hardwireBypass, 800.0f, 540.0f, "HARDWIRE", "BYPASS", "IN", "OUT", true);

    if (auto* sp = proc.apvts.getParameter (ids::stereo))
    {
        stereoSel = std::make_unique<LedSelector> (*sp, LedSelector::Style::miniKnob, colours::ledGreen, false);
        addAndMakeVisible (*stereoSel);
        place (*stereoSel, { 745.0f, 400.0f, 110.0f, 90.0f });   // knob centre (800, 463)
        const float spread = degreesToRadians (60.0f);
        arcTexts.push_back ({ "DUAL MONO", 800.0f, 463.0f, 61.0f, -spread, 11.0f });
        arcTexts.push_back ({ "STEREO",    800.0f, 463.0f, 61.0f,  spread, 11.0f });
    }

    addAndMakeVisible (eye);
    place (eye, { 770.0f, 610.0f, 60.0f, 60.0f });
    arcTexts.push_back ({ "CLASS-A EDITION", 800.0f, 640.0f, 42.0f, 0.0f, 11.0f });

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
    c.optLed.setColour (colours::ledGreen);
    c.dscLed.setColour (colours::ledGreen);
    addAndMakeVisible (c.optLed);
    addAndMakeVisible (c.dscLed);
    place (c.optLed, { mx (240.0f) - 13.0f, 222.0f - 13.0f, 26.0f, 26.0f });
    place (c.dscLed, { mx (684.0f) - 13.0f, 428.0f - 13.0f, 26.0f, 26.0f });

    // ---- transformer select (3-way toggle with blue LEDs)
    if (auto* xp = proc.apvts.getParameter (id (ids::xfmr)))
    {
        c.xfmr = std::make_unique<LedSelector> (*xp, LedSelector::Style::verticalToggle, colours::ledGreen, ch == 1);
        addAndMakeVisible (*c.xfmr);
        const float x0 = ch == 0 ? 660.0f : 865.0f;
        place (*c.xfmr, { x0, 535.0f, 75.0f, 120.0f });
        textSpecs.push_back ({ "NICKEL", x0 + 37.5f, 526.0f, 12.0f, true });
        textSpecs.push_back ({ "STEEL",  x0 + 37.5f, 666.0f, 12.0f, true });
    }

    // ---- meter select (mini knob with LEDs on an arc)
    if (auto* mp = proc.apvts.getParameter (id (ids::meterSel)))
    {
        c.meterSel = std::make_unique<LedSelector> (*mp, LedSelector::Style::miniKnob, colours::ledGreen, false);
        addAndMakeVisible (*c.meterSel);
        const float kx = mx (345.0f);
        place (*c.meterSel, { kx - 55.0f, 50.0f, 110.0f, 90.0f });   // knob centre = (kx, 113)
        const float spread = degreesToRadians (60.0f);
        arcTexts.push_back ({ "OPTICAL",  kx, 113.0f, 61.0f, -spread, 11.0f });
        arcTexts.push_back ({ "DISCRETE", kx, 113.0f, 61.0f,  0.0f,   11.0f });
        arcTexts.push_back ({ "OUTPUT",   kx, 113.0f, 61.0f,  spread, 11.0f });
    }

    // ---- VU meter + O / D / VU indicator LEDs beneath it
    c.vu.setBrand ("macdanial hills");
    addAndMakeVisible (c.vu);
    place (c.vu, { ch == 0 ? 425.0f : 810.0f, 35.0f, 365.0f, 340.0f });

    const float xs[2][3] = { { 545.0f, 620.0f, 695.0f }, { 905.0f, 980.0f, 1055.0f } };
    const char* names[3] = { "O", "D", "VU" };
    for (int k = 0; k < 3; ++k)
    {
        auto& led = c.meterLeds[(size_t) k];
        led.setColour (k == 2 ? colours::ledGreen : colours::ledGreen);
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

    // ---- brushed copper faceplate ----
    {
        ColourGradient cu (colours::copperLight, 0.0f, 0.0f, colours::copperDark, kBaseW, kBaseH, false);
        cu.addColour (0.45, colours::copperMid);
        cu.addColour (0.70, Colour (0xffe39c66));
        g.setGradientFill (cu);
        g.fillRect (panel);

        Random rng (0xC0FFEE);
        for (int i = 0; i < 1600; ++i)
        {
            const float y = rng.nextFloat() * kBaseH, x = rng.nextFloat() * kBaseW;
            const bool light = rng.nextBool();
            g.setColour ((light ? Colours::white : Colour (0xff5a2a10)).withAlpha (0.025f + rng.nextFloat() * 0.035f));
            g.drawLine (x, y, x + 60.0f + rng.nextFloat() * 220.0f, y, 0.7f);
        }
        // soft patina
        g.setGradientFill (ColourGradient (Colour (0x22fff0e0), kBaseW * 0.3f, kBaseH * 0.2f,
                                           Colour (0x00fff0e0), kBaseW * 0.3f + 500.0f, kBaseH * 0.2f, true));
        g.fillRect (panel);
    }

    // ---- engraved section outlines (mirrored) ----
    {
        Path border;
        border.addRoundedRectangle (panel.reduced (8.0f), 14.0f);
        engraveStroke (g, border, 2.2f);
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto mirror = ch == 0 ? AffineTransform() : AffineTransform::scale (-1.0f, 1.0f).translated (kBaseW, 0.0f);

        auto optical  = polygon ({ { 18, 16 }, { 418, 16 }, { 418, 380 }, { 252, 380 }, { 252, 316 }, { 18, 316 } }, 14.0f);
        auto discrete = polygon ({ { 18, 322 }, { 246, 322 }, { 246, 386 }, { 705, 386 }, { 705, 507 },
                                   { 646, 507 }, { 646, 704 }, { 18, 704 } }, 14.0f);
        optical.applyTransform (mirror);
        discrete.applyTransform (mirror);
        engraveStroke (g, optical);
        engraveStroke (g, discrete);
    }
    engraveStroke (g, polygon ({ { 712, 392 }, { 888, 392 }, { 888, 507 }, { 948, 507 }, { 948, 704 },
                                 { 652, 704 }, { 652, 507 }, { 712, 507 } }, 14.0f));

    // ---- brand plaques ----
    auto plaque = [&g] (const String& text, Rectangle<float> r, float size, float kerning)
    {
        Path p;
        p.addRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (colours::copperDark.withAlpha (0.55f));
        g.fillPath (p);
        engraveStroke (g, p, 1.6f);
        g.setColour (colours::engrave);
        g.setFont (PanelLookAndFeel::panelFont (size).withExtraKerningFactor (kerning));
        g.drawText (text, r, Justification::centred, false);
    };
    plaque ("MACDANIAL HILLS  -  MODEL B", { 600.0f, 6.0f, 400.0f, 26.0f }, 18.0f, 0.10f);
    plaque ("mastering compressor", { 680.0f, 680.0f, 240.0f, 24.0f }, 16.0f, 0.02f);

    // ---- knob dial plates: black discs with light numerals, engraved titles ----
    for (const auto& ks : knobSpecs)
    {
        const Point<float> c (ks.cx, ks.cy);
        const int n = ks.labels.size();
        const bool big = ks.r > 60.0f;
        const float plateR = ks.r * (big ? 1.42f : 1.45f);
        const float ringR  = ks.r * 1.08f;
        const float numR   = ks.r * (big ? 1.26f : 1.28f);
        const float numSize = (n > 12) ? (big ? 11.5f : 9.5f) : 11.0f;

        // plate with a bevelled rim
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - plateR + 2.0f, c.y - plateR + 4.0f, plateR * 2.0f, plateR * 2.0f);
        g.setGradientFill (ColourGradient (Colour (0xff2a2522), c.x, c.y - plateR,
                                           Colour (0xff080706), c.x, c.y + plateR, false));
        g.fillEllipse (c.x - plateR, c.y - plateR, plateR * 2.0f, plateR * 2.0f);
        g.setColour (colours::copperLight.withAlpha (0.55f));
        g.drawEllipse (c.x - plateR, c.y - plateR, plateR * 2.0f, plateR * 2.0f, 1.4f);

        Path ring;
        ring.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, kRotStart, kRotEnd, true);
        g.setColour (colours::plateText.withAlpha (0.8f));
        g.strokePath (ring, PathStrokeType (1.1f));

        g.setFont (PanelLookAndFeel::panelFont (numSize, true));
        for (int i = 0; i < n; ++i)
        {
            const float a = kRotStart + (kRotEnd - kRotStart) * (float) i / (float) jmax (1, n - 1);
            g.drawLine (Line<float> (c.getPointOnCircumference (ringR, a), c.getPointOnCircumference (ringR + 4.0f, a)), 1.1f);

            // long words (FLOOD / DUAL) sit outside the plate on the copper
            const bool word = ks.labels[i].length() > 3;
            g.setColour (word ? colours::engrave : colours::plateText);
            const auto lp = c.getPointOnCircumference (word ? plateR + 14.0f : numR, a);
            g.drawText (ks.labels[i], Rectangle<float> (lp.x - 22.0f, lp.y - numSize * 0.6f, 44.0f, numSize * 1.2f),
                        Justification::centred, false);
            g.setColour (colours::plateText.withAlpha (0.8f));
        }

        // arched title plaque
        const auto titleFont = PanelLookAndFeel::panelFont (big ? 15.0f : 13.5f).withExtraKerningFactor (0.06f);
        const float titleR = plateR + 9.0f;
        const float half = (PanelLookAndFeel::textWidth (titleFont, ks.title) * 0.5f + 12.0f) / titleR;
        Path band;
        band.addCentredArc (c.x, c.y, titleR + 5.0f, titleR + 5.0f, 0.0f, -half, half, true);
        g.setColour (colours::copperDark.withAlpha (0.6f));
        g.strokePath (band, PathStrokeType (18.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (colours::engrave.withAlpha (0.6f));
        Path bandEdge;
        bandEdge.addCentredArc (c.x, c.y, titleR + 14.5f, titleR + 14.5f, 0.0f, -half, half, true);
        bandEdge.addCentredArc (c.x, c.y, titleR - 4.5f, titleR - 4.5f, 0.0f, -half, half, true);
        g.strokePath (bandEdge, PathStrokeType (1.2f));

        g.setColour (colours::engrave);
        PanelLookAndFeel::drawTextOnArc (g, ks.title, titleFont, c, titleR, 0.0f);
    }

    // ---- toggle legends ----
    g.setColour (colours::engrave);
    g.setFont (PanelLookAndFeel::panelFont (12.0f, true));
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

    for (const auto& t : textSpecs)
    {
        g.setFont (PanelLookAndFeel::panelFont (t.size, t.bold));
        g.drawText (t.text, Rectangle<float> (t.x - 50.0f, t.y - t.size * 0.65f, 100.0f, t.size * 1.3f),
                    Justification::centred, false);
    }

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
