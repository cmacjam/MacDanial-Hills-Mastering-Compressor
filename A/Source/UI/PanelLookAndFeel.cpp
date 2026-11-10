#include "PanelLookAndFeel.h"

namespace od2
{
using namespace juce;

PanelLookAndFeel::PanelLookAndFeel()
{
    setColour (BubbleComponent::backgroundColourId, Colour (0xf0101010));
    setColour (BubbleComponent::outlineColourId,    Colour (0xff3a3a3a));
    setColour (TooltipWindow::textColourId,         colours::legend);
}

Font PanelLookAndFeel::panelFont (float height, bool bold)
{
    return Font (FontOptions (height, bold ? Font::bold : Font::plain));
}

//==============================================================================
void PanelLookAndFeel::drawBakeliteKnob (Graphics& g, Point<float> c, float R, float angle, bool enabled)
{
    // MacDanial Hills knob: matte knurled skirt, brushed-metal cap, cream pointer.
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - R + R * 0.06f, c.y - R + R * 0.12f, R * 2.0f, R * 2.0f);

    g.setGradientFill (ColourGradient (Colour (0xff36363a), c.x - R * 0.5f, c.y - R * 0.6f,
                                       Colour (0xff070708), c.x + R * 0.6f, c.y + R * 0.8f, true));
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);

    // knurling
    const int ridges = 48;
    for (int k = 0; k < ridges; ++k)
    {
        const float a = (float) k * MathConstants<float>::twoPi / (float) ridges;
        g.setColour ((k % 2 == 0 ? Colours::white : Colours::black).withAlpha (k % 2 == 0 ? 0.07f : 0.35f));
        g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.80f, a), c.getPointOnCircumference (R * 0.985f, a)),
                    jmax (0.6f, R * 0.025f));
    }
    g.setColour (Colours::black);
    g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, jmax (1.0f, R * 0.03f));

    // brushed-metal cap
    const float rc = R * 0.64f;
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - rc * 1.06f, c.y - rc * 1.02f, rc * 2.12f, rc * 2.16f);
    g.setGradientFill (ColourGradient (Colour (0xffe4e3df), c.x - rc * 0.6f, c.y - rc * 0.7f,
                                       Colour (0xff7d7c78), c.x + rc * 0.7f, c.y + rc * 0.8f, false));
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);
    for (int k = 1; k < 9; ++k)   // concentric brushing
    {
        const float rr = rc * (float) k / 9.0f;
        g.setColour ((k % 2 ? Colours::white : Colours::black).withAlpha (0.05f));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, jmax (0.5f, rc * 0.04f));
    }
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.45f), c.x - rc * 0.45f, c.y - rc * 0.55f,
                                       Colours::white.withAlpha (0.0f), c.x, c.y, true));
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f, jmax (0.8f, R * 0.02f));

    // pointer: engraved dot on the cap + cream line on the skirt
    const auto dot = c.getPointOnCircumference (rc * 0.68f, angle);
    g.setColour (Colour (0xff1c1c1c));
    g.fillEllipse (Rectangle<float> (rc * 0.2f, rc * 0.2f).withCentre (dot));
    const auto p1 = c.getPointOnCircumference (rc * 1.08f, angle), p2 = c.getPointOnCircumference (R * 0.97f, angle);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawLine (Line<float> (p1, p2).withShortenedStart (0.0f), jmax (2.0f, R * 0.10f));
    g.setColour (Colour (0xfff1ead8));
    g.drawLine (Line<float> (p1, p2), jmax (1.4f, R * 0.065f));

    if (! enabled)
    {
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    }
}

void PanelLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<int> (x, y, w, h).toFloat();
    const float R = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    if (s.getProperties()["style"].toString() == "cream")
        drawCreamKnob (g, bounds.getCentre(), R, angle);
    else
        drawBakeliteKnob (g, bounds.getCentre(), R, angle, s.isEnabled());
}

