#include "PanelComponents.h"

namespace od2
{
using namespace juce;

//==============================================================================
void Led::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    PanelLookAndFeel::drawLed (g, b.getCentre(), jmin (b.getWidth(), b.getHeight()) * 0.22f, colour, on);
}

//==============================================================================
LedSelector::LedSelector (RangedAudioParameter& p, Style s, Colour led, bool mirror)
    : style (s), ledColour (led), mirrored (mirror),
      numItems (jmax (2, (int) std::lround (p.getNormalisableRange().end - p.getNormalisableRange().start) + 1)),
      attachment (p, [this] (float v)
                  {
                      index = jlimit (0, numItems - 1, (int) std::lround (v));
                      repaint();
                  })
{
    attachment.sendInitialUpdate();
}

void LedSelector::select (int newIndex)
{
    newIndex = jlimit (0, numItems - 1, newIndex);
    if (newIndex != index)
        attachment.setValueAsCompleteGesture ((float) newIndex);
}

Point<float> LedSelector::ledCentre (int i) const
{
    const float w = (float) getWidth(), h = (float) getHeight();

    if (style == Style::verticalToggle)
        return { mirrored ? w * 0.74f : w * 0.26f, h * (0.17f + 0.33f * (float) i) };

    const Point<float> knob (w * 0.5f, h * 0.70f);
    const float spread = degreesToRadians (60.0f);
    const float a = -spread + 2.0f * spread * (float) i / (float) (numItems - 1);
    return knob.getPointOnCircumference (h * 0.48f, a);
}

float LedSelector::knobAngleFor (int i) const
{
    const float spread = degreesToRadians (60.0f);
    return -spread + 2.0f * spread * (float) i / (float) (numItems - 1);
}

void LedSelector::paint (Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const float ledR = h * 0.065f;

    if (style == Style::verticalToggle)
    {
        // Three stacked lit buttons: Nickel / Iron / Steel
        const char* letters[] = { "N", "I", "S" };
        const float bh = h / (float) numItems;
        for (int i = 0; i < numItems; ++i)
            PanelLookAndFeel::drawPushButton (g, Rectangle<float> (w * 0.12f, bh * (float) i, w * 0.76f, bh),
                                              letters[jmin (i, 2)], i == index, ledColour, false, false);
    }
    else
    {
        for (int i = 0; i < numItems; ++i)
            PanelLookAndFeel::drawLed (g, ledCentre (i), ledR, ledColour, i == index);
    }

    if (style == Style::verticalToggle)
    {
    }
    else
    {
        PanelLookAndFeel::drawBakeliteKnob (g, { w * 0.5f, h * 0.70f }, h * 0.22f, knobAngleFor (index), isEnabled());
    }

}

void LedSelector::mouseDown (const MouseEvent& e)
{
    if (style == Style::verticalToggle)
    {
        select ((int) (e.position.y / (float) getHeight() * (float) numItems));
        return;
    }

    // Click an LED to select it directly; otherwise step through positions.
    for (int i = 0; i < numItems; ++i)
        if (e.position.getDistanceFrom (ledCentre (i)) < (float) getHeight() * 0.14f)
        {
            select (i);
            return;
        }

    select ((index + 1) % numItems);
}

void LedSelector::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    const float d = wheel.deltaY != 0.0f ? wheel.deltaY : wheel.deltaX;
    if (d == 0.0f) return;
    const int step = (d > 0.0f) == (style == Style::verticalToggle) ? -1 : 1;
    select (index + step);
}

//==============================================================================
VuMeter::VuMeter() { setInterceptsMouseClicks (false, false); }

float VuMeter::vuToPos (float db)
{
    // Needle deflection is proportional to voltage; +3 VU is full scale.
    return jlimit (-0.03f, 1.05f, std::pow (10.0f, db / 20.0f) / 1.41254f);
}

float VuMeter::posToAngle (float p)
{
    return degreesToRadians (-48.0f + 96.0f * p);
}

void VuMeter::setReading (float vuDb, float dt)
{
    // VU ballistics: ~300 ms to reach 99 %.
    const float target = vuToPos (vuDb);
    const float a = 1.0f - std::exp (-dt / 0.065f);
    const float old = pos;
    pos += a * (target - pos);
    if (std::abs (pos - old) > 0.0004f)
        repaint();
}

VuMeter::Geo VuMeter::geometry() const
{
    const auto b = getLocalBounds().toFloat();
    Geo geo;
    geo.face  = { b.getX() + b.getWidth() * 0.075f, b.getY() + b.getHeight() * 0.075f,
                  b.getWidth() * 0.85f, b.getHeight() * 0.50f };
    const float fh = geo.face.getHeight();
    geo.pivot = { geo.face.getCentreX(), geo.face.getY() + fh * 1.05f };
    geo.R     = fh * 0.80f;
    return geo;
}

