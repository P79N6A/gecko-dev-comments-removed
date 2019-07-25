







































#ifndef GFX_FT2FONTLIST_H
#define GFX_FT2FONTLIST_H

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#include <windows.h>
#endif
#include "gfxPlatformFontList.h"

#include <bitset>

class gfxFT2FontList : public gfxPlatformFontList
{
public:
    gfxFT2FontList();

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         PRBool& aNeedsBold);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

protected:
    virtual nsresult InitFontList();

    void AppendFacesFromFontFile(const PRUnichar *aFileName);
    void AppendFacesFromFontFile(const char *aFileName);
    void FindFonts();
};

#endif 
