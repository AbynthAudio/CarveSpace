#include "CarveSpaceLookAndFeel.h"

#include <cmath>

#include "BinaryData.h"
#include "ParameterKnob.h"

namespace ts
{
namespace
{
    constexpr float kPi = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
    constexpr float kStartAngle = -2.426f; // -139 degrees, matches the reference knobs
    constexpr float kEndAngle   =  2.426f; // +139 degrees

    juce::Colour argb (juce::uint32 value) { return juce::Colour (value); }

    Palette makeLightPalette()
    {
        Palette p;

        p.windowBg        = argb (0xff2a2a2a);
        p.plateHi         = argb (0xfff7f3ed);
        p.plateMid        = argb (0xffded9d1);
        p.plateLo         = argb (0xffb2babd);
        p.plateWarmTint   = argb (0x18ffd9a8);
        p.plateCoolTint   = argb (0x1a9fc6cf);
        p.plateSheen      = argb (0x2cffffff);
        p.plateBevelHi    = argb (0xb8ffffff);
        p.plateBevelLo    = argb (0x90000000);
        p.ink             = argb (0xff2c2c2c);
        p.inkSoft         = argb (0xff57534d);

        p.displayHi       = argb (0xff2b2b2b);
        p.displayLo       = argb (0xff111111);
        p.gridLine        = argb (0xff393939);
        p.gridFaint       = argb (0xff2c2c2c);
        p.panelOutline    = argb (0xeb000000);

        p.sidechainBlue   = argb (0xff5f97ee);
        p.reductionWhite  = argb (0xffd9d6d1);
        p.accent          = argb (0xff74b1ff);
        p.accentText      = argb (0xff8abcee);

        p.knobBodyHi      = argb (0xff4c4a45);
        p.knobBodyMid     = argb (0xff242422);
        p.knobBodyLo      = argb (0xff050505);
        p.knobRimHi       = argb (0xffc4c1b8);
        p.knobRimLo       = argb (0xff000000);
        p.knobEdgeDark    = argb (0xff000000);

        p.bezelHi         = argb (0xffd2cec4);
        p.bezelUpper      = argb (0xff8f8b82);
        p.bezelMid        = argb (0xff4e483f);
        p.bezelLow        = argb (0xff282420);
        p.bezelDeep       = argb (0xff121110);
        p.knobWarm        = argb (0xff8a6428);
        p.knobTeal        = argb (0xff517c8c);
        p.knobSpecular    = argb (0x88ffffff);

        p.flatTop         = argb (0xff2a2927);
        p.flatLo          = argb (0xff060606);
        p.chamferHi       = argb (0xe8f4f0e5);
        p.capHi           = argb (0xfffcfcf9);
        p.capHiMid        = argb (0xffeaeceb);
        p.capMid          = argb (0xffd3d6d6);
        p.capLo           = argb (0xff82878a);
        p.capSpec         = argb (0x90ffffff);
        p.capShade        = argb (0x50000000);
        p.indicatorHi     = argb (0xffe9e6df);
        p.indicatorLo     = argb (0xff8a8984);

        p.collarHi        = argb (0xfffefdf9);
        p.collarMid       = argb (0xffe2ded5);
        p.collarLo        = argb (0xff8d8b84);
        p.collarRimHi     = argb (0xffffffff);
        p.collarRimLo     = argb (0x70000000);
        p.dishOffHi       = argb (0xff16130f);
        p.dishOffMid      = argb (0xff332e27);
        p.dishOffLo       = argb (0xff756d60);
        p.dishOnHi        = argb (0xff081626);
        p.dishOnMid       = argb (0xff2a6db8);
        p.dishOnLo        = argb (0xff1e4d7e);
        p.freezeKnobHi    = argb (0xff5a5a58);
        p.freezeKnobLo    = argb (0xff0d0d0d);

        p.badgeHi         = argb (0xff4a4a48);
        p.badgeLo         = argb (0xff2a2a29);
        p.badgeInk        = argb (0xffe8e6e0);
        p.wordmarkInk     = argb (0xff545250);
        p.shadow          = argb (0xff000000);

        p.plateCornerExtra = 0.0f;
        p.displayCornerExtra = 0.0f;
        return p;
    }

