







#ifndef SKFONTHOST_FREETYPE_COMMON_H_
#define SKFONTHOST_FREETYPE_COMMON_H_

#include "SkGlyph.h"
#include "SkScalerContext.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef SK_DEBUG
    #define SkASSERT_CONTINUE(pred)                                                         \
        do {                                                                                \
            if (!(pred))                                                                    \
                SkDebugf("file %s:%d: assert failed '" #pred "'\n", __FILE__, __LINE__);    \
        } while (false)
#else
    #define SkASSERT_CONTINUE(pred)
#endif


class SkScalerContext_FreeType_Base : public SkScalerContext {
public:
    
    
    static const FT_Pos kBitmapEmboldenStrength = 1 << 6;

    SkScalerContext_FreeType_Base(const SkDescriptor *desc)
        : SkScalerContext(desc)
    {}

protected:
    void generateGlyphImage(FT_Face face, const SkGlyph& glyph, SkMaskGamma::PreBlend* maskPreBlend);
    void generateGlyphPath(FT_Face face, const SkGlyph& glyph, SkPath* path);
    void emboldenOutline(FT_Face face, FT_Outline* outline);
};

#endif 
