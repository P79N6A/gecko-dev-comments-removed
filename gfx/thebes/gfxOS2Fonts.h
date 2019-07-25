




































#ifndef GFX_OS2_FONTS_H
#define GFX_OS2_FONTS_H

#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxMatrix.h"
#include "nsDataHashtable.h"

#define INCL_GPI
#include <os2.h>
#include <cairo-os2.h>
#include "cairo-ft.h" 
#include <freetype/tttables.h>

#include "nsICharsetConverterManager.h"

class gfxOS2FontEntry : public gfxFontEntry {
public:
    gfxOS2FontEntry(const nsAString& aName) : gfxFontEntry(aName) {}
    ~gfxOS2FontEntry() {}
};

class gfxOS2Font : public gfxFont {
public:
    gfxOS2Font(gfxOS2FontEntry *aFontEntry, const gfxFontStyle *aFontStyle);
    virtual ~gfxOS2Font();

    virtual const gfxFont::Metrics& GetMetrics();
    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();

    
    virtual PRUint32 GetSpaceGlyph() {
        if (!mMetrics)
            GetMetrics();
        return mSpaceGlyph;
    }

    static already_AddRefed<gfxOS2Font> GetOrMakeFont(const nsAString& aName,
                                                      const gfxFontStyle *aStyle);

protected:
    virtual PRBool SetupCairoFont(gfxContext *aContext);

private:
    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;
    Metrics *mMetrics;
    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;
    int mHinting;
    PRBool mAntialias;
};


class THEBES_API gfxOS2FontGroup : public gfxFontGroup {
public:
    gfxOS2FontGroup(const nsAString& aFamilies, const gfxFontStyle* aStyle, gfxUserFontSet *aUserFontSet);
    virtual ~gfxOS2FontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);

    gfxOS2Font *GetFontAt(PRInt32 i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");

#ifdef DEBUG_thebes_2
        printf("gfxOS2FontGroup[%#x]::GetFontAt(%d), %#x, %#x\n",
               (unsigned)this, i, (unsigned)&mFonts, (unsigned)&mFonts[i]);
#endif
        return static_cast<gfxOS2Font*>(static_cast<gfxFont*>(mFonts[i]));
    }

protected:
    void InitTextRun(gfxTextRun *aTextRun, const PRUint8 *aUTF8Text,
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength);
    void CreateGlyphRunsFT(gfxTextRun *aTextRun, const PRUint8 *aUTF8,
                           PRUint32 aUTF8Length);
    static PRBool FontCallback(const nsAString& aFontName,
                               const nsACString& aGenericName, void *aClosure);

private:
    PRBool mEnableKerning;
};

#endif 