    Palette makeDarkPalette()
    {
        Palette p;

        p.windowBg        = argb (0xff0c0c0d);
        p.plateHi         = argb (0xff585a5d);
        p.plateMid        = argb (0xff3b3b3e);
        p.plateLo         = argb (0xff1d1d20);
        p.plateWarmTint   = argb (0x16ffb066);
        p.plateCoolTint   = argb (0x1690e0ff);
        p.plateSheen      = argb (0x22ffffff);
        p.plateBevelHi    = argb (0x8affffff);
        p.plateBevelLo    = argb (0xa0000000);
        p.ink             = argb (0xffeae7e0);
        p.inkSoft         = argb (0xffb6b2a9);

        p.displayHi       = argb (0xff141416);
        p.displayLo       = argb (0xff050506);
        p.gridLine        = argb (0xff323236);
        p.gridFaint       = argb (0xff242428);
        p.panelOutline    = argb (0xeb000000);

        p.sidechainBlue   = argb (0xff6ba6ff);
        p.reductionWhite  = argb (0xfff0ece5);
        p.accent          = argb (0xff74b1ff);
        p.accentText      = argb (0xff8abcee);

        p.knobBodyHi      = argb (0xff3c3b38);
        p.knobBodyMid     = argb (0xff1a1918);
        p.knobBodyLo      = argb (0xff030303);
        p.knobRimHi       = argb (0xffaaa79f);
        p.knobRimLo       = argb (0xff000000);
        p.knobEdgeDark    = argb (0xff000000);

        p.bezelHi         = argb (0xffb2aea4);
        p.bezelUpper      = argb (0xff726d64);
        p.bezelMid        = argb (0xff3c3730);
        p.bezelLow        = argb (0xff1c1916);
        p.bezelDeep       = argb (0xff0a0908);
        p.knobWarm        = argb (0xff7d5a24);
        p.knobTeal        = argb (0xff456b78);
        p.knobSpecular    = argb (0x66ffffff);

        p.flatTop         = argb (0xff232221);
        p.flatLo          = argb (0xff040404);
        p.chamferHi       = argb (0xb8d8d4c8);
        p.capHi           = argb (0xfff0f2f1);
        p.capHiMid        = argb (0xffe6e9e9);
        p.capMid          = argb (0xffced2d2);
        p.capLo           = argb (0xff8a8f91);
        p.capSpec         = argb (0x70ffffff);
        p.capShade        = argb (0x60000000);
        p.indicatorHi     = argb (0xffdcd9d2);
        p.indicatorLo     = argb (0xff767570);

        p.collarHi        = argb (0xffe8e5dd);
        p.collarMid       = argb (0xffc6c2b8);
        p.collarLo        = argb (0xff6e6b65);
        p.collarRimHi     = argb (0xccffffff);
        p.collarRimLo     = argb (0x66000000);
        p.dishOffHi       = argb (0xff0c0b09);
        p.dishOffMid      = argb (0xff262420);
        p.dishOffLo       = argb (0xff5a544a);
        p.dishOnHi        = argb (0xff071322);
        p.dishOnMid       = argb (0xff2867ad);
        p.dishOnLo        = argb (0xff17406a);
        p.freezeKnobHi    = argb (0xff4a4a48);
        p.freezeKnobLo    = argb (0xff0a0a0a);

        p.badgeHi         = argb (0xffd8d4cb);
        p.badgeLo         = argb (0xffaaa69d);
        p.badgeInk        = argb (0xff2b2a28);
        p.wordmarkInk     = argb (0xffb6b2a9);
        p.shadow          = argb (0xff000000);

        p.plateCornerExtra = 0.0f;
        p.displayCornerExtra = 0.0f;
        return p;
    }

    Palette makePinkPalette()
    {
        Palette p;

        p.windowBg        = argb (0xfff8bbd0);
        p.plateHi         = argb (0xfffdf4f7);
        p.plateMid        = argb (0xfffce4ec);
        p.plateLo         = argb (0xfff4b6cb);
        p.plateWarmTint   = argb (0x22ffb3c8);
        p.plateCoolTint   = argb (0x16e1bee7);
        p.plateSheen      = argb (0x3affffff);
        p.plateBevelHi    = argb (0xccffffff);
        p.plateBevelLo    = argb (0x704a2030);
        p.ink             = argb (0xff5d4037);
        p.inkSoft         = argb (0xff8d6e63);

        p.displayHi       = argb (0xff3a2a32);
        p.displayLo       = argb (0xff1a1218);
        p.gridLine        = argb (0xff4c3742);
        p.gridFaint       = argb (0xff382630);
        p.panelOutline    = argb (0xeb2a1018);

        p.sidechainBlue   = argb (0xfff48fb1);
        p.reductionWhite  = argb (0xfffff2f7);
        p.accent          = argb (0xffec407a);
        p.accentText      = argb (0xfff8bbd0);

        p.knobBodyHi      = argb (0xff6b4a55);
        p.knobBodyMid     = argb (0xff3a2730);
        p.knobBodyLo      = argb (0xff1a1013);
        p.knobRimHi       = argb (0xffe6c3ce);
        p.knobRimLo       = argb (0xff2a1219);
        p.knobEdgeDark    = argb (0xff2a1219);

        p.bezelHi         = argb (0xfff0d3dc);
        p.bezelUpper      = argb (0xffb98a99);
        p.bezelMid        = argb (0xff7a5260);
        p.bezelLow        = argb (0xff4a2e38);
        p.bezelDeep       = argb (0xff2a1820);
        p.knobWarm        = argb (0xffb0627a);
        p.knobTeal        = argb (0xff7a8fa8);
        p.knobSpecular    = argb (0x88ffffff);

        p.flatTop         = argb (0xff4a3038);
        p.flatLo          = argb (0xff1e1216);
        p.chamferHi       = argb (0xe8fbebf0);
        p.capHi           = argb (0xfffff7fa);
        p.capHiMid        = argb (0xfff6e3ea);
        p.capMid          = argb (0xffe0c3ce);
        p.capLo           = argb (0xff9c7a88);
        p.capSpec         = argb (0x90ffffff);
        p.capShade        = argb (0x45000000);
        p.indicatorHi     = argb (0xfffbeef3);
        p.indicatorLo     = argb (0xffa9828f);

        p.collarHi        = argb (0xfffff7fa);
        p.collarMid       = argb (0xfff3dbe4);
        p.collarLo        = argb (0xffbd97a4);
        p.collarRimHi     = argb (0xffffffff);
        p.collarRimLo     = argb (0x6600000);
        p.dishOffHi       = argb (0xff241820);
        p.dishOffMid      = argb (0xff4a323d);
        p.dishOffLo       = argb (0xff8a6a76);
        p.dishOnHi        = argb (0xff2a0d1a);
        p.dishOnMid       = argb (0xffc2185b);
        p.dishOnLo        = argb (0xffec407a);
        p.freezeKnobHi    = argb (0xff7a5560);
        p.freezeKnobLo    = argb (0xff2a1a20);

        p.badgeHi         = argb (0xfff8bbd0);
        p.badgeLo         = argb (0xffec407a);
        p.badgeInk        = argb (0xfffff5f8);
        p.wordmarkInk     = argb (0xff8d6e63);
        p.shadow          = argb (0xff4a2030);

        p.plateCornerExtra = 2.0f;
        p.displayCornerExtra = 1.0f;
        return p;
    }

