







































#ifndef GFX_CORETEXTFONTS_H
#define GFX_CORETEXTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

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

    
    static void Shutdown();

    static CTFontRef CreateCTFontWithDisabledLigatures(ATSFontRef aFontRef, CGFloat aSize);

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

    virtual void InitTextRun(gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength);

    nsresult SetGlyphsFromRun(gfxTextRun *aTextRun,
                              CTRunRef aCTRun,
                              PRInt32 aStringOffset,
                              PRInt32 aLayoutStart,
                              PRInt32 aLayoutLength);

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

#endif 
