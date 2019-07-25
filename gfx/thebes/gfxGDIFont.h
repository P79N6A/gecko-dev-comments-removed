




#ifndef GFX_GDIFONT_H
#define GFX_GDIFONT_H

#include "gfxFont.h"
#include "gfxGDIFontList.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

#include "cairo.h"

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

    
    virtual const gfxFont::Metrics& GetMetrics();

    virtual uint32_t GetSpaceGlyph();

    virtual bool SetupCairoFont(gfxContext *aContext);

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    virtual bool ProvidesGlyphWidths() { return true; }

    
    virtual int32_t GetGlyphWidth(gfxContext *aCtx, uint16_t aGID);

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;

    virtual FontType GetType() const { return FONT_TYPE_GDI; }

protected:
    virtual void CreatePlatformShaper();

    
    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aString,
                           bool aPreferPlatformShaping = false);

    void Initialize(); 

    
    
    
    void FillLogFont(LOGFONTW& aLogFont, gfxFloat aSize, bool aUseGDIFakeItalic);

    
    
    nsAutoPtr<gfxFontShaper>   mUniscribeShaper;

    HFONT                 mFont;
    cairo_font_face_t    *mFontFace;

    Metrics              *mMetrics;
    uint32_t              mSpaceGlyph;

    bool                  mNeedsBold;

    
    nsDataHashtable<nsUint32HashKey,int32_t>    mGlyphWidths;
};

#endif 
