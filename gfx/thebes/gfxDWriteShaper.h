




#ifndef GFX_DWRITESHAPER_H
#define GFX_DWRITESHAPER_H

#include "gfxDWriteFonts.h"




class gfxDWriteShaper : public gfxFontShaper
{
public:
    gfxDWriteShaper(gfxDWriteFont *aFont)
        : gfxFontShaper(aFont)
    {
        MOZ_COUNT_CTOR(gfxDWriteShaper);
    }

    virtual ~gfxDWriteShaper()
    {
        MOZ_COUNT_DTOR(gfxDWriteShaper);
    }

    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);
};

#endif 
