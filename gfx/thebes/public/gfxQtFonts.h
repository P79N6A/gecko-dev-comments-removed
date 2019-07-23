





































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

protected: 
    virtual nsString GetUniqueName ();
    virtual PRUint32 GetSpaceGlyph ();
    virtual const gfxFont::Metrics& GetMetrics();
    virtual PRBool SetupCairoFont(gfxContext *aContext);

protected: 
    cairo_scaled_font_t* CreateScaledFont(cairo_t *aCR, 
                                          cairo_matrix_t *aCTM, 
                                          QFont &aQFont);

protected: 

    QFont* mQFont;
    cairo_scaled_font_t *mCairoFont;

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

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);

    gfxQtFont * GetFontAt (PRInt32 i) {
        return static_cast < gfxQtFont * >(static_cast < gfxFont * >(mFonts[i]));
    }

protected:
    void InitTextRun (gfxTextRun * aTextRun, const char * aUTF8Text,
                      PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                      PRBool aTake8BitPath);






#if defined(ENABLE_FAST_PATH_8BIT) || defined(ENABLE_FAST_PATH_ALWAYS)
    PRBool CanTakeFastPath (PRUint32 aFlags);
    nsresult CreateGlyphRunsFast (gfxTextRun * aTextRun,
                                  const char * aUTF8, PRUint32 aUTF8Length);
#endif


    static PRBool FontCallback (const nsAString & fontName, const nsACString & genericName, void *closure);

};

#endif 

