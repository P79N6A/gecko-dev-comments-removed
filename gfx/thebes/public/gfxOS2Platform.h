




































#ifndef GFX_OS2_PLATFORM_H
#define GFX_OS2_PLATFORM_H

#define INCL_GPIBITMAPS
#include <os2.h>

#include "gfxPlatform.h"

class THEBES_API gfxOS2Platform : public gfxPlatform {

public:
    gfxOS2Platform();
    virtual ~gfxOS2Platform();

    static gfxOS2Platform *GetPlatform() {
        return (gfxOS2Platform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface>
        CreateOffscreenSurface(const gfxIntSize& size,
                               gfxASurface::gfxImageFormat imageFormat);

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle);

private:
    HDC mDC;
    HPS mPS;
    HBITMAP mBitmap;
};

#endif 
