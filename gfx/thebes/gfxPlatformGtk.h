




#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include "gfxPlatform.h"
#include "gfxPrefs.h"
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

    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) override;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) override;

    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts) override;

    virtual nsresult UpdateFontList() override;

    virtual nsresult GetStandardFamilyName(const nsAString& aFontName,
                                           nsAString& aFamilyName) override;

    virtual gfxFontGroup* CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet *aUserFontSet) override;

    



    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic) override;

    



    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength) override;

    



    virtual bool IsFontFormatSupported(nsIURI *aFontURI,
                                         uint32_t aFormatFlags) override;

#if (MOZ_WIDGET_GTK == 2)
    static void SetGdkDrawable(cairo_surface_t *target,
                               GdkDrawable *drawable);
    static GdkDrawable *GetGdkDrawable(cairo_surface_t *target);
#endif

    static int32_t GetDPI();
    static double  GetDPIScale();

    bool UseXRender() {
#if defined(MOZ_X11)
        if (GetContentBackend() != mozilla::gfx::BackendType::NONE &&
            GetContentBackend() != mozilla::gfx::BackendType::CAIRO)
            return false;

        return sUseXRender;
#else
        return false;
#endif
    }

    bool UseImageOffscreenSurfaces() {
        
        
#if (MOZ_WIDGET_GTK == 3)
        return gfxPrefs::UseImageOffscreenSurfaces();
#else
        return false;
#endif
    }

    virtual gfxImageFormat GetOffscreenFormat() override;

    virtual int GetScreenDepth() const override;

    bool SupportsApzWheelInput() const override {
      return true;
    }

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual void GetPlatformCMSOutputProfile(void *&mem,
                                             size_t &size) override;

#ifdef MOZ_X11
    static bool sUseXRender;
#endif
};

#endif 
