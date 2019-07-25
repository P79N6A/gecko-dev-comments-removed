




































#ifndef _NS_SYSTEMFONTSANDROID_H_
#define _NS_SYSTEMFONTSANDROID_H_

#include "gfxFont.h"
#include "nsDeviceContext.h"

class nsSystemFontsAndroid
{
public:
    nsSystemFontsAndroid();
    ~nsSystemFontsAndroid();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;

private:

    nsresult GetSystemFontInfo(const char *aClassName, nsString *aFontName,
                               gfxFontStyle *aFontStyle) const;

    















    nsString mDefaultFontName, mButtonFontName, mFieldFontName, mMenuFontName;
    gfxFontStyle mDefaultFontStyle, mButtonFontStyle, mFieldFontStyle, mMenuFontStyle;
};

#endif 

