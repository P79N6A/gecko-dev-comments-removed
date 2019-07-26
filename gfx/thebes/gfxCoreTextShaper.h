




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

    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    
    static void Shutdown();

protected:
    CTFontRef mCTFont;
    CFDictionaryRef mAttributesDict;

    nsresult SetGlyphsFromRun(gfxShapedText *aShapedText,
                              uint32_t       aOffset,
                              uint32_t       aLength,
                              CTRunRef       aCTRun,
                              int32_t        aStringOffset);

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
