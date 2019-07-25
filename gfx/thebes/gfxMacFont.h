







































#ifndef GFX_MACFONT_H
#define GFX_MACFONT_H

#include "gfxFont.h"
#include "gfxMacPlatformFontList.h"
#include "mozilla/gfx/2D.h"

#include "cairo.h"

class gfxMacFont : public gfxFont
{
public:
    gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
               bool aNeedsBold);

    virtual ~gfxMacFont();

    CGFontRef GetCGFontRef() const { return mCGFont; }

    
    virtual const gfxFont::Metrics& GetMetrics() {
        return mMetrics;
    }

    virtual PRUint32 GetSpaceGlyph() {
        return mSpaceGlyph;
    }

    virtual bool SetupCairoFont(gfxContext *aContext);

    
    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

    mozilla::RefPtr<mozilla::gfx::ScaledFont> GetScaledFont();

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;

protected:
    virtual void CreatePlatformShaper();

    
    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText,
                           bool aPreferPlatformShaping = false);

    void InitMetrics();
    void InitMetricsFromPlatform();
    void InitMetricsFromATSMetrics(ATSFontRef aFontRef);

    
    
    gfxFloat GetCharWidth(CFDataRef aCmap, PRUnichar aUniChar,
                          PRUint32 *aGlyphID, gfxFloat aConvFactor);

    static void DestroyBlobFunc(void* aUserData);

    
    
    CGFontRef             mCGFont;

    cairo_font_face_t    *mFontFace;

    Metrics               mMetrics;
    PRUint32              mSpaceGlyph;

    mozilla::RefPtr<mozilla::gfx::ScaledFont> mAzureFont;
};

#endif 
