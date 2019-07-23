




































#ifndef GFX_FONTMISSINGGLYPHS_H
#define GFX_FONTMISSINGGLYPHS_H

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxRect.h"





class THEBES_API gfxFontMissingGlyphs {
public:
    





    static void DrawMissingGlyph(gfxContext *aContext, const gfxRect& aRect,
                                 PRUnichar aChar);
    



    static gfxFloat GetDesiredMinWidth();
};

#endif