    const Palette& lightPalette()
    {
        static const Palette palette = makeLightPalette();
        return palette;
    }

    const Palette& darkPalette()
    {
        static const Palette palette = makeDarkPalette();
        return palette;
    }

    const Palette& pinkPalette()
    {
        static const Palette palette = makePinkPalette();
        return palette;
    }

    ThemeId activeTheme = ThemeId::dark;

    std::vector<float> glyphInkWidths (const juce::Font& font, const juce::String& text)
    {
        std::vector<float> widths;
        widths.reserve ((size_t) text.length());

        for (int i = 0; i < text.length(); ++i)
        {
            const auto character = juce::String::charToString (text[i]);

            if (character == " ")
            {
                widths.push_back (font.getHeight() * 0.30f);
                continue;
            }

            juce::GlyphArrangement arrangement;
            arrangement.addLineOfText (font, character, 0.0f, 0.0f);
            widths.push_back (arrangement.getBoundingBox (0, -1, true).getWidth());
        }

        return widths;
    }
} // namespace

//==============================================================================
namespace theme
{
    ThemeId currentThemeId() noexcept { return activeTheme; }

    const Palette& palette() noexcept
    {
        switch (activeTheme)
        {
            case ThemeId::light: return lightPalette();
            case ThemeId::pink:  return pinkPalette();
            case ThemeId::dark:
            default:             return darkPalette();
        }
    }

    void setTheme (ThemeId newTheme) noexcept { activeTheme = newTheme; }

    juce::String toString (ThemeId id)
    {
        switch (id)
        {
            case ThemeId::light: return "light";
            case ThemeId::pink:  return "pink";
            case ThemeId::dark:
            default:             return "dark";
        }
    }

    ThemeId fromString (const juce::String& name) noexcept
    {
        const auto lower = name.trim().toLowerCase();

        if (lower == "light") return ThemeId::light;
        if (lower == "pink")  return ThemeId::pink;
        return ThemeId::dark;
    }
} // namespace theme

//==============================================================================
juce::Font ui::boldFont (float pixelHeight)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), pixelHeight, juce::Font::bold);
}

juce::Font ui::regularFont (float pixelHeight)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), pixelHeight, juce::Font::plain);
}

void ui::drawFittedText (juce::Graphics& g,
                         const juce::String& text,
                         juce::Rectangle<float> target,
                         juce::Colour colour,
                         bool bold)
{
    if (text.isEmpty() || target.isEmpty())
        return;

    juce::GlyphArrangement arrangement;
    arrangement.addLineOfText (bold ? boldFont (200.0f) : regularFont (200.0f), text, 0.0f, 0.0f);

    if (arrangement.getNumGlyphs() == 0)
        return;

    const auto bounds = arrangement.getBoundingBox (0, -1, true);

    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
        return;

    const auto transform = juce::AffineTransform::translation (-bounds.getX(), -bounds.getY())
                               .scaled (target.getWidth() / bounds.getWidth(),
                                        target.getHeight() / bounds.getHeight())
                               .translated (target.getX(), target.getY());

    g.setColour (colour);
    arrangement.draw (g, transform);
}

void ui::drawTrackedText (juce::Graphics& g,
                          const juce::String& text,
                          juce::Rectangle<float> target,
                          juce::Colour colour,
                          bool bold)
{
    if (text.isEmpty() || target.isEmpty())
        return;

    const auto baseFont = bold ? boldFont (100.0f) : regularFont (100.0f);

    juce::GlyphArrangement measure;
    measure.addLineOfText (baseFont, text, 0.0f, 0.0f);
    const auto measured = measure.getBoundingBox (0, -1, true);

    if (measured.getHeight() <= 0.0f)
        return;

    const auto font = baseFont.withHeight (baseFont.getHeight() * target.getHeight() / measured.getHeight());
    const auto widths = glyphInkWidths (font, text);

    float inkTotal = 0.0f;
    for (auto w : widths)
        inkTotal += w;

    const int gaps = juce::jmax (1, (int) widths.size() - 1);
    const float tracking = (target.getWidth() - inkTotal) / (float) gaps;

    float cursor = target.getX();
    g.setColour (colour);

    for (size_t i = 0; i < widths.size(); ++i)
    {
        const auto character = juce::String::charToString (text[(int) i]);

        if (character != " ")
        {
            juce::GlyphArrangement arrangement;
            arrangement.addLineOfText (font, character, 0.0f, 0.0f);
            const auto b = arrangement.getBoundingBox (0, -1, true);

            if (! b.isEmpty())
                arrangement.draw (g, juce::AffineTransform::translation (-b.getX(), -b.getY())
                                         .translated (cursor, target.getY()));
        }

        cursor += widths[i] + tracking;
    }
}

