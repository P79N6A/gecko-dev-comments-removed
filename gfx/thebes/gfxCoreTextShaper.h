




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

    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText);

    
    static void Shutdown();

protected:
    CTFontRef mCTFont;
    CFDictionaryRef mAttributesDict;

    nsresult SetGlyphsFromRun(gfxShapedWord *aShapedWord,
                              CTRunRef aCTRun,
                              PRInt32 aStringOffset);

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
