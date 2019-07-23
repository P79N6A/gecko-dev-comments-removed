







































#ifndef GFX_CORETEXTFONTS_H
#define GFX_CORETEXTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

class gfxCoreTextFontGroup;

class MacOSFontEntry;

class gfxCoreTextFont : public gfxFont {
public:

    gfxCoreTextFont(MacOSFontEntry *aFontEntry,
                    const gfxFontStyle *fontStyle, PRBool aNeedsBold);

    virtual ~gfxCoreTextFont();

    virtual const gfxFont::Metrics& GetMetrics() {
        NS_ASSERTION(mHasMetrics == PR_TRUE, "metrics not initialized");
        return mMetrics;
    }

    float GetCharWidth(PRUnichar c, PRUint32 *aGlyphID = nsnull);
    float GetCharHeight(PRUnichar c);

    ATSFontRef GetATSFont() {
        return mATSFont;
    }

    CTFontRef GetCTFont() {
        return mCTFont;
    }

    CFDictionaryRef GetAttributesDictionary() {
        return mAttributesDict;
    }

    cairo_font_face_t *CairoFontFace() {
        return mFontFace;
    }

    cairo_scaled_font_t *CairoScaledFont() {
        return mScaledFont;
    }

    virtual nsString GetUniqueName() {
        return GetName();
    }

    virtual PRUint32 GetSpaceGlyph() {
        return mSpaceGlyph;
    }

    PRBool TestCharacterMap(PRUint32 aCh);

    MacOSFontEntry* GetFontEntry();

    PRBool Valid() {
        return mIsValid;
    }

    
    static void Shutdown();

    static CTFontRef CreateCTFontWithDisabledLigatures(ATSFontRef aFont, CGFloat aSize);

protected:
    const gfxFontStyle *mFontStyle;

    ATSFontRef mATSFont;
    CTFontRef mCTFont;
    CFDictionaryRef mAttributesDict;

    PRBool mHasMetrics;

    nsString mUniqueName;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics mMetrics;

    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;    

    void InitMetrics();

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    static void CreateDefaultFeaturesDescriptor();

    static CTFontDescriptorRef GetDefaultFeaturesDescriptor() {
        if (sDefaultFeaturesDescriptor == NULL)
            CreateDefaultFeaturesDescriptor();
        return sDefaultFeaturesDescriptor;
    }

    
    static CTFontDescriptorRef    sDefaultFeaturesDescriptor;
    
    static CTFontDescriptorRef    sDisableLigaturesDescriptor;
};

class THEBES_API gfxCoreTextFontGroup : public gfxFontGroup {
public:
    gfxCoreTextFontGroup(const nsAString& families,
                         const gfxFontStyle *aStyle,
                         gfxUserFontSet *aUserFontSet);
    virtual ~gfxCoreTextFontGroup() {};

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    
    
    
    
    
    
    void MakeTextRunInternal(const PRUnichar *aString, PRUint32 aLength,
                             PRBool aWrapped, gfxTextRun *aTextRun);

    gfxCoreTextFont* GetFontAt(PRInt32 aFontIndex) {
        return static_cast<gfxCoreTextFont*>(static_cast<gfxFont*>(mFonts[aFontIndex]));
    }

    PRBool HasFont(ATSFontRef aFontRef);

    inline gfxCoreTextFont* WhichFontSupportsChar(nsTArray< nsRefPtr<gfxFont> >& aFontList, 
                                                  PRUint32 aCh)
    {
        PRUint32 len = aFontList.Length();
        for (PRUint32 i = 0; i < len; i++) {
            gfxCoreTextFont* font = static_cast<gfxCoreTextFont*>(aFontList.ElementAt(i).get());
            if (font->TestCharacterMap(aCh))
                return font;
        }
        return nsnull;
    }

    
    already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);

    already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    void UpdateFontList();

protected:
    static PRBool FindCTFont(const nsAString& aName,
                             const nsACString& aGenericName,
                             void *closure);

    






    void InitTextRun(gfxTextRun *aTextRun,
                     const PRUnichar *aString,
                     PRUint32 aTotalLength,
                     PRUint32 aLayoutStart,
                     PRUint32 aLayoutLength);

    nsresult SetGlyphsFromRun(gfxTextRun *aTextRun,
                              CTRunRef aCTRun,
                              const PRPackedBool *aUnmatched,
                              PRInt32 aLayoutStart,
                              PRInt32 aLayoutLength);

    
    nsRefPtr<gfxFontFamily>       mLastPrefFamily;
    nsRefPtr<gfxCoreTextFont>     mLastPrefFont;
    eFontPrefLang                 mLastPrefLang;       
    PRBool                        mLastPrefFirstFont;  
    eFontPrefLang                 mPageLang;
};

#endif 