void ui::drawSoftShadow (juce::Graphics& g,
                         juce::Rectangle<float> area,
                         float cornerRadius,
                         float spread,
                         float alpha,
                         juce::Point<float> offset)
{
    const auto shadowColour = theme::palette().shadow;
    constexpr int steps = 9;

    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float grow = spread * t;
        const float layerAlpha = alpha * (1.0f - t) * (1.0f - t) * 0.55f;

        g.setColour (shadowColour.withAlpha (juce::jlimit (0.0f, 1.0f, layerAlpha)));
        g.fillRoundedRectangle (area.expanded (grow).translated (offset.x, offset.y), cornerRadius + grow);
    }

    g.setColour (shadowColour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha * 0.30f)));
    g.fillRoundedRectangle (area.translated (offset.x, offset.y), cornerRadius);
}

void ui::drawSoftShadowEllipse (juce::Graphics& g,
                                juce::Rectangle<float> area,
                                float spread,
                                float alpha,
                                juce::Point<float> offset)
{
    const auto shadowColour = theme::palette().shadow;
    constexpr int steps = 9;

    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float grow = spread * t;
        const float layerAlpha = alpha * (1.0f - t) * (1.0f - t) * 0.50f;

        g.setColour (shadowColour.withAlpha (juce::jlimit (0.0f, 1.0f, layerAlpha)));
        g.fillEllipse (area.expanded (grow).translated (offset.x, offset.y));
    }

    g.setColour (shadowColour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha * 0.28f)));
    g.fillEllipse (area.translated (offset.x, offset.y));
}

