






































#ifndef GFX_PLATFORM_BEOS_H
#define GFX_PLATFORM_BEOS_H

#include "gfxPlatform.h"

class gfxFontconfigUtils;

class NS_EXPORT gfxBeOSPlatform : public gfxPlatform {
public:
    gfxBeOSPlatform();
    virtual ~gfxBeOSPlatform();

    static gfxBeOSPlatform *GetPlatform() {
        return (gfxBeOSPlatform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface>
        CreateOffscreenSurface(PRUint32 width,
                               PRUint32 height,
                               gfxASurface::gfxImageFormat imageFormat);

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);
protected:
    static gfxFontconfigUtils *sFontconfigUtils;
};

#endif 
