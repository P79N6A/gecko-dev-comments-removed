







































#ifndef _NS_SYSTEMFONTSOS2_H_
#define _NS_SYSTEMFONTSOS2_H_

#include <gfxFont.h>
#define INCL_WINWINDOWMGR
#define INCL_WINSHELLDATA
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>

class nsSystemFontsOS2
{
public:
    nsSystemFontsOS2();
    nsresult GetSystemFont(nsSystemFontID aID, nsString* aFontName,
                           gfxFontStyle *aFontStyle) const;
};

#endif 