void VuMeter::renderCaches()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    constexpr float k = 2.0f;   // render at 2x for crisp retina scaling

    staticLayer  = Image (Image::ARGB, (int) (w * k), (int) (h * k), true);
    overlayLayer = Image (Image::ARGB, (int) (w * k), (int) (h * k), true);
    {
        Graphics g (staticLayer);
        g.addTransform (AffineTransform::scale (k));
        drawStatic (g);
    }
    {
        Graphics g (overlayLayer);
        g.addTransform (AffineTransform::scale (k));
        drawOverlay (g);
    }
    repaint();
}

void VuMeter::drawStatic (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto geo = geometry();
    const auto& face = geo.face;
    const float fh = face.getHeight(), fw = face.getWidth();

    // ---- bakelite bezel ----
    const float corner = b.getWidth() * 0.07f;
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (b.translated (3.0f, 5.0f).reduced (2.0f), corner);
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2c), b.getX(), b.getY(),
                                       Colour (0xff050505), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (2.0f), corner);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (b.reduced (3.0f), corner, 1.2f);

    // lower skirt grooves
    for (float gx : { 0.20f, 0.80f })
        for (int k = -1; k <= 1; ++k)
        {
            const float x = b.getX() + b.getWidth() * gx + (float) k * b.getWidth() * 0.012f;
            const float y1 = face.getBottom() + b.getHeight() * 0.08f, y2 = b.getBottom() - b.getHeight() * 0.06f;
            g.setColour (Colours::black.withAlpha (0.8f));
            g.drawLine (x, y1, x, y2, 1.4f);
            g.setColour (Colours::white.withAlpha (0.07f));
            g.drawLine (x + 1.2f, y1, x + 1.2f, y2, 1.0f);
        }

    // window frame
    g.setColour (Colour (0xff000000));
    g.fillRoundedRectangle (face.expanded (4.0f), fw * 0.05f);

    // ---- illuminated face ----
    {
        Graphics::ScopedSaveState ss (g);
        Path facePath;
        facePath.addRoundedRectangle (face, fw * 0.04f);
        g.reduceClipRegion (facePath);

        g.setGradientFill (ColourGradient (faceCentre, face.getCentreX(), face.getY() + fh * 0.45f,
                                           faceEdge, face.getX(), face.getBottom(), true));
        g.fillRect (face);

        // lamp glow
        g.setGradientFill (ColourGradient (Colour (0x66fff6d0), face.getCentreX(), face.getY() + fh * 0.55f,
                                           Colour (0x00fff6d0), face.getCentreX() + fw * 0.55f, face.getY() + fh * 0.55f, true));
        g.fillRect (face);

        const auto& c = geo.pivot;
        const float R = geo.R;
        const auto ink = Colour (0xff1b1712);

        // main arc
        Path arc;
        arc.addCentredArc (c.x, c.y, R, R, 0.0f, posToAngle (vuToPos (-20.0f)), posToAngle (vuToPos (0.0f)), true);
        g.setColour (ink);
        g.strokePath (arc, PathStrokeType (fh * 0.008f));

        // red zone 0..+3
        Path red;
        red.addCentredArc (c.x, c.y, R - fh * 0.018f, R - fh * 0.018f, 0.0f,
                           posToAngle (vuToPos (0.0f)), posToAngle (vuToPos (3.0f)), true);
        g.setColour (colours::vuRed);
        g.strokePath (red, PathStrokeType (fh * 0.04f));

        // dB ticks and legends
        const float dbs[]    = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
        const char* labels[] = { "-20", "-10", "-7", "-5", "-3", "-2", "-1", "0", "+1", "+2", "+3" };
        g.setFont (PanelLookAndFeel::panelFont (fh * 0.075f, false));
        for (int i = 0; i < 11; ++i)
        {
            const float a = posToAngle (vuToPos (dbs[i]));
            const bool  hot = dbs[i] > 0.0f;
            g.setColour (hot ? colours::vuRed : ink);
            g.drawLine (Line<float> (c.getPointOnCircumference (R, a), c.getPointOnCircumference (R * 1.06f, a)),
                        fh * 0.012f);
            const auto lp = c.getPointOnCircumference (R * 1.13f, a);
            g.drawText (labels[i], Rectangle<float> (lp.x - fh * 0.15f, lp.y - fh * 0.045f, fh * 0.3f, fh * 0.09f),
                        Justification::centred, false);
        }

        // minor ticks between -20 and -10, and the percent scale
        g.setColour (ink);
        for (float d = -19.0f; d < -10.0f; d += 1.0f)
        {
            const float a = posToAngle (vuToPos (d));
            g.drawLine (Line<float> (c.getPointOnCircumference (R, a), c.getPointOnCircumference (R * 1.03f, a)), fh * 0.006f);
        }

        g.setFont (PanelLookAndFeel::panelFont (fh * 0.05f, false));
        for (int pct = 0; pct <= 100; pct += 20)
        {
            const float a = posToAngle ((float) pct / 100.0f / 1.41254f);
            g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.94f, a), c.getPointOnCircumference (R * 0.985f, a)),
                        fh * 0.007f);
            if (pct > 0)
            {
                const auto lp = c.getPointOnCircumference (R * 0.87f, a);
                g.drawText (String (pct), Rectangle<float> (lp.x - fh * 0.1f, lp.y - fh * 0.03f, fh * 0.2f, fh * 0.06f),
                            Justification::centred, false);
            }
        }

        // "VU" legends
        g.setFont (PanelLookAndFeel::panelFont (fh * 0.075f, false));
        g.setColour (ink);
        g.drawText ("VU", Rectangle<float> (face.getX() + fw * 0.03f, face.getY() + fh * 0.60f, fw * 0.12f, fh * 0.1f),
                    Justification::centred, false);
        g.setColour (colours::vuRed);
        g.drawText ("VU", Rectangle<float> (face.getRight() - fw * 0.15f, face.getY() + fh * 0.60f, fw * 0.12f, fh * 0.1f),
                    Justification::centred, false);

        // brand
        g.setColour (ink.withAlpha (0.85f));
        g.setFont (PanelLookAndFeel::panelFont (fh * 0.055f, true));
        g.drawText (brand, Rectangle<float> (face.getX(), face.getY() + fh * 0.03f, fw, fh * 0.07f),
                    Justification::centred, false);
    }
}

