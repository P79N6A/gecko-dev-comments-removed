




#ifndef GFX_GDIFONT_H
#define GFX_GDIFONT_H

#include "mozilla/MemoryReporting.h"
#include "gfxFont.h"
#include "gfxGDIFontList.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

#include "cairo.h"
#include "usp10.h"

class gfxGDIFont : public gfxFont
{
public:
    gfxGDIFont(GDIFontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle,
               bool aNeedsBold,
               AntialiasOption anAAOption = kAntialiasDefault);

    virtual ~gfxGDIFont();

    HFONT GetHFONT() { if (!mMetrics) Initialize(); return mFont; }

    gfxFloat GetAdjustedSize() { if (!mMetrics) Initialize(); return mAdjustedSize; }

    cairo_font_face_t   *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    
    virtual uint32_t GetSpaceGlyph();

    virtual bool SetupCairoFont(gfxContext *aContext);

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing,
                               uint16_t aOrientation);

    
    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    
    
    virtual bool ProvidesGetGlyph() const {
        return !mFontEntry->HasCmapTable();
    }

    virtual uint32_t GetGlyph(uint32_t aUnicode, uint32_t aVarSelector);

    virtual bool ProvidesGlyphWidths() const { return true; }

    
    virtual int32_t GetGlyphWidth(DrawTarget& aDrawTarget, uint16_t aGID);

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_GDI; }

protected:
    virtual const Metrics& GetHorizontalMetrics();

    
    virtual bool ShapeText(gfxContext     *aContext,
                           const char16_t *aText,
                           uint32_t        aOffset,
                           uint32_t        aLength,
                           int32_t         aScript,
                           bool            aVertical,
                           gfxShapedText  *aShapedText);

    void Initialize(); 

    
    
    
    void FillLogFont(LOGFONTW& aLogFont, gfxFloat aSize, bool aUseGDIFakeItalic);

    HFONT                 mFont;
    cairo_font_face_t    *mFontFace;

    Metrics              *mMetrics;
    uint32_t              mSpaceGlyph;

    bool                  mNeedsBold;

    
    nsAutoPtr<nsDataHashtable<nsUint32HashKey,uint32_t> > mGlyphIDs;
    SCRIPT_CACHE          mScriptCache;

    
    nsAutoPtr<nsDataHashtable<nsUint32HashKey,int32_t> > mGlyphWidths;
};

#endif 
