





































#include "gfxPlatformMac.h"

#include "gfxImageSurface.h"
#include "gfxQuartzSurface.h"

#include "gfxQuartzFontCache.h"

#ifdef MOZ_ENABLE_GLITZ
#include "gfxGlitzSurface.h"
#include "glitz-agl.h"
#endif

gfxPlatformMac::gfxPlatformMac()
{
#ifdef MOZ_ENABLE_GLITZ
    if (UseGlitz())
        glitz_agl_init();
#endif
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenSurface(const gfxIntSize& size,
                                       gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *newSurface = nsnull;

    if (!UseGlitz()) {
        newSurface = new gfxQuartzSurface(size, imageFormat);
    } else {
#ifdef MOZ_ENABLE_GLITZ
        int bpp, glitzf;
        switch (imageFormat) {
            case gfxASurface::ImageFormatARGB32:
                bpp = 32;
                glitzf = 0; 
                break;
            case gfxASurface::ImageFormatRGB24:
                bpp = 24;
                glitzf = 1; 
                break;
            case gfxASurface::ImageFormatA8:
                bpp = 8;
                glitzf = 2; 
            case gfxASurface::ImageFormatA1:
                bpp = 1;
                glitzf = 3; 
                break;
            default:
                return nsnull;
        }

        
        glitz_drawable_format_t templ;
        memset(&templ, 0, sizeof(templ));
        templ.color.red_size = 8;
        templ.color.green_size = 8;
        templ.color.blue_size = 8;
        if (bpp == 32)
            templ.color.alpha_size = 8;
        else
            templ.color.alpha_size = 0;
        templ.doublebuffer = FALSE;
        templ.samples = 1;

        unsigned long mask =
            GLITZ_FORMAT_RED_SIZE_MASK |
            GLITZ_FORMAT_GREEN_SIZE_MASK |
            GLITZ_FORMAT_BLUE_SIZE_MASK |
            GLITZ_FORMAT_ALPHA_SIZE_MASK |
            GLITZ_FORMAT_SAMPLES_MASK |
            GLITZ_FORMAT_DOUBLEBUFFER_MASK;

        glitz_drawable_format_t *gdformat =
            glitz_agl_find_pbuffer_format(mask, &templ, 0);

        glitz_drawable_t *gdraw =
            glitz_agl_create_pbuffer_drawable(gdformat, width, height);

        glitz_format_t *gformat =
            glitz_find_standard_format(gdraw, (glitz_format_name_t) glitzf);

        glitz_surface_t *gsurf =
            glitz_surface_create(gdraw,
                                 gformat,
                                 width,
                                 height,
                                 0,
                                 NULL);

        glitz_surface_attach(gsurf, gdraw, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);

        newSurface = new gfxGlitzSurface(gdraw, gsurf, PR_TRUE);
#endif
    }

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

nsresult
gfxPlatformMac::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure, PRBool& aAborted)
{
    nsAutoString resolvedName;
    if (!gfxQuartzFontCache::SharedFontCache()->
             ResolveFontName(aFontName, resolvedName)) {
        aAborted = PR_FALSE;
        return NS_OK;
    }
    aAborted = !(*aCallback)(resolvedName, aClosure);
    return NS_OK;
}

nsresult
gfxPlatformMac::GetFontList(const nsACString& aLangGroup,
                            const nsACString& aGenericFamily,
                            nsStringArray& aListOfFonts)
{
    gfxQuartzFontCache::SharedFontCache()->
        GetFontList(aLangGroup, aGenericFamily, aListOfFonts);
    return NS_OK;
}

nsresult
gfxPlatformMac::UpdateFontList()
{
    gfxQuartzFontCache::SharedFontCache()->UpdateFontList();
    return NS_OK;
}