void VuMeter::drawOverlay (Graphics& g)
{
    const auto geo = geometry();
    const auto& face = geo.face;
    const float fh = face.getHeight(), fw = face.getWidth();

    Graphics::ScopedSaveState ss (g);
    Path facePath;
    facePath.addRoundedRectangle (face, fw * 0.04f);
    g.reduceClipRegion (facePath);

    // black needle-pivot dome
    const Rectangle<float> dome (face.getCentreX() - fw * 0.23f, face.getBottom() - fh * 0.27f, fw * 0.46f, fh * 0.54f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (dome.expanded (fw * 0.01f).translated (0.0f, -fh * 0.01f));
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), dome.getCentreX() - fw * 0.06f, dome.getY(),
                                       Colour (0xff050505), dome.getCentreX(), dome.getCentreY(), false));
    g.fillEllipse (dome);

    // glass reflection
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), face.getX(), face.getY(),
                                       Colours::white.withAlpha (0.0f),  face.getX() + fw * 0.45f, face.getY() + fh * 0.6f, false));
    g.fillRect (face);
}

void VuMeter::paint (Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    g.drawImage (staticLayer, area);

    const auto geo = geometry();
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (geo.face.toNearestInt());

        const float a = posToAngle (pos);
        const auto tip = geo.pivot.getPointOnCircumference (geo.R * 1.04f, a);
        const float th = jmax (1.0f, geo.face.getHeight() * 0.011f);

        g.setColour (Colours::black.withAlpha (0.22f));
        g.drawLine (Line<float> (geo.pivot.translated (3.0f, 3.0f), tip.translated (3.0f, 3.0f)), th);
        g.setColour (Colour (0xff111111));
        g.drawLine (Line<float> (geo.pivot, tip), th);
    }

    g.drawImage (overlayLayer, area);
}

//==============================================================================
void MagicEye::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = b.getCentre();
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;

    // green glass rim
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - r + 2.0f, c.y - r + 4.0f, r * 2.0f, r * 2.0f);
    g.setGradientFill (ColourGradient (Colour (0xff9fd8bd), c.x - r * 0.5f, c.y - r * 0.6f,
                                       Colour (0xff1d3a2c), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    // phosphor target
    const float rp = r * 0.70f;
    g.setColour (Colour (0xff04140c));
    g.fillEllipse (c.x - rp, c.y - rp, rp * 2.0f, rp * 2.0f);

    const auto glow = Colour (0xff35ff96).withAlpha (0.45f + 0.5f * level);
    Path fan;
    const float shadow = degreesToRadians (110.0f * (1.0f - level) + 6.0f);   // closes on peaks
    fan.addPieSegment (c.x - rp, c.y - rp, rp * 2.0f, rp * 2.0f,
                       shadow * 0.5f, MathConstants<float>::twoPi - shadow * 0.5f, 0.36f);
    g.setGradientFill (ColourGradient (glow.brighter (0.4f), c.x, c.y, glow.darker (0.6f), c.x + rp, c.y, true));
    g.fillPath (fan);

    // centre cap
    const float rc = rp * 0.34f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x - rc * 0.4f, c.y - rc * 0.4f,
                                       Colour (0xff000000), c.x + rc, c.y + rc, true));
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);

    g.setColour (Colours::white.withAlpha (0.18f));
    g.fillEllipse (c.x - r * 0.6f, c.y - r * 0.75f, r * 0.7f, r * 0.35f);
}

} // namespace od2