//==============================================================================
void ui::drawKnob (juce::Graphics& g,
                   juce::Rectangle<float> area,
                   float discScale,
                   float angle)
{
    const auto& pal = theme::palette();
    const auto centre = area.getCentre();
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f * discScale;
    const auto disc = juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (centre);

    // ---- drop shadow -------------------------------------------------------
    drawSoftShadowEllipse (g, disc.reduced (R * 0.03f), R * 0.20f, 0.45f, { 0.0f, R * 0.055f });

    // ---- base body ---------------------------------------------------------
    {
        juce::ColourGradient base (pal.knobBodyHi, centre.x - R * 0.55f, centre.y - R * 0.9f,
                                   pal.knobBodyLo, centre.x + R * 0.55f, centre.y + R * 0.9f, false);
        base.addColour (0.45, pal.knobBodyMid);
        base.addColour (0.75, pal.knobBodyLo);
        g.setGradientFill (base);
        g.fillEllipse (disc);
    }

    // ---- serrated coin edge ------------------------------------------------
    {
        constexpr int teeth = 120;
        g.setColour (pal.knobEdgeDark.withAlpha (0.35f));

        for (int i = 0; i < teeth; ++i)
        {
            const float a = kTwoPi * (float) i / (float) teeth;
            const float s = std::sin (a);
            const float c = std::cos (a);
            const float r0 = R * 0.962f;
            const float r1 = R * 0.999f;

            g.drawLine (centre.x + s * r0, centre.y - c * r0,
                        centre.x + s * r1, centre.y - c * r1,
                        juce::jmax (0.7f, R * 0.007f));
        }

        g.setColour (juce::Colours::white.withAlpha (0.05f));

        for (int i = 0; i < teeth / 2; ++i)
        {
            const float a = kTwoPi * ((float) i + 0.5f) / (float) (teeth / 2);
            const float s = std::sin (a);
            const float c = std::cos (a);
            g.drawLine (centre.x + s * R * 0.972f, centre.y - c * R * 0.972f,
                        centre.x + s * R * 0.998f, centre.y - c * R * 0.998f,
                        juce::jmax (0.6f, R * 0.005f));
        }
    }

    // ---- lit outer bevel ---------------------------------------------------
    {
        juce::ColourGradient rim (pal.knobRimHi.withAlpha (0.9f), centre.x - R * 0.7f, centre.y - R * 0.9f,
                                  pal.knobRimLo.withAlpha (0.9f), centre.x + R * 0.55f, centre.y + R * 0.95f, false);
        rim.addColour (0.35, pal.knobRimHi.withAlpha (0.12f));
        g.setGradientFill (rim);
        g.drawEllipse (disc.reduced (R * 0.028f), juce::jmax (1.0f, R * 0.05f));
    }

    // ---- black gutter around the raised bezel -----------------------------
    g.setColour (pal.knobBodyLo);
    g.fillEllipse (disc.reduced (R * 0.078f));

    // ---- wavy raised bezel -------------------------------------------------
    juce::Path wavy;

    {
        constexpr int steps = 400;

        for (int i = 0; i <= steps; ++i)
        {
            const float th = kTwoPi * (float) i / (float) steps;
            const float r = R * (0.854f + 0.056f * std::cos (6.0f * (th - 0.14f))
                                       + 0.014f * std::cos (3.0f * th + 2.1f)
                                       + 0.006f * std::cos (2.0f * th + 1.1f));
            const float x = centre.x + std::sin (th) * r;
            const float y = centre.y - std::cos (th) * r;

            if (i == 0) wavy.startNewSubPath (x, y);
            else        wavy.lineTo (x, y);
        }

        wavy.closeSubPath();
    }

    // Step shadow so the raised part looks like it sits above the rim.
    {
        auto shadow = wavy;
        shadow.applyTransform (juce::AffineTransform::scale (1.035f, 1.035f, centre.x, centre.y));
        g.setColour (pal.shadow.withAlpha (0.50f));
        g.fillPath (shadow);
    }

    {
        juce::ColourGradient bez (pal.bezelHi, centre.x - R * 0.2f, centre.y - R,
                                  pal.bezelDeep, centre.x, centre.y + R, false);
        bez.addColour (0.10, pal.bezelUpper);
        bez.addColour (0.20, pal.bezelMid);
        bez.addColour (0.34, pal.bezelLow);
        bez.addColour (0.55, pal.bezelDeep);
        g.setGradientFill (bez);
        g.fillPath (wavy);
    }

    {
        g.saveState();
        g.reduceClipRegion (wavy);

        {
            juce::ColourGradient warm (pal.knobWarm.withAlpha (0.42f), centre.x - R * 0.98f, centre.y + R * 0.05f,
                                       pal.knobWarm.withAlpha (0.0f), centre.x - R * 0.30f, centre.y + R * 0.05f, false);
            g.setGradientFill (warm);
            g.fillRect (disc);
        }

        {
            juce::ColourGradient teal (pal.knobTeal.withAlpha (0.40f), centre.x + R * 0.98f, centre.y + R * 0.02f,
                                       pal.knobTeal.withAlpha (0.0f), centre.x + R * 0.20f, centre.y + R * 0.02f, false);
            g.setGradientFill (teal);
            g.fillRect (disc);
        }

        {
            juce::ColourGradient spec (pal.knobSpecular, centre.x, centre.y - R * 1.02f,
                                       pal.knobSpecular.withAlpha (0.0f), centre.x, centre.y - R * 0.58f, false);
            g.setGradientFill (spec);
            g.fillRect (disc);
        }

        g.restoreState();
    }

    // ---- flat ring around the aluminium cap --------------------------------
    {
        const float innerR = R * 0.700f;
        const auto inner = juce::Rectangle<float> (innerR * 2.0f, innerR * 2.0f).withCentre (centre);

        juce::ColourGradient flat (pal.flatTop, centre.x, centre.y - innerR,
                                   pal.flatLo, centre.x, centre.y + innerR, false);
        g.setGradientFill (flat);
        g.fillEllipse (inner);

        juce::ColourGradient chamfer (pal.chamferHi, centre.x, centre.y - innerR,
                                      pal.shadow.withAlpha (0.35f), centre.x, centre.y + innerR * 0.6f, false);
        chamfer.addColour (0.45, pal.chamferHi.withAlpha (0.10f));
        g.setGradientFill (chamfer);
        g.drawEllipse (inner.reduced (0.5f), juce::jmax (1.0f, R * 0.018f));
    }

    // ---- spun aluminium cap ------------------------------------------------
    {
        const float domeR = R * 0.575f;
        const auto dome = juce::Rectangle<float> (domeR * 2.0f, domeR * 2.0f).withCentre (centre);

        juce::ColourGradient silver (pal.capHi, centre.x - domeR * 0.55f, centre.y - domeR * 0.80f,
                                     pal.capLo, centre.x + domeR * 0.75f, centre.y + domeR * 0.90f, true);
        silver.addColour (0.14, pal.capHiMid);
        silver.addColour (0.42, pal.capMid);
        silver.addColour (0.70, pal.capLo);
        g.setGradientFill (silver);
        g.fillEllipse (dome);

        {
            g.saveState();
            juce::Path domeClip;
            domeClip.addEllipse (dome);
            g.reduceClipRegion (domeClip);

            constexpr int lines = 260;

            for (int i = 0; i < lines; ++i)
            {
                const float a = kTwoPi * (float) i / (float) lines;
                const float v = 0.5f + 0.5f * std::sin (a * 9.0f + 0.7f);
                g.setColour (juce::Colours::white.withAlpha (0.022f + 0.040f * v));
                g.drawLine (centre.x + std::sin (a) * domeR * 0.05f,
                            centre.y - std::cos (a) * domeR * 0.05f,
                            centre.x + std::sin (a) * domeR,
                            centre.y - std::cos (a) * domeR,
                            juce::jmax (0.5f, R * 0.005f));
            }

            {
                juce::ColourGradient spec (pal.capSpec, centre.x, centre.y - domeR * 1.0f,
                                           pal.capSpec.withAlpha (0.0f), centre.x, centre.y - domeR * 0.05f, false);
                g.setGradientFill (spec);
                g.fillEllipse (dome);
            }

            {
                juce::ColourGradient dim (pal.capShade.withAlpha (0.0f), centre.x - domeR * 0.1f, centre.y + domeR * 0.15f,
                                          pal.capShade, centre.x + domeR * 0.5f, centre.y + domeR, true);
                g.setGradientFill (dim);
                g.fillEllipse (dome);
            }

            g.restoreState();
        }

        // Spun-metal convergence in the middle.
        {
            const auto c1 = juce::Rectangle<float> (domeR * 0.16f, domeR * 0.16f).withCentre (centre);
            juce::ColourGradient hotSpot (juce::Colours::white.withAlpha (0.9f), centre.x - domeR * 0.02f, centre.y - domeR * 0.03f,
                                          juce::Colours::white.withAlpha (0.0f), centre.x + domeR * 0.09f, centre.y + domeR * 0.09f, true);
            g.setGradientFill (hotSpot);
            g.fillEllipse (c1);

            g.setColour (pal.capShade.withAlpha (0.16f));
            g.fillEllipse (juce::Rectangle<float> (domeR * 0.055f, domeR * 0.055f).withCentre (centre));
        }

        // Bevel around the cap.
        {
            juce::ColourGradient bevel (juce::Colours::white.withAlpha (0.85f), centre.x, centre.y - domeR,
                                        pal.shadow.withAlpha (0.55f), centre.x, centre.y + domeR, false);
            bevel.addColour (0.5, juce::Colours::white.withAlpha (0.09f));
            g.setGradientFill (bevel);
            g.drawEllipse (dome.reduced (0.6f), juce::jmax (0.9f, R * 0.011f));

            g.setColour (pal.shadow.withAlpha (0.45f));
            g.drawEllipse (dome.expanded (juce::jmax (0.8f, R * 0.010f)), juce::jmax (0.7f, R * 0.008f));
        }
    }

    // ---- indicator dot -----------------------------------------------------
    {
        const float dotDist = R * 0.893f;
        const float dotR = juce::jmax (1.8f, R * 0.043f);
        const juce::Point<float> dot (centre.x + std::sin (angle) * dotDist,
                                      centre.y - std::cos (angle) * dotDist);

        g.setColour (pal.shadow.withAlpha (0.60f));
        g.fillEllipse (juce::Rectangle<float> (dotR * 2.7f, dotR * 2.7f).withCentre (dot).translated (0.3f, 0.6f));

        juce::ColourGradient dotGrad (pal.indicatorHi, dot.x - dotR * 0.6f, dot.y - dotR * 0.7f,
                                      pal.indicatorLo, dot.x + dotR * 0.7f, dot.y + dotR * 0.8f, true);
        g.setGradientFill (dotGrad);
        g.fillEllipse (juce::Rectangle<float> (dotR * 2.0f, dotR * 2.0f).withCentre (dot));

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (dotR * 0.8f, dotR * 0.8f)
                           .withCentre (dot.translated (-dotR * 0.22f, -dotR * 0.28f)));
    }
}

