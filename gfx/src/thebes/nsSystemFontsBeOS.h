



































#ifndef _NS_SYSTEMFONTSBEOS_H_
#define _NS_SYSTEMFONTSBEOS_H_

#include <gfxFont.h>

class BFont;

class nsSystemFontsBeOS
{
public:
    nsSystemFontsBeOS();

    nsresult GetSystemFont(nsSystemFontID anID, nsString *aFontName,
                           gfxFontStyle *aFontStyle) const;

private:
    nsresult GetSystemFontInfo(const BFont *theFont, nsString *aFontName,
                               gfxFontStyle *aFontStyle) const;

    















    nsString mDefaultFontName, mMenuFontName, mCaptionFontName;
    gfxFontStyle mDefaultFontStyle, mMenuFontStyle, mCaptionFontStyle;
};

#endif 
