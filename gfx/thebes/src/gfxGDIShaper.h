




































#ifndef GFX_GDISHAPER_H
#define GFX_GDISHAPER_H

#include "gfxGDIFont.h"

class gfxGDIShaper : public gfxFontShaper
{
public:
    gfxGDIShaper(gfxGDIFont *aFont)
        : gfxFontShaper(aFont) { }

    virtual ~gfxGDIShaper() { }

    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength);
};

#endif 