//==============================================================================
void ui::drawFreezeButton (juce::Graphics& g,
                           juce::Rectangle<float> area,
                           bool on,
                           bool highlighted)
{
    const auto& pal = theme::palette();
    const auto centre = area.getCentre();
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto disc = juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (centre);

    drawSoftShadowEllipse (g, disc.reduced (R * 0.05f), R * 0.14f, 0.42f, { 0.0f, R * 0.05f });

    // Metal collar.
    {
        juce::ColourGradient metal (pal.collarHi, centre.x - R * 0.5f, centre.y - R * 0.85f,
                                    pal.collarLo, centre.x + R * 0.6f, centre.y + R * 0.9f, true);
        metal.addColour (0.40, pal.collarMid);
        metal.addColour (0.75, pal.collarLo);
        g.setGradientFill (metal);
        g.fillEllipse (disc);
    }

    // Bright outer bevel.
    {
        juce::ColourGradient rim (pal.collarRimHi, centre.x - R * 0.45f, centre.y - R * 0.9f,
                                  pal.collarRimLo, centre.x + R * 0.5f, centre.y + R * 0.9f, false);
        g.setGradientFill (rim);
        g.drawEllipse (disc.reduced (R * 0.025f), juce::jmax (1.0f, R * 0.05f));
    }

    if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillEllipse (disc);
    }

    // Recess shadow inside the collar.
    {
        juce::ColourGradient recess (pal.shadow.withAlpha (0.45f), centre.x, centre.y - R * 0.85f,
                                     pal.shadow.withAlpha (0.0f), centre.x, centre.y - R * 0.10f, false);
        g.setGradientFill (recess);
        g.drawEllipse (disc.reduced (R * 0.20f), juce::jmax (1.0f, R * 0.09f));
    }

    // Concave dish.
    const float dishR = R * 0.615f;
    const auto dish = juce::Rectangle<float> (dishR * 2.0f, dishR * 2.0f).withCentre (centre);

    if (! on)
    {
        juce::ColourGradient dishGrad (pal.dishOffHi, centre.x, centre.y - dishR,
                                       pal.dishOffLo, centre.x, centre.y + dishR, false);
        dishGrad.addColour (0.40, pal.dishOffMid);
        dishGrad.addColour (0.78, pal.dishOffLo.withAlpha (0.85f));
        g.setGradientFill (dishGrad);
        g.fillEllipse (dish);
    }
    else
    {
        juce::ColourGradient dishGrad (pal.dishOnHi, centre.x, centre.y - dishR,
                                       pal.dishOnLo, centre.x, centre.y + dishR, false);
        dishGrad.addColour (0.25, pal.dishOnMid);
        dishGrad.addColour (0.60, pal.dishOnLo);
        g.setGradientFill (dishGrad);
        g.fillEllipse (dish);

        g.setColour (pal.accent.withAlpha (0.35f));
        g.drawEllipse (dish.reduced (1.0f), 1.4f);
    }

    g.setColour (pal.shadow.withAlpha (0.55f));
    g.drawEllipse (dish, 1.0f);

    // Central knob / lamp.
    const float knobR = R * 0.315f;
    const auto knob = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);

    if (! on)
    {
        juce::ColourGradient knobGrad (pal.freezeKnobHi, centre.x - knobR * 0.4f, centre.y - knobR * 0.8f,
                                       pal.freezeKnobLo, centre.x + knobR * 0.5f, centre.y + knobR * 0.9f, true);
        g.setGradientFill (knobGrad);
        g.fillEllipse (knob);

        g.setColour (pal.shadow.withAlpha (0.45f));
        g.drawEllipse (knob, 0.8f);
    }
    else
    {
        juce::ColourGradient glow (juce::Colours::white.withAlpha (0.92f), centre.x, centre.y,
                                   pal.accent, centre.x + knobR * 0.9f, centre.y + knobR * 0.9f, true);
        g.setGradientFill (glow);
        g.fillEllipse (knob);

        g.setColour (pal.accent.withAlpha (0.40f));
        g.drawEllipse (knob.expanded (2.0f), 1.4f);

        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (juce::Rectangle<float> (knobR * 0.8f, knobR * 0.8f)
                           .withCentre (centre.translated (-knobR * 0.18f, -knobR * 0.22f)));
    }
}

