




































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

    virtual bool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript);
};

#endif 
