




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
                           bool             aVertical,
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

    CTFontRef CreateCTFontWithFeatures(CGFloat aSize,
                                       CTFontDescriptorRef aDescriptor);

    static CTFontDescriptorRef
    CreateFontFeaturesDescriptor(const std::pair<SInt16,SInt16> aFeatures[],
                                 size_t aCount);

    static CTFontDescriptorRef GetDefaultFeaturesDescriptor();
    static CTFontDescriptorRef GetDisableLigaturesDescriptor();
    static CTFontDescriptorRef GetIndicFeaturesDescriptor();
    static CTFontDescriptorRef GetIndicDisableLigaturesDescriptor();

    
    static CTFontDescriptorRef    sDefaultFeaturesDescriptor;

    
    static CTFontDescriptorRef    sDisableLigaturesDescriptor;

    
    static CTFontDescriptorRef    sIndicFeaturesDescriptor;
    static CTFontDescriptorRef    sIndicDisableLigaturesDescriptor;
};

#endif 
