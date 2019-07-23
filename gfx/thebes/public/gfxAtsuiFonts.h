





































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

    float GetCharWidth(PRUnichar c, PRUint32 *aGlyphID = nsnull);
    float GetCharHeight(PRUnichar c);

    ATSUFontID GetATSUFontID() { return mATSUFontID; }

    cairo_font_face_t *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    ATSUStyle GetATSUStyle() { return mATSUStyle; }

    virtual nsString GetUniqueName();

    virtual PRUint32 GetSpaceGlyph() { return mSpaceGlyph; }
    
    PRBool HasMirroringInfo();

    virtual void SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
            PRBool aNeedTight, gfxGlyphExtents *aExtents);

protected:
    const gfxFontStyle *mFontStyle;

    ATSUFontID mATSUFontID;
    ATSUStyle mATSUStyle;
    
    PRBool mHasMirroring;
    PRBool mHasMirroringLookedUp;

    nsString mUniqueName;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics mMetrics;

    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;    

    void InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef);

    virtual PRBool SetupCairoFont(gfxContext *aContext);
};

class THEBES_API gfxAtsuiFontGroup : public gfxFontGroup {
public:
    gfxAtsuiFontGroup(const nsAString& families,
                      const gfxFontStyle *aStyle);
    virtual ~gfxAtsuiFontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    
    
    
    
    
    
    void MakeTextRunInternal(const PRUnichar *aString, PRUint32 aLength,
                             PRBool aWrapped, gfxTextRun *aTextRun);

    ATSUFontFallbacks *GetATSUFontFallbacksPtr() { return &mFallbacks; }
    
    gfxAtsuiFont* GetFontAt(PRInt32 i) {
        return static_cast<gfxAtsuiFont*>(static_cast<gfxFont*>(mFonts[i]));
    }

    gfxAtsuiFont* FindFontFor(ATSUFontID fid);

protected:
    static PRBool FindATSUFont(const nsAString& aName,
                               const nsACString& aGenericName,
                               void *closure);

    PRUint32 GuessMaximumStringLength();

    
    PRBool InitTextRun(gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength,
                       PRBool aWrapped, PRUint32 aSegmentStart, PRUint32 aSegmentLength);

    ATSUFontFallbacks mFallbacks;
};
#endif 