void PanelLookAndFeel::drawCreamKnob (Graphics& g, Point<float> c, float R, float angle)
{
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - R + R * 0.08f, c.y - R + R * 0.14f, R * 2.0f, R * 2.0f);

    // knurled skirt
    g.setGradientFill (ColourGradient (Colour (0xffeeeadf), c.x - R * 0.5f, c.y - R * 0.6f,
                                       Colour (0xff8f897c), c.x + R * 0.7f, c.y + R * 0.9f, true));
    g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    g.setColour (Colours::black.withAlpha (0.18f));
    for (int k = 0; k < 36; ++k)
    {
        const float a = (float) k * MathConstants<float>::twoPi / 36.0f;
        g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.82f, a), c.getPointOnCircumference (R * 0.99f, a)), 1.0f);
    }

    // cap
    const float r2 = R * 0.74f;
    g.setGradientFill (ColourGradient (Colour (0xfff7f4ec), c.x - r2 * 0.4f, c.y - r2 * 0.5f,
                                       Colour (0xffb3ad9f), c.x + r2 * 0.7f, c.y + r2 * 0.8f, true));
    g.fillEllipse (c.x - r2, c.y - r2, r2 * 2.0f, r2 * 2.0f);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.0f, r2 * 2.0f, 1.0f);

    // pointer
    g.setColour (Colour (0xff141414));
    g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.25f, angle), c.getPointOnCircumference (R * 0.97f, angle)),
                jmax (1.5f, R * 0.08f));
}

