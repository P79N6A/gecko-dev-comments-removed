




































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

#include "nsAutoBuffer.h"
#include "nsICharsetConverterManager.h"

class gfxOS2Font : public gfxFont {
public:
    gfxOS2Font(const nsAString &aName, const gfxFontStyle *aFontStyle);
    virtual ~gfxOS2Font();

    virtual const gfxFont::Metrics& GetMetrics();
    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();

    virtual nsString GetUniqueName();

    
    virtual PRUint32 GetSpaceGlyph() {
        if (!mMetrics)
            GetMetrics();
        return mSpaceGlyph;
    }

protected:
    gfxMatrix mCTM;
    virtual void SetupCairoFont(cairo_t *aCR);

private:
    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;
    Metrics *mMetrics;
    PRUint32 mSpaceGlyph;
};


class THEBES_API gfxOS2FontGroup : public gfxFontGroup {
public:
    gfxOS2FontGroup(const nsAString& aFamilies, const gfxFontStyle* aStyle);
    virtual ~gfxOS2FontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);

    gfxOS2Font *GetFontAt(PRInt32 i) {
#ifdef DEBUG_thebes_2
        printf("gfxOS2FontGroup[%#x]::GetFontAt(%d), %#x, %#x\n",
               (unsigned)this, i, (unsigned)&mFonts, (unsigned)&mFonts[i]);
#endif
        return static_cast<gfxOS2Font*>(static_cast<gfxFont*>(mFonts[i]));
    }

    gfxOS2Font *GetCachedFont(const nsAString& aName) const {
        nsRefPtr<gfxOS2Font> font;
        if (mFontCache.Get(aName, &font))
            return font;
        return nsnull;
    }

    void PutCachedFont(const nsAString& aName, gfxOS2Font *aFont) {
        mFontCache.Put(aName, aFont);
    }

protected:
    void InitTextRun(gfxTextRun *aTextRun, const PRUint8 *aUTF8Text,
                     PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength);
    void CreateGlyphRunsFT(gfxTextRun *aTextRun, const PRUint8 *aUTF8,
                           PRUint32 aUTF8Length);
    static PRBool FontCallback(const nsAString& aFontName,
                               const nsACString& aGenericName, void *aClosure);

private:
    nsDataHashtable<nsStringHashKey, nsRefPtr<gfxOS2Font> > mFontCache;
};

#endif 
