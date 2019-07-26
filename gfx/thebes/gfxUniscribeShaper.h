




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
    gfxUniscribeShaper(gfxGDIFont *aFont)
        : gfxFontShaper(aFont)
        , mScriptCache(NULL)
    {
        MOZ_COUNT_CTOR(gfxUniscribeShaper);
    }

    virtual ~gfxUniscribeShaper()
    {
        MOZ_COUNT_DTOR(gfxUniscribeShaper);
    }

    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    SCRIPT_CACHE *ScriptCache() { return &mScriptCache; }

    gfxGDIFont *GetFont() { return static_cast<gfxGDIFont*>(mFont); }

private:
    SCRIPT_CACHE mScriptCache;

    enum {
        kUnicodeVS17 = gfxFontUtils::kUnicodeVS17,
        kUnicodeVS256 = gfxFontUtils::kUnicodeVS256
    };
};

#endif 
