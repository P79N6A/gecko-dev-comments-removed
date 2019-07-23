





































#ifndef GFX_QTFONTS_H
#define GFX_QTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxContext.h"

#include "nsDataHashtable.h"
#include "nsClassHashtable.h"

#define ENABLE_FAST_PATH_8BIT 1
#define ENABLE_FAST_PATH_ALWAYS 1

class QFont;

class gfxQtFont : public gfxFont {
public: 
    gfxQtFont (const nsAString& aName,
            const gfxFontStyle *aFontStyle);
    virtual ~gfxQtFont ();

    inline const QFont& GetQFont();

public: 
    virtual PRUint32 GetSpaceGlyph ();
    virtual const gfxFont::Metrics& GetMetrics();
    cairo_font_face_t *CairoFontFace(QFont *aFont = nsnull);

protected: 
    virtual nsString GetUniqueName ();
    virtual PRBool SetupCairoFont(gfxContext *aContext);

protected: 
    cairo_scaled_font_t* CreateScaledFont(cairo_t *aCR, 
                                          cairo_matrix_t *aCTM, 
                                          QFont &aQFont);

protected: 

    QFont* mQFont;
    cairo_scaled_font_t *mCairoFont;
    cairo_font_face_t *mFontFace;

    PRBool mHasSpaceGlyph;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

};

class THEBES_API gfxQtFontGroup : public gfxFontGroup {
public: 
    gfxQtFontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle);
    virtual ~gfxQtFontGroup ();

    inline gfxQtFont * GetFontAt (PRInt32 i);

protected: 
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, 
                                    PRUint32 aLength,
                                    const Parameters *aParams, 
                                    PRUint32 aFlags);

    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, 
                                    PRUint32 aLength,
                                    const Parameters *aParams, 
                                    PRUint32 aFlags);

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);


protected: 
    void InitTextRun(gfxTextRun *aTextRun, 
                     const PRUint8 *aUTF8Text,
                     PRUint32 aUTF8Length, 
                     PRUint32 aUTF8HeaderLength);

    void CreateGlyphRunsFT(gfxTextRun *aTextRun, 
                           const PRUint8 *aUTF8,
                           PRUint32 aUTF8Length);

    static PRBool FontCallback (const nsAString & fontName, 
                                const nsACString & genericName, 
                                void *closure);
    PRBool mEnableKerning;

};

inline const QFont& gfxQtFont::GetQFont()
{
    return *mQFont;
}

inline gfxQtFont * gfxQtFontGroup::GetFontAt (PRInt32 i) 
{
    return static_cast < gfxQtFont * >(static_cast < gfxFont * >(mFonts[i]));
}


#endif 

