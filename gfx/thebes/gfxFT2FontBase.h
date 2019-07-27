




#ifndef GFX_FT2FONTBASE_H
#define GFX_FT2FONTBASE_H

#include "cairo.h"
#include "gfxContext.h"
#include "gfxFont.h"
#include "mozilla/gfx/2D.h"

class gfxFT2FontBase : public gfxFont {
public:
    gfxFT2FontBase(cairo_scaled_font_t *aScaledFont,
                   gfxFontEntry *aFontEntry,
                   const gfxFontStyle *aFontStyle);
    virtual ~gfxFT2FontBase();

    uint32_t GetGlyph(uint32_t aCharCode);
    void GetGlyphExtents(uint32_t aGlyph,
                         cairo_text_extents_t* aExtents);
    virtual uint32_t GetSpaceGlyph() MOZ_OVERRIDE;
    virtual bool ProvidesGetGlyph() const MOZ_OVERRIDE { return true; }
    virtual uint32_t GetGlyph(uint32_t unicode,
                              uint32_t variation_selector) MOZ_OVERRIDE;
    virtual bool ProvidesGlyphWidths() const MOZ_OVERRIDE { return true; }
    virtual int32_t GetGlyphWidth(DrawTarget& aDrawTarget,
                                  uint16_t aGID) MOZ_OVERRIDE;

    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; };
    virtual bool SetupCairoFont(gfxContext *aContext) MOZ_OVERRIDE;

    virtual FontType GetType() const MOZ_OVERRIDE { return FONT_TYPE_FT2; }

    mozilla::gfx::FontOptions* GetFontOptions() { return &mFontOptions; }

protected:
    virtual const Metrics& GetHorizontalMetrics() MOZ_OVERRIDE;

    uint32_t mSpaceGlyph;
    bool mHasMetrics;
    Metrics mMetrics;

    
    mozilla::gfx::FontOptions  mFontOptions;
    void ConstructFontOptions();
};

#endif 
