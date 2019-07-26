




#ifndef GFX_FONTMISSINGGLYPHS_H
#define GFX_FONTMISSINGGLYPHS_H

#include "gfxTypes.h"
#include "gfxRect.h"

class gfxContext;





class gfxFontMissingGlyphs {
public:
    







    static void DrawMissingGlyph(gfxContext    *aContext,
                                 const gfxRect& aRect,
                                 uint32_t       aChar,
                                 uint32_t       aAppUnitsPerDevPixel);
    



    static gfxFloat GetDesiredMinWidth(uint32_t aChar,
                                       uint32_t aAppUnitsPerDevUnit);
};

#endif
