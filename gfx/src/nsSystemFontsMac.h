




































#ifndef _NS_SYSTEMFONTSMAC_H_
#define _NS_SYSTEMFONTSMAC_H_

#include "gfxFont.h"
#include "nsDeviceContext.h"

class nsSystemFontsMac
{
public:
    nsSystemFontsMac();
    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;
};

#endif 
