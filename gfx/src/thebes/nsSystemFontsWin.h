





































#ifndef _NS_SYSTEMFONTSWIN_H_
#define _NS_SYSTEMFONTSWIN_H_

#include "gfxFont.h"
#include "nsIDeviceContext.h"

class nsSystemFontsWin
{
public:
    nsSystemFontsWin();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;
private:
    nsresult CopyLogFontToNSFont(HDC* aHDC, const LOGFONT* ptrLogFont,
                                 nsString *aFontName, gfxFontStyle *aFontStyle,
				 PRBool aIsWide = PR_FALSE) const;
    nsresult GetSysFontInfo(HDC aHDC, nsSystemFontID anID,
                            nsString *aFontName,
                            gfxFontStyle *aFontStyle) const;
};

#endif 

