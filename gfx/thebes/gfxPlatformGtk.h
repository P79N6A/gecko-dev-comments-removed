




#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"
#include "nsTArray.h"

#if (MOZ_WIDGET_GTK == 2)
extern "C" {
    typedef struct _GdkDrawable GdkDrawable;
}
#endif

class gfxFontconfigUtils;

class gfxPlatformGtk : public gfxPlatform {
public:
    gfxPlatformGtk();
    virtual ~gfxPlatformGtk();

    static gfxPlatformGtk *GetPlatform() {
        return (gfxPlatformGtk*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxContentType contentType);

    mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, bool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    



    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    



    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    



    virtual bool IsFontFormatSupported(nsIURI *aFontURI,
                                         uint32_t aFormatFlags);

#if (MOZ_WIDGET_GTK == 2)
    static void SetGdkDrawable(cairo_surface_t *target,
                               GdkDrawable *drawable);
    static GdkDrawable *GetGdkDrawable(cairo_surface_t *target);
#endif

    static int32_t GetDPI();

    bool UseXRender() {
#if defined(MOZ_X11)
        if (GetContentBackend() != mozilla::gfx::BACKEND_NONE &&
            GetContentBackend() != mozilla::gfx::BACKEND_CAIRO)
            return false;

        return sUseXRender;
#else
        return false;
#endif
    }

    virtual gfxImageFormat GetOffscreenFormat();

    virtual int GetScreenDepth() const;

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual qcms_profile *GetPlatformCMSOutputProfile();

    virtual bool SupportsOffMainThreadCompositing();
#ifdef MOZ_X11
    static bool sUseXRender;
#endif
};

#endif 
