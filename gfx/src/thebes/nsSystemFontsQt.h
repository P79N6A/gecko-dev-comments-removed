





































#ifndef _NS_SYSTEMFONTSQT_H_
#define _NS_SYSTEMFONTSQT_H_

#include <gfxFont.h>

class nsSystemFontsQt
{
public:
    nsSystemFontsQt();
    ~nsSystemFontsQt();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;

private:

    















    nsString mDefaultFontName, mButtonFontName, mFieldFontName, mMenuFontName;
    gfxFontStyle mDefaultFontStyle, mButtonFontStyle, mFieldFontStyle, mMenuFontStyle;
};

#endif 