void PanelLookAndFeel::drawPushButton (Graphics& g, Rectangle<float> r, const String& text, bool lit,
                                       Colour led, bool down, bool hover)
{
    r = r.reduced (r.getWidth() * 0.06f, r.getHeight() * 0.12f);
    const float corner = r.getHeight() * 0.32f;

    // recessed bezel
    g.setColour (Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (r.expanded (2.5f).translated (0.0f, 1.0f), corner + 2.0f);
    g.setGradientFill (ColourGradient (Colour (0xff4a4a4e), r.getX(), r.getY() - 2.0f,
                                       Colour (0xff111113), r.getX(), r.getBottom() + 2.0f, false));
    g.fillRoundedRectangle (r.expanded (1.5f), corner + 1.5f);

    // glow behind the cap when lit
    if (lit)
    {
        g.setGradientFill (ColourGradient (led.withAlpha (0.45f), r.getCentreX(), r.getCentreY(),
                                           led.withAlpha (0.0f), r.getRight() + r.getHeight(), r.getCentreY(), true));
        g.fillRoundedRectangle (r.expanded (r.getHeight() * 0.6f), corner * 2.0f);
    }

    // cap
    auto cap = r.translated (0.0f, down ? 1.0f : 0.0f);
    g.setGradientFill (ColourGradient (lit ? Colour (0xff2c2f2d) : Colour (0xff2a2a2d), cap.getX(), cap.getY(),
                                       Colour (0xff0d0d0e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, corner);
    g.setColour (Colours::white.withAlpha (hover ? 0.16f : 0.09f));
    g.drawRoundedRectangle (cap.reduced (0.5f), corner, 1.0f);

    // light strip along the top edge of the cap
    auto strip = cap.reduced (cap.getWidth() * 0.18f, 0.0f).removeFromTop (cap.getHeight() * 0.2f).translated (0.0f, cap.getHeight() * 0.12f);
    g.setColour (lit ? led : Colour (0xff1a1d1b));
    g.fillRoundedRectangle (strip, strip.getHeight() * 0.5f);
    if (lit)
    {
        g.setColour (Colours::white.withAlpha (0.55f));
        g.fillRoundedRectangle (strip.reduced (strip.getWidth() * 0.3f, strip.getHeight() * 0.3f), strip.getHeight() * 0.3f);
    }

    g.setColour (lit ? Colour (0xfff2efe6) : Colour (0xff8a8a8d));
    g.setFont (panelFont (cap.getHeight() * 0.36f, true).withExtraKerningFactor (0.06f));
    g.drawText (text, cap.withTrimmedTop (cap.getHeight() * 0.3f), Justification::centred, false);
}

void PanelLookAndFeel::drawLampButton (Graphics& g, Rectangle<float> r, const String& text, bool lit, bool down)
{
    r = r.reduced (2.0f);
    const float corner = r.getHeight() * 0.12f;

    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (1.5f, 2.5f), corner);
    g.setColour (Colour (0xff0a0a0a));
    g.fillRoundedRectangle (r, corner);

    auto cap = r.reduced (r.getWidth() * 0.07f).translated (0.0f, down ? 1.0f : 0.0f);
    if (lit)
    {
        g.setGradientFill (ColourGradient (Colour (0x66fff1c8), cap.getCentreX(), cap.getCentreY(),
                                           Colour (0x00fff1c8), cap.getRight() + cap.getWidth() * 0.4f, cap.getCentreY(), true));
        g.fillRoundedRectangle (cap.expanded (cap.getWidth() * 0.25f), corner * 2.0f);
    }
    g.setGradientFill (ColourGradient (lit ? Colour (0xfffffbee) : Colour (0xffc9c3b3), cap.getX(), cap.getY(),
                                       lit ? Colour (0xffe9dcb2) : Colour (0xff8e887a), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, corner);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (cap, corner, 1.0f);

    g.setColour (Colour (0xff1b1b1b).withAlpha (lit ? 1.0f : 0.75f));
    g.setFont (panelFont (cap.getHeight() * 0.28f, true));
    g.drawText (text, cap, Justification::centred, false);
}

//==============================================================================
void PanelLookAndFeel::drawToggleSwitch (Graphics& g, Point<float> c, float nutR, Point<float> dir, float len)
{
    // hex nut
    Path hex;
    for (int k = 0; k < 6; ++k)
    {
        const float a = (float) k * MathConstants<float>::pi / 3.0f + MathConstants<float>::pi / 6.0f;
        const Point<float> p (c.x + std::cos (a) * nutR * 1.15f, c.y + std::sin (a) * nutR * 1.15f);
        if (k == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillPath (hex, AffineTransform::translation (nutR * 0.12f, nutR * 0.2f));
    g.setGradientFill (ColourGradient (Colour (0xffe6e6e6), c.x - nutR, c.y - nutR,
                                       Colour (0xff3e3e3e), c.x + nutR, c.y + nutR, false));
    g.fillPath (hex);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.strokePath (hex, PathStrokeType (jmax (0.8f, nutR * 0.06f)));

    // threaded barrel
    const float rb = nutR * 0.62f;
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2a), c.x - rb, c.y - rb,
                                       Colour (0xffbdbdbd), c.x + rb, c.y + rb, false));
    g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);

    const float tipR = nutR * 0.34f;
    auto chromeBall = [&g] (Point<float> p, float r)
    {
        ColourGradient grad (Colours::white, p.x - r * 0.4f, p.y - r * 0.4f,
                             Colour (0xff555555), p.x + r, p.y + r, true);
        g.setGradientFill (grad);
        g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    };

    if (dir.getDistanceFromOrigin() < 0.01f)
    {   // centre position: bat pointing straight at the viewer
        chromeBall (c, nutR * 0.48f);
        return;
    }

    const Point<float> perp (-dir.y, dir.x);
    const Point<float> tip = c + dir * len;
    const float bw = nutR * 0.36f, tw = nutR * 0.24f;

    Path bat;
    bat.startNewSubPath (c + perp * bw);
    bat.lineTo (tip + perp * tw);
    bat.lineTo (tip - perp * tw);
    bat.lineTo (c - perp * bw);
    bat.closeSubPath();

    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillPath (bat, AffineTransform::translation (nutR * 0.18f, nutR * 0.35f));

    ColourGradient chrome (Colour (0xff5a5a5a), (c + perp * bw).x, (c + perp * bw).y,
                           Colour (0xff4a4a4a), (c - perp * bw).x, (c - perp * bw).y, false);
    chrome.addColour (0.4, Colour (0xfff8f8f8));
    chrome.addColour (0.6, Colour (0xffb0b0b0));
    g.setGradientFill (chrome);
    g.fillPath (bat);

    chromeBall (tip, tw * 1.25f);
}

void PanelLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool isDown)
{
    if (b.getProperties()["style"].toString() == "lamp")
    {
        drawLampButton (g, b.getLocalBounds().toFloat(), b.getButtonText(), b.getToggleState(), isDown);
        return;
    }

    // Lit push-button: glows in the unit's LED colour when the function is engaged.
    const auto& props = b.getProperties();
    const bool invert = (bool) props.getWithDefault ("invertLit", false);
    const bool lit    = b.getToggleState() != invert;
    const String text = lit ? props.getWithDefault ("onText", "IN").toString()
                            : props.getWithDefault ("offText", "OUT").toString();
    const Colour led ((uint32) (int) props.getWithDefault ("led", (int) colours::ledGreen.getARGB()));
    drawPushButton (g, b.getLocalBounds().toFloat(), text, lit, led, isDown, b.isMouseOver());
}

