





































#ifndef _NS_SYSTEMFONTSWIN_H_
#define _NS_SYSTEMFONTSWIN_H_

#include "gfxFont.h"
#include "nsDeviceContext.h"
#include <windows.h> 

class nsSystemFontsWin
{
public:
    nsSystemFontsWin();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;
private:
    nsresult CopyLogFontToNSFont(HDC* aHDC, const LOGFONTW* ptrLogFont,
                                 nsString *aFontName, gfxFontStyle *aFontStyle) const;
    nsresult GetSysFontInfo(HDC aHDC, nsSystemFontID anID,
                            nsString *aFontName,
                            gfxFontStyle *aFontStyle) const;
};

#endif 