//==============================================================================
void ui::drawLogoBadge (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto& pal = theme::palette();
    const auto centre = area.getCentre();
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto disc = juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (centre);

    g.setColour (pal.shadow.withAlpha (0.28f));
    g.fillEllipse (disc.translated (0.0f, R * 0.06f));

    // Dark backplate so the swirl reads on every theme.
    {
        juce::ColourGradient backplate (juce::Colour (0xff2b2137), centre.x - R * 0.4f, centre.y - R * 0.6f,
                                        juce::Colour (0xff0a070e), centre.x + R * 0.5f, centre.y + R * 0.6f, true);
        g.setGradientFill (backplate);
        g.fillEllipse (disc);
    }

    // Full-colour galaxy swirl, clipped to the circular badge. The logo colours
    // are intentionally never tinted by the UI theme.
    const auto& icon = brandIconImage();

    if (icon.isValid())
    {
        const auto innerDisc = disc.reduced (R * 0.045f);

        g.saveState();
        juce::Path clip;
        clip.addEllipse (innerDisc);
        g.reduceClipRegion (clip);

        const auto source = brandIconSwirlSource();
        const float scale = juce::jmax (innerDisc.getWidth() / source.getWidth(),
                                        innerDisc.getHeight() / source.getHeight());
        const auto destination = juce::Rectangle<float> (source.getWidth() * scale,
                                                         source.getHeight() * scale).withCentre (centre);

        g.drawImage (icon, destination.getX(), destination.getY(),
                     destination.getWidth(), destination.getHeight(),
                     (int) source.getX(), (int) source.getY(),
                     (int) source.getWidth(), (int) source.getHeight(), false);
        g.restoreState();
    }

    // Theme-aware ring / backplate edge only.
    g.setColour (pal.badgeInk.withAlpha (0.55f));
    g.drawEllipse (disc.reduced (1.0f), 1.5f);

    g.setColour (pal.shadow.withAlpha (0.40f));
    g.drawEllipse (disc.expanded (0.7f), 1.0f);
}

//==============================================================================
namespace
{
    const juce::Image& loadEmbeddedImage (const void* data, int dataSize, juce::Image& destination)
    {
        if (! destination.isValid())
            destination = juce::ImageCache::getFromMemory (data, dataSize);

        return destination;
    }
} // namespace

const juce::Image& ui::brandIconImage()
{
    static juce::Image image;
    return loadEmbeddedImage (BinaryData::abynth_icon_png, BinaryData::abynth_icon_pngSize, image);
}

const juce::Image& ui::brandLogoImage()
{
    static juce::Image image;
    return loadEmbeddedImage (BinaryData::abynth_logo_png, BinaryData::abynth_logo_pngSize, image);
}

juce::Rectangle<float> ui::brandIconSwirlSource()
{
    // The square icon also contains the wordmark at the bottom: crop tightly
    // around the galaxy swirl (vortex centre ~ (308, 256) in the 600x600 icon).
    return { 164.0f, 112.0f, 280.0f, 280.0f };
}

const juce::Image& ui::brandWordmarkImage()
{
    static juce::Image image;
    return loadEmbeddedImage (BinaryData::abynth_wordmark_png, BinaryData::abynth_wordmark_pngSize, image);
}

juce::Typeface::Ptr ui::comfortaaTypeface()
{
    static juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::ComfortaaBold_ttf,
                                                                                   BinaryData::ComfortaaBold_ttfSize);
    return typeface;
}

//==============================================================================
AboutContent::AboutContent()
{
    setSize (430, 520);

    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff6f4fa8));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.onClick = [this]
    {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->exitModalState (0);
    };

    addAndMakeVisible (closeButton);
}

