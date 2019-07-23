




































#include "gfxBeOSPlatform.h"
#include "gfxFontconfigUtils.h"
#include "gfxPangoFonts.h"

#include "gfxImageSurface.h"
#include "gfxBeOSSurface.h"

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

    gfxPangoFont::Shutdown();

#if 0
    
    
    
    
    FcFini();
#endif
}

already_AddRefed<gfxASurface>
gfxBeOSPlatform::CreateOffscreenSurface (PRUint32 width,
                                         PRUint32 height,
                                         gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *newSurface = nsnull;

    if (imageFormat == gfxASurface::ImageFormatA1 ||
        imageFormat == gfxASurface::ImageFormatA8) {
        newSurface = new gfxImageSurface(imageFormat, width, height);
    } else {
        newSurface = new gfxBeOSSurface(width, height,
                                        imageFormat == gfxASurface::ImageFormatARGB32 ? B_RGBA32 : B_RGB32);
    }

    NS_ADDREF(newSurface);
    return newSurface;
}

nsresult
gfxBeOSPlatform::GetFontList(const nsACString& aLangGroup,
                             const nsACString& aGenericFamily,
                             nsStringArray& aListOfFonts)
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
