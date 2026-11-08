#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
/** Selectable UI themes. */
enum class ThemeId
{
    dark = 0,
    light,
    pink
};

/** Complete colour set for one theme. Every painted colour in the UI comes
    from here so that switching theme only needs to rebuild this palette.
*/
struct Palette
{
    // Window / faceplate.
    juce::Colour windowBg;
    juce::Colour plateHi;
    juce::Colour plateMid;
    juce::Colour plateLo;
    juce::Colour plateWarmTint;
    juce::Colour plateCoolTint;
    juce::Colour plateSheen;
    juce::Colour plateBevelHi;
    juce::Colour plateBevelLo;
    juce::Colour ink;
    juce::Colour inkSoft;

    // Dark analyser panel.
    juce::Colour displayHi;
    juce::Colour displayLo;
    juce::Colour gridLine;
    juce::Colour gridFaint;
    juce::Colour panelOutline;

    // Traces and accent.
    juce::Colour sidechainBlue;
    juce::Colour reductionWhite;
    juce::Colour accent;
    juce::Colour accentText;

    // Metal knob body.
    juce::Colour knobBodyHi;
    juce::Colour knobBodyMid;
    juce::Colour knobBodyLo;
    juce::Colour knobRimHi;
    juce::Colour knobRimLo;
    juce::Colour knobEdgeDark;

    // Raised wavy bezel.
    juce::Colour bezelHi;
    juce::Colour bezelUpper;
    juce::Colour bezelMid;
    juce::Colour bezelLow;
    juce::Colour bezelDeep;
    juce::Colour knobWarm;
    juce::Colour knobTeal;
    juce::Colour knobSpecular;

    // Flat ring + aluminium cap.
    juce::Colour flatTop;
    juce::Colour flatLo;
    juce::Colour chamferHi;
    juce::Colour capHi;
    juce::Colour capHiMid;
    juce::Colour capMid;
    juce::Colour capLo;
    juce::Colour capSpec;
    juce::Colour capShade;
    juce::Colour indicatorHi;
    juce::Colour indicatorLo;

    // Freeze button.
    juce::Colour collarHi;
    juce::Colour collarMid;
    juce::Colour collarLo;
    juce::Colour collarRimHi;
    juce::Colour collarRimLo;
    juce::Colour dishOffHi;
    juce::Colour dishOffMid;
    juce::Colour dishOffLo;
    juce::Colour dishOnHi;
    juce::Colour dishOnMid;
    juce::Colour dishOnLo;
    juce::Colour freezeKnobHi;
    juce::Colour freezeKnobLo;

    // Badge + wordmark.
    juce::Colour badgeHi;
    juce::Colour badgeLo;
    juce::Colour badgeInk;
    juce::Colour wordmarkInk;

    // Shadows.
    juce::Colour shadow;

    // Geometry tweaks (pink reads softer with slightly rounder corners).
    float plateCornerExtra = 0.0f;
    float displayCornerExtra = 0.0f;
};

namespace theme
{
    ThemeId currentThemeId() noexcept;
    const Palette& palette() noexcept;
    void setTheme (ThemeId newTheme) noexcept;
    juce::String toString (ThemeId id);
    ThemeId fromString (const juce::String& name) noexcept;
} // namespace theme

/** Design-coordinate layout of the whole faceplate (650 x 650).

    All values were measured from the reference faceplate at 650x650 so the
    recreated faceplate lines up with the original artwork.
*/
namespace layout
{
    inline constexpr int designWidth  = 650;
    inline constexpr int designHeight = 650;

    inline constexpr float plateCorner   = 26.0f;
    inline constexpr float displayCorner = 20.0f;

    // Dark spectrum panel.
    inline const juce::Rectangle<float> display { 54.0f, 55.0f, 541.0f, 205.0f };

    // Main knobs (component bounds include a little room for the drop shadow).
    inline const juce::Rectangle<float> amountKnob  { 213.0f, 274.0f, 224.0f, 224.0f };
    inline const juce::Rectangle<float> lowCutKnob  { 43.0f, 445.0f, 156.0f, 156.0f };
    inline const juce::Rectangle<float> highCutKnob { 450.0f, 445.0f, 156.0f, 156.0f };

    inline constexpr float amountDiscScale  = 0.9085f; // disc radius / half bounds
    inline constexpr float smallDiscScale   = 0.8365f;

    // Freeze button (metal push button) and its label.
    inline const juce::Rectangle<float> freezeButton { 90.0f, 309.0f, 62.0f, 62.0f };
    inline const juce::Rectangle<float> freezeLabel  { 100.0f, 278.0f, 42.0f, 13.0f };

    // "— CARVESPACE —" brand line.
    inline const juce::Rectangle<float> brandDashLeft  { 462.0f, 284.0f, 16.0f, 2.0f };
    inline const juce::Rectangle<float> brandText      { 487.0f, 277.0f, 82.0f, 14.0f };
    inline const juce::Rectangle<float> brandDashRight { 579.0f, 284.0f, 16.0f, 2.0f };

