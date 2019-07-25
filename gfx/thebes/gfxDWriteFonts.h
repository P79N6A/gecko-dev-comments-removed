




































#ifndef GFX_WINDOWSDWRITEFONTS_H
#define GFX_WINDOWSDWRITEFONTS_H

#include <dwrite.h>

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"




class gfxDWriteFont : public gfxFont 
{
public:
    gfxDWriteFont(gfxFontEntry *aFontEntry,
                  const gfxFontStyle *aFontStyle,
                  PRBool aNeedsBold = PR_FALSE,
                  AntialiasOption = kAntialiasDefault);
    ~gfxDWriteFont();

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption);

    virtual nsString GetUniqueName();

    virtual const gfxFont::Metrics& GetMetrics();

    virtual PRUint32 GetSpaceGlyph();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    virtual PRBool IsValid() { return mFontFace != NULL; }

    gfxFloat GetAdjustedSize() const { return mAdjustedSize; }

    IDWriteFontFace *GetFontFace() { return mFontFace.get(); }

    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

    virtual PRBool ProvidesHintedWidths() const {
        return !mUsingClearType;
    }

    virtual PRInt32 GetHintedGlyphWidth(gfxContext *aCtx, PRUint16 aGID);

protected:
    virtual void CreatePlatformShaper();

    void ComputeMetrics();

    cairo_font_face_t *CairoFontFace();

    cairo_scaled_font_t *CairoScaledFont();

    static void DestroyBlobFunc(void* userArg);

    nsRefPtr<IDWriteFontFace> mFontFace;
    cairo_font_face_t *mCairoFontFace;
    cairo_scaled_font_t *mCairoScaledFont;

    gfxFont::Metrics mMetrics;

    
    nsDataHashtable<nsUint32HashKey,PRInt32>    mGlyphWidths;

    PRPackedBool mNeedsOblique;
    PRPackedBool mNeedsBold;
    PRPackedBool mUsingClearType;
};

#endif
