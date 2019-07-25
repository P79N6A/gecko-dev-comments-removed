







































#ifndef _NS_SYSTEMFONTSOS2_H_
#define _NS_SYSTEMFONTSOS2_H_

#include "gfxFont.h"
#include "nsDeviceContext.h"

class nsSystemFontsOS2
{
public:
    nsSystemFontsOS2();
    nsresult GetSystemFont(nsSystemFontID aID, nsString* aFontName,
                           gfxFontStyle *aFontStyle) const;
};

#endif 
