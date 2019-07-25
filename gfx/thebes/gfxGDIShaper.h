




































#ifndef GFX_GDISHAPER_H
#define GFX_GDISHAPER_H

#include "gfxGDIFont.h"

class gfxGDIShaper : public gfxFontShaper
{
public:
    gfxGDIShaper(gfxGDIFont *aFont)
        : gfxFontShaper(aFont)
    {
        MOZ_COUNT_CTOR(gfxGDIShaper);
    }

    virtual ~gfxGDIShaper()
    {
        MOZ_COUNT_DTOR(gfxGDIShaper);
    }

    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aString);
};

#endif 
