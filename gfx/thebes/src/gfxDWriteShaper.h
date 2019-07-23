




































#ifndef GFX_DWRITESHAPER_H
#define GFX_DWRITESHAPER_H

#include "gfxDWriteFonts.h"




class gfxDWriteShaper : public gfxFontShaper
{
public:
    gfxDWriteShaper(gfxDWriteFont *aFont)
        : gfxFontShaper(aFont)
    { }

    virtual ~gfxDWriteShaper() { }

    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength);
};

#endif 