//==============================================================================
void PanelLookAndFeel::drawLed (Graphics& g, Point<float> c, float r, Colour col, bool on)
{
    if (on)
    {
        const float halo = r * 2.8f;
        g.setGradientFill (ColourGradient (col.withAlpha (0.55f), c.x, c.y,
                                           col.withAlpha (0.0f),  c.x + halo, c.y, true));
        g.fillEllipse (c.x - halo, c.y - halo, halo * 2.0f, halo * 2.0f);
    }

    // bezel
    g.setColour (Colour (0xff050505));
    g.fillEllipse (c.x - r * 1.18f, c.y - r * 1.18f, r * 2.36f, r * 2.36f);

    ColourGradient lens (on ? col.brighter (0.9f) : col.darker (3.0f).interpolatedWith (Colours::black, 0.6f),
                         c.x - r * 0.3f, c.y - r * 0.3f,
                         on ? col.darker (0.2f) : Colour (0xff080808),
                         c.x + r, c.y + r, true);
    g.setGradientFill (lens);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.setColour (Colours::white.withAlpha (on ? 0.85f : 0.35f));
    g.fillEllipse (c.x - r * 0.55f, c.y - r * 0.6f, r * 0.45f, r * 0.35f);
}

void PanelLookAndFeel::drawScrew (Graphics& g, Point<float> c, float r, float slotAngle)
{
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - r * 1.15f, c.y - r * 1.15f + r * 0.15f, r * 2.3f, r * 2.3f);
    g.setGradientFill (ColourGradient (Colour (0xff9a9a9a), c.x - r * 0.4f, c.y - r * 0.5f,
                                       Colour (0xff2a2a2a), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    const auto d = Point<float> (std::cos (slotAngle), std::sin (slotAngle)) * (r * 0.8f);
    g.setColour (Colour (0xff111111));
    g.drawLine (Line<float> (c - d, c + d), r * 0.28f);
}

float PanelLookAndFeel::textWidth (const Font& f, const String& t)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, t, 0.0f, 0.0f);
    return ga.getNumGlyphs() > 0 ? ga.getGlyph (ga.getNumGlyphs() - 1).getRight() - ga.getGlyph (0).getLeft() : 0.0f;
}

void PanelLookAndFeel::drawTextOnArc (Graphics& g, const String& text, const Font& font,
                                      Point<float> centre, float radius, float centreAngle)
{
    GlyphArrangement ga;
    ga.addLineOfText (font, text, 0.0f, 0.0f);
    const int n = ga.getNumGlyphs();
    if (n == 0)
        return;

    const float left  = ga.getGlyph (0).getLeft();
    const float right = ga.getGlyph (n - 1).getRight();
    const float mid   = 0.5f * (left + right);

    for (int i = 0; i < n; ++i)
    {
        const auto& glyph = ga.getGlyph (i);
        const float gx = 0.5f * (glyph.getLeft() + glyph.getRight());
        const float a  = centreAngle + (gx - mid) / radius;
        const auto  p  = centre.getPointOnCircumference (radius, a);

        glyph.draw (g, AffineTransform::translation (-gx, 0.0f).rotated (a).translated (p.x, p.y));
    }
}

} // namespace od2
