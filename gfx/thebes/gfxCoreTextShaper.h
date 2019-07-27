




#ifndef GFX_CORETEXTSHAPER_H
#define GFX_CORETEXTSHAPER_H

#include "gfxFont.h"

#include <ApplicationServices/ApplicationServices.h>

class gfxMacFont;

class gfxCoreTextShaper : public gfxFontShaper {
public:
    explicit gfxCoreTextShaper(gfxMacFont *aFont);

    virtual ~gfxCoreTextShaper();

    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
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
        if (sDefaultFeaturesDescriptor == nullptr) {
            CreateDefaultFeaturesDescriptor();
        }
        return sDefaultFeaturesDescriptor;
    }

    
    static CTFontDescriptorRef    sDefaultFeaturesDescriptor;

    
    static CTFontDescriptorRef    sDisableLigaturesDescriptor;
};

#endif 
