




































#include "gfxBeOSPlatform.h"
#include "gfxFontconfigUtils.h"
#include "gfxPangoFonts.h"

#include "gfxImageSurface.h"
#include "gfxBeOSSurface.h"

#include "nsTArray.h"

gfxFontconfigUtils *gfxPlatformGtk::sFontconfigUtils = nsnull;

gfxBeOSPlatform::gfxBeOSPlatform()
{
    if (!sFontconfigUtils)
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();
}

gfxBeOSPlatform::~gfxBeOSPlatform()
{
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nsnull;

    gfxPangoFontGroup::Shutdown();

#if 0
    
    
    
    
    FcFini();
#endif
}

already_AddRefed<gfxASurface>
gfxBeOSPlatform::CreateOffscreenSurface (PRUint32 width,
                                         PRUint32 height,
                                         gfxASurface::gfxContentType contentType)
{
    gfxASurface *newSurface = nsnull;

    if (contentType == gfxASurface::CONTENT_ALPHA) {
        newSurface = new gfxImageSurface(imageFormat, width, height);
    } else {
        newSurface = new gfxBeOSSurface(width, height,
                                        contentType == gfxASurface::CONTENT_COLOR_ALPHA ? B_RGBA32 : B_RGB32);
    }

    NS_ADDREF(newSurface);
    return newSurface;
}

nsresult
gfxBeOSPlatform::GetFontList(nsIAtom *aLangGroup,
                             const nsACString& aGenericFamily,
                             nsTArray<nsString>& aListOfFonts)
{
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
}

nsresult
gfxBeOSPlatform::UpdateFontList()
{
    return sFontconfigUtils->UpdateFontList();
}

nsresult
gfxBeOSPlatform::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure,
                                PRBool& aAborted)
{
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback,
                                             aClosure, aAborted);
}

nsresult
gfxBeOSPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    return sFontconfigUtils->GetStandardFamilyName(aFontName, aFamilyName);
}
