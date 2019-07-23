







































#ifndef GFX_FT2FONTLIST_H
#define GFX_FT2FONTLIST_H

#include "gfxWindowsPlatform.h"
#include "gfxPlatformFontList.h"

#include <windows.h>
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
    virtual void InitFontList();

    void AppendFacesFromFontFile(const PRUnichar *aFileName);
    void FindFonts();
};

#endif 
