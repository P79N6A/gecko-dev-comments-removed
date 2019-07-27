




#ifndef GFX_FONTMISSINGGLYPHS_H
#define GFX_FONTMISSINGGLYPHS_H

#include "mozilla/Attributes.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace gfx {
class DrawTarget;
class Pattern;
}
}





class gfxFontMissingGlyphs MOZ_FINAL
{
    typedef mozilla::gfx::DrawTarget DrawTarget;
    typedef mozilla::gfx::Float Float;
    typedef mozilla::gfx::Pattern Pattern;
    typedef mozilla::gfx::Rect Rect;

    gfxFontMissingGlyphs() MOZ_DELETE; 

public:
    








    static void DrawMissingGlyph(uint32_t aChar,
                                 const Rect& aRect,
                                 DrawTarget& aDrawTarget,
                                 const Pattern& aPattern,
                                 uint32_t aAppUnitsPerDevPixel);
    



    static Float GetDesiredMinWidth(uint32_t aChar,
                                    uint32_t aAppUnitsPerDevUnit);
};

#endif
