




#ifndef GFX_FONTMISSINGGLYPHS_H
#define GFX_FONTMISSINGGLYPHS_H

#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxRect.h"





class THEBES_API gfxFontMissingGlyphs {
public:
    







    static void DrawMissingGlyph(gfxContext    *aContext,
                                 const gfxRect& aRect,
                                 uint32_t       aChar,
                                 uint32_t       aAppUnitsPerDevPixel);
    



    static gfxFloat GetDesiredMinWidth(uint32_t aChar,
                                       uint32_t aAppUnitsPerDevUnit);
};

#endif
