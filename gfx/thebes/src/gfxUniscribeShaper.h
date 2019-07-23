







































#ifndef GFX_UNISCRIBESHAPER_H
#define GFX_UNISCRIBESHAPER_H

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxGDIFont.h"

#include <usp10.h>
#include <cairo-win32.h>


class gfxUniscribeShaper : public gfxFontShaper
{
public:
    gfxUniscribeShaper(gfxGDIFont *aFont);

    virtual ~gfxUniscribeShaper();

    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength);

    SCRIPT_CACHE *ScriptCache() { return &mScriptCache; }

    gfxGDIFont *GetFont() { return static_cast<gfxGDIFont*>(mFont); }

private:
    SCRIPT_CACHE mScriptCache;
};

#endif 
