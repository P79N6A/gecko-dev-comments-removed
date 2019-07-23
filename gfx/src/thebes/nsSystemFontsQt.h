





































#ifndef _NS_SYSTEMFONTSQT_H_
#define _NS_SYSTEMFONTSQT_H_

#include <gfxFont.h>

class QFont;

class nsSystemFontsQt
{
public:
    nsSystemFontsQt();
    ~nsSystemFontsQt();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;

private:

    nsresult GetSystemFontInfo(const QFont &aFont, nsString *aFontName,
                               gfxFontStyle *aFontStyle) const;

    















    nsString mDefaultFontName, mButtonFontName, mFieldFontName, mMenuFontName;
    gfxFontStyle mDefaultFontStyle, mButtonFontStyle, mFieldFontStyle, mMenuFontStyle;
};

#endif 

