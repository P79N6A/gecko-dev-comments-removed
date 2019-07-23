





































#ifndef _NS_SYSTEMFONTSGTK2_H_
#define _NS_SYSTEMFONTSGTK2_H_

#include <gfxFont.h>

class nsSystemFontsGTK2
{
public:
    nsSystemFontsGTK2();
    ~nsSystemFontsGTK2();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;

private:

    nsresult GetSystemFontInfo(GtkWidget *aWidget, nsString *aFontName,
                               gfxFontStyle *aFontStyle) const;

    















    nsString mDefaultFontName, mButtonFontName, mFieldFontName, mMenuFontName;
    gfxFontStyle mDefaultFontStyle, mButtonFontStyle, mFieldFontStyle, mMenuFontStyle;
};

#endif 
