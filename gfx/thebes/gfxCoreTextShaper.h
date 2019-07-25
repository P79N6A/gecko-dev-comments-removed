







































#ifndef GFX_CORETEXTSHAPER_H
#define GFX_CORETEXTSHAPER_H

#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "gfxPlatform.h"
#include "gfxMacPlatformFontList.h"

#include <Carbon/Carbon.h>

class gfxMacFont;

class gfxCoreTextShaper : public gfxFontShaper {
public:
    gfxCoreTextShaper(gfxMacFont *aFont);

    virtual ~gfxCoreTextShaper();

    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript);

    
    static void Shutdown();

protected:
    CTFontRef mCTFont;
    CFDictionaryRef mAttributesDict;

    nsresult SetGlyphsFromRun(gfxTextRun *aTextRun,
                              CTRunRef aCTRun,
                              PRInt32 aStringOffset,
                              PRInt32 aLayoutStart,
                              PRInt32 aLayoutLength);

    CTFontRef CreateCTFontWithDisabledLigatures(CGFloat aSize);

    static void CreateDefaultFeaturesDescriptor();

    static CTFontDescriptorRef GetDefaultFeaturesDescriptor() {
        if (sDefaultFeaturesDescriptor == NULL) {
            CreateDefaultFeaturesDescriptor();
        }
        return sDefaultFeaturesDescriptor;
    }

    
    static CTFontDescriptorRef    sDefaultFeaturesDescriptor;

    
    static CTFontDescriptorRef    sDisableLigaturesDescriptor;
};

#endif 