    // Amount scale markings.
    inline const juce::Rectangle<float> amountDotLeft  { 242.0f, 478.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> amountDotRight { 397.0f, 478.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> amountZero     { 244.0f, 493.0f, 5.0f, 11.0f };
    inline const juce::Rectangle<float> amountHundred  { 393.0f, 493.0f, 16.0f, 11.0f };
    inline const juce::Rectangle<float> amountCaption  { 300.0f, 506.0f, 49.0f, 14.0f };

    // Low-cut markings.
    inline const juce::Rectangle<float> lowCutDotLeft   { 61.0f, 578.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> lowCutDotRight  { 171.0f, 578.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> lowCutMinLabel  { 53.0f, 595.0f, 26.0f, 11.0f };
    inline const juce::Rectangle<float> lowCutMaxLabel  { 165.0f, 595.0f, 20.0f, 11.0f };
    inline const juce::Rectangle<float> lowCutCaption   { 95.0f, 607.0f, 52.0f, 14.0f };

    // High-cut markings.
    inline const juce::Rectangle<float> highCutDotLeft  { 468.0f, 578.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> highCutDotRight { 578.0f, 578.0f, 9.0f, 9.0f };
    inline const juce::Rectangle<float> highCutMinLabel { 460.0f, 595.0f, 26.0f, 11.0f };
    inline const juce::Rectangle<float> highCutMaxLabel { 571.0f, 595.0f, 20.0f, 11.0f };
    inline const juce::Rectangle<float> highCutCaption  { 501.0f, 607.0f, 55.0f, 14.0f };

    // Little angle marks above the cut knobs.
    inline const juce::Rectangle<float> bracketLeft  { 101.0f, 431.0f, 39.0f, 15.0f };
    inline const juce::Rectangle<float> bracketRight { 508.0f, 431.0f, 40.0f, 15.0f };

    // Brand badge and maker wordmark.
    inline const juce::Rectangle<float> logo      { 307.5f, 542.5f, 35.0f, 35.0f }; // swirl badge, centred on x=325
    inline const juce::Rectangle<float> wordmark  { 252.5f, 604.0f, 145.0f, 31.0f }; // full-colour wordmark image

    // Always-visible theme / skin switcher (top-right of the plate).
    inline const juce::Rectangle<float> skinButton { 578.0f, 12.0f, 56.0f, 32.0f };
} // namespace layout

/** Small drawing helpers shared by the faceplate components. */
namespace ui
{
    juce::Font boldFont (float pixelHeight);
    juce::Font regularFont (float pixelHeight);

    /** Draws text squeezed/stretched so its ink box exactly matches target. */
    void drawFittedText (juce::Graphics& g,
                         const juce::String& text,
                         juce::Rectangle<float> target,
                         juce::Colour colour,
                         bool bold = true);

    /** Draws text with per-glyph tracking so its ink box exactly matches target. */
    void drawTrackedText (juce::Graphics& g,
                          const juce::String& text,
                          juce::Rectangle<float> target,
                          juce::Colour colour,
                          bool bold = true);

    /** Cheap multi-pass soft shadow for a rounded rectangle. */
    void drawSoftShadow (juce::Graphics& g,
                         juce::Rectangle<float> area,
                         float cornerRadius,
                         float spread,
                         float alpha,
                         juce::Point<float> offset = {});

    /** Cheap multi-pass soft shadow for an ellipse. */
    void drawSoftShadowEllipse (juce::Graphics& g,
                                juce::Rectangle<float> area,
                                float spread,
                                float alpha,
                                juce::Point<float> offset = {});

    /** Draws the full metal knob (bezel + spun aluminium cap + indicator dot).

        @param area    square area, the knob disc is drawn inside it
        @param discScale  disc radius as a fraction of half the area width
        @param angle   indicator angle in radians (0 = up, clockwise)
    */
    void drawKnob (juce::Graphics& g,
                   juce::Rectangle<float> area,
                   float discScale,
                   float angle);

    /** Metal freeze push-button, off/on state. */
    void drawFreezeButton (juce::Graphics& g,
                           juce::Rectangle<float> area,
                           bool on,
                           bool highlighted);

    /** Round engraved brand badge. */
    void drawLogoBadge (juce::Graphics& g, juce::Rectangle<float> area);

    /** The three fixed theme swatch colours used by the skin button. */
    juce::Colour swatchColour (ThemeId id);

    /** Full-colour Abynth Audio brand assets (embedded PNG data). The colours
        of the logos are never tinted by the UI theme. */
    const juce::Image& brandIconImage();
    const juce::Image& brandLogoImage();

    /** Source rectangle of the galaxy swirl inside the square icon. */
    juce::Rectangle<float> brandIconSwirlSource();

    /** Full-colour Abynth Audio wordmark image (transparent background). */
    const juce::Image& brandWordmarkImage();

    /** Embedded Comfortaa Bold typeface used for all UI text. */
    juce::Typeface::Ptr comfortaaTypeface();
} // namespace ui

/** Custom About / info dialog content: full-colour Abynth Audio logo plus the
    version, company and licence credits.
*/
class AboutContent : public juce::Component
{
public:
    AboutContent();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextButton closeButton { "Close" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutContent)
};

/** Small always-visible skin button: three colour swatches. Left click cycles
    Dark -> Light -> Pink; right click can open a theme menu.
*/
class SkinButton : public juce::Button
{
public:
    SkinButton();

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    std::function<void()> onSecondaryClick;

private:
    bool secondaryPress = false;
};

class CarveSpaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CarveSpaceLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

    /** Re-applies the palette-dependent LookAndFeel colours (popups, tooltips). */
    void refreshTheme();
};
} // namespace ts
