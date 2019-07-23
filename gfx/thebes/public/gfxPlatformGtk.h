





































#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include <gdk/gdkx.h>

#include "gfxPlatform.h"

class gfxFontconfigUtils;

class THEBES_API gfxPlatformGtk : public gfxPlatform {
public:
    gfxPlatformGtk();
    virtual ~gfxPlatformGtk();

    static gfxPlatformGtk *GetPlatform() {
        return (gfxPlatformGtk*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxImageFormat imageFormat);

    GdkDrawable *GetSurfaceGdkDrawable(gfxASurface *aSurf);

    void SetSurfaceGdkWindow(gfxASurface *aSurf,
                             GdkWindow *win);

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle);

    static PRInt32 DPI() {
        if (sDPI == -1) {
            InitDPI();
        }
        NS_ASSERTION(sDPI != 0, "Something is wrong");
        return sDPI;
    }

protected:
    static void InitDPI();

    static PRInt32 sDPI;
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
};

#endif 
