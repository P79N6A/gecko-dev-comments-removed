











































#include "gfxPlatformQt.h"
#include "gfxQtFonts.h"





gfxQtFontGroup::gfxQtFontGroup (const nsAString& families,
                                const gfxFontStyle *aStyle)
    : gfxFontGroup(families, aStyle)
{

}

gfxQtFontGroup::~gfxQtFontGroup()
{
}

gfxFontGroup *
gfxQtFontGroup::Copy(const gfxFontStyle *aStyle)
{
     return new gfxQtFontGroup(mFamilies, aStyle);
}

gfxTextRun *
gfxQtFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    return nsnull;
}

gfxTextRun *
gfxQtFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    return nsnull;
}





gfxQtFont::gfxQtFont(const nsAString &aName,
                           const gfxFontStyle *aFontStyle)
    : gfxFont(aName, aFontStyle)
{

}

gfxQtFont::~gfxQtFont()
{

}

 void
gfxQtFont::Shutdown()
{

}
