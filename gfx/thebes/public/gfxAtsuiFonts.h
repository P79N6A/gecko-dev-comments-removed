





































#ifndef GFX_ATSUIFONTS_H
#define GFX_ATSUIFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"

#include <Carbon/Carbon.h>

class gfxAtsuiFontGroup;

class gfxAtsuiFont : public gfxFont {
public:
    gfxAtsuiFont(ATSUFontID fontID,
                 const nsAString& name,
                 const gfxFontStyle *fontStyle);
    virtual ~gfxAtsuiFont();

    virtual const gfxFont::Metrics& GetMetrics();

    float GetCharWidth(PRUnichar c);
    float GetCharHeight(PRUnichar c);

    ATSUFontID GetATSUFontID() { return mATSUFontID; }

    cairo_font_face_t *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    ATSUStyle GetATSUStyle() { return mATSUStyle; }

    virtual nsString GetUniqueName();

protected:
    const gfxFontStyle *mFontStyle;

    ATSUFontID mATSUFontID;
    ATSUStyle mATSUStyle;

    nsString mUniqueName;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics mMetrics;

    gfxFloat mAdjustedSize;
    void InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef);

    virtual void SetupCairoFont(cairo_t *aCR)
    {
        cairo_set_scaled_font (aCR, CairoScaledFont());
    }
};

class THEBES_API gfxAtsuiFontGroup : public gfxFontGroup {
public:
    gfxAtsuiFontGroup(const nsAString& families,
                      const gfxFontStyle *aStyle);
    virtual ~gfxAtsuiFontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    Parameters* aParams);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    Parameters* aParams);
    
    
    
    
    
    
    gfxTextRun *MakeTextRunInternal(const PRUnichar *aString, PRUint32 aLength,
                                    PRBool aWrapped, Parameters *aParams);

    ATSUFontFallbacks *GetATSUFontFallbacksPtr() { return &mFallbacks; }
    
    gfxAtsuiFont* GetFontAt(PRInt32 i) {
        return NS_STATIC_CAST(gfxAtsuiFont*, NS_STATIC_CAST(gfxFont*, mFonts[i]));
    }

    gfxAtsuiFont* FindFontFor(ATSUFontID fid);

protected:
    static PRBool FindATSUFont(const nsAString& aName,
                               const nsACString& aGenericName,
                               void *closure);

    void InitTextRun(gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength,
                     PRBool aWrapped);

    ATSUFontFallbacks mFallbacks;
};
#endif 
