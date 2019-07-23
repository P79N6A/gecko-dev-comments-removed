







































#ifndef GFX_ATSUIFONTS_H
#define GFX_ATSUIFONTS_H

#ifndef __LP64__ 

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

class gfxAtsuiFontGroup;

class MacOSFontEntry;

#define kLiGothicBadCharUnicode  0x775B // ATSUI failure on 10.6 (bug 532346)
#define kLiGothicBadCharGlyph    3774   // the expected glyph for this char

class gfxAtsuiFont : public gfxFont {
public:

    gfxAtsuiFont(MacOSFontEntry *aFontEntry,
                 const gfxFontStyle *fontStyle, PRBool aNeedsBold);

    virtual ~gfxAtsuiFont();

    virtual const gfxFont::Metrics& GetMetrics();

    float GetCharWidth(PRUnichar c, PRUint32 *aGlyphID = nsnull);
    float GetCharHeight(PRUnichar c);

    ATSFontRef GetATSFontRef();

    cairo_font_face_t *CairoFontFace() { return mFontFace; }
    cairo_scaled_font_t *CairoScaledFont() { return mScaledFont; }

    ATSUStyle GetATSUStyle() { return mATSUStyle; }

    virtual nsString GetUniqueName();

    virtual PRUint32 GetSpaceGlyph() { return mSpaceGlyph; }

    PRBool HasMirroringInfo();

    virtual void SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
            PRBool aNeedTight, gfxGlyphExtents *aExtents);

    PRBool TestCharacterMap(PRUint32 aCh);

    MacOSFontEntry* GetFontEntry();
    PRBool Valid() { return mIsValid; }

protected:
    const gfxFontStyle *mFontStyle;

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
                      const gfxFontStyle *aStyle,
                      gfxUserFontSet *aUserFontSet);
    virtual ~gfxAtsuiFontGroup() {};

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    
    
    
    
    
    
    void MakeTextRunInternal(const PRUnichar *aString, PRUint32 aLength,
                             PRBool aWrapped, gfxTextRun *aTextRun);

    gfxAtsuiFont* GetFontAt(PRInt32 aFontIndex) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");

        return static_cast<gfxAtsuiFont*>(static_cast<gfxFont*>(mFonts[aFontIndex]));
    }

    PRBool HasFont(ATSFontRef aFontRef);

    inline gfxAtsuiFont* WhichFontSupportsChar(nsTArray< nsRefPtr<gfxFont> >& aFontList, 
                                               PRUint32 aCh)
    {
        PRUint32 len = aFontList.Length();
        for (PRUint32 i = 0; i < len; i++) {
            gfxAtsuiFont* font = static_cast<gfxAtsuiFont*>(aFontList.ElementAt(i).get());
            if (font->TestCharacterMap(aCh))
                return font;
        }
        return nsnull;
    }

    
    already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);

    already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    void UpdateFontList();

protected:
    static PRBool FindATSFont(const nsAString& aName,
                              const nsACString& aGenericName,
                              void *closure);

    PRUint32 GuessMaximumStringLength();

    
















    PRBool InitTextRun(gfxTextRun *aRun,
                       const PRUnichar *aString, PRUint32 aLength,
                       PRUint32 aLayoutStart, PRUint32 aLayoutLength,
                       PRUint32 aOffsetInTextRun, PRUint32 aLengthInTextRun);

    



    void InitFontList();
    
    
    nsRefPtr<gfxFontFamily>       mLastPrefFamily;
    nsRefPtr<gfxAtsuiFont>        mLastPrefFont;
    eFontPrefLang                 mLastPrefLang;       
    PRBool                        mLastPrefFirstFont;  
    eFontPrefLang                 mPageLang;
};

#endif 

#endif 
