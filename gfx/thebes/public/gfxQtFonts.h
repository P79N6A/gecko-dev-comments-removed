





































#ifndef GFX_QTFONTS_H
#define GFX_QTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxContext.h"

#include "nsDataHashtable.h"
#include "nsClassHashtable.h"



class gfxQtFont : public gfxFont {
public:
     gfxQtFont (const nsAString& aName,
                const gfxFontStyle *aFontStyle);
     virtual ~gfxQtFont ();

     virtual nsString GetUniqueName ();

     virtual PRUint32 GetSpaceGlyph ()
     {
        NS_ASSERTION (GetStyle ()->size != 0,
        "forgot to short-circuit a text run with zero-sized font?");
        GetMetrics ();
       return mSpaceGlyph;
     }

    static void Shutdown();

    virtual const gfxFont::Metrics& GetMetrics();

protected:
    void *mQFont;
    cairo_scaled_font_t *mCairoFont;

    PRBool mHasMetrics;
    PRUint32 mSpaceGlyph;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

    virtual PRBool SetupCairoFont(gfxContext *aContext);

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


    static PRBool FontCallback (const nsAString & fontName, const nsACString & genericName, void *closure);

};

#endif 