void AboutContent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (juce::Colour (0xff1a1424), bounds.getCentreX(), bounds.getY(),
                                     juce::Colour (0xff0c0a10), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (background);
    g.fillAll();
    g.setGradientFill (background);
    g.fillRect (bounds);

    // Soft purple halo behind the logo.
    {
        juce::ColourGradient halo (juce::Colour (0x336f4fa8), bounds.getCentreX(), bounds.getY() + 190.0f,
                                   juce::Colour (0x00000000), bounds.getCentreX(), bounds.getY() + 380.0f, true);
        g.setGradientFill (halo);
        g.fillRect (bounds);
    }

    // Full-colour Abynth Audio logo.
    const auto& logo = ui::brandLogoImage();

    if (logo.isValid())
    {
        const auto logoArea = juce::Rectangle<float> (30.0f, 18.0f, 370.0f, 248.0f);
        g.drawImage (logo, logoArea, juce::RectanglePlacement::centred, false);
    }

    auto centred = [&g, &bounds] (const juce::String& text, float y, float height,
                                  juce::Colour colour, float fontHeight, bool bold)
    {
        g.setColour (colour);
        g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), fontHeight,
                               bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (text, juce::Rectangle<float> (0.0f, y, bounds.getWidth(), height),
                    juce::Justification::centred);
    };

    centred ("CarveSpace 1.0.0", 288.0f, 34.0f, juce::Colour (0xfff1ecff), 24.0f, true);
    centred ("Abynth Audio", 326.0f, 26.0f, juce::Colour (0xffb39ddb), 17.0f, false);
    centred ("32-band spectral sidechain carver", 362.0f, 22.0f, juce::Colour (0xff9a92ad), 13.0f, false);
    centred ("GPLv3 - no authorization required", 388.0f, 22.0f, juce::Colour (0xff9a92ad), 13.0f, false);

    g.setColour (juce::Colour (0x22ffffff));
    g.drawHorizontalLine ((int) (bounds.getHeight() - 92.0f), 40.0f, bounds.getWidth() - 40.0f);
}

void AboutContent::resized()
{
    closeButton.setBounds ((getWidth() - 108) / 2, getHeight() - 64, 108, 34);
}

//==============================================================================
juce::Colour ui::swatchColour (ThemeId id)
{
    switch (id)
    {
        case ThemeId::dark:  return juce::Colour (0xff43454a);
        case ThemeId::light: return juce::Colour (0xffebe5da);
        case ThemeId::pink:  return juce::Colour (0xfff8bbd0);
    }

    return juce::Colours::grey;
}

SkinButton::SkinButton()
    : juce::Button ("Theme")
{
    setTooltip ("Theme / skin: click to cycle Dark - Light - Pink");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SkinButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const float corner = bounds.getHeight() * 0.5f;

    g.setColour (pal.ink.withAlpha (down ? 0.20f : (highlighted ? 0.15f : 0.10f)));
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (pal.ink.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (0.6f), corner, 1.1f);

    const ThemeId ids[] = { ThemeId::dark, ThemeId::light, ThemeId::pink };
    const float dotRadius = juce::jmin (bounds.getHeight() * 0.19f, 6.2f);
    const float spacing = bounds.getWidth() / 4.0f;

    for (int i = 0; i < 3; ++i)
    {
        const juce::Point<float> dot (bounds.getX() + spacing * (float) (i + 1), bounds.getCentreY());
        const auto circle = juce::Rectangle<float> (dotRadius * 2.0f, dotRadius * 2.0f).withCentre (dot);

        if (ids[i] == theme::currentThemeId())
        {
            g.setColour (pal.ink.withAlpha (0.9f));
            g.drawEllipse (circle.expanded (2.4f), 1.6f);
        }

        g.setColour (ui::swatchColour (ids[i]));
        g.fillEllipse (circle);

        g.setColour (pal.ink.withAlpha (0.75f));
        g.drawEllipse (circle, 1.1f);
    }
}

void SkinButton::mouseDown (const juce::MouseEvent& event)
{
    secondaryPress = event.mods.isPopupMenu();

    if (secondaryPress)
    {
        if (onSecondaryClick != nullptr)
            onSecondaryClick();

        return;
    }

    juce::Button::mouseDown (event);
}

void SkinButton::mouseUp (const juce::MouseEvent& event)
{
    if (secondaryPress)
    {
        secondaryPress = false;
        return;
    }

    juce::Button::mouseUp (event);
}

//==============================================================================
CarveSpaceLookAndFeel::CarveSpaceLookAndFeel()
{
    // Use the logo's rounded geometric typeface (Comfortaa Bold) for all UI text.
    if (auto comfortaa = ui::comfortaaTypeface())
    {
        setDefaultSansSerifTypeface (comfortaa);
        juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface (comfortaa);
    }

    refreshTheme();
}

void CarveSpaceLookAndFeel::refreshTheme()
{
    const auto& pal = theme::palette();

    setColour (juce::ResizableWindow::backgroundColourId, pal.windowBg);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xee1d1d1d));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal.accent.withAlpha (0.55f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, pal.ink);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xee1d1d1d));
    setColour (juce::TooltipWindow::textColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, pal.accent);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
}

juce::Font CarveSpaceLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain);
}

juce::Typeface::Ptr CarveSpaceLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (font.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        if (auto comfortaa = ui::comfortaaTypeface())
            return comfortaa;

    return juce::LookAndFeel_V4::getTypefaceForFont (font);
}

void CarveSpaceLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPosProportional, float rotaryStartAngle,
                                              float rotaryEndAngle, juce::Slider& slider)
{
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    float discScale = 1.0f;

    if (auto* knob = dynamic_cast<ParameterKnob*> (&slider))
        discScale = knob->getDiscScale();

    ui::drawKnob (g, area, discScale, angle);
}

void CarveSpaceLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsDown);
    ui::drawFreezeButton (g, button.getLocalBounds().toFloat(),
                          button.getToggleState(), shouldDrawButtonAsHighlighted);
}
} // namespace ts
