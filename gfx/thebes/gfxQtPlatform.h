




#ifndef GFX_PLATFORM_QT_H
#define GFX_PLATFORM_QT_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"
#ifdef MOZ_X11
#include "X11/Xlib.h"
#endif

class gfxImageSurface;
class gfxFontconfigUtils;
class QWindow;

class gfxQtPlatform : public gfxPlatform {
public:
    gfxQtPlatform();
    virtual ~gfxQtPlatform();

    static gfxQtPlatform *GetPlatform() {
        return static_cast<gfxQtPlatform*>(gfxPlatform::GetPlatform());
    }

    virtual already_AddRefed<gfxASurface>
    OptimizeImage(gfxImageSurface *aSurface,
                  gfxImageFormat format) MOZ_OVERRIDE;
    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) MOZ_OVERRIDE;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) MOZ_OVERRIDE;

    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts) MOZ_OVERRIDE;

    virtual nsresult UpdateFontList() MOZ_OVERRIDE;

    virtual nsresult ResolveFontName(const nsAString& aFontName,
                                     FontResolverCallback aCallback,
                                     void *aClosure, bool& aAborted) MOZ_OVERRIDE;

    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) MOZ_OVERRIDE;

    virtual gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet* aUserFontSet) MOZ_OVERRIDE;

    



    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName) MOZ_OVERRIDE;

    



    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength) MOZ_OVERRIDE;

    



    virtual bool IsFontFormatSupported(nsIURI *aFontURI,
                                       uint32_t aFormatFlags) MOZ_OVERRIDE;

    virtual void ClearPrefFonts() { mPrefFonts.Clear(); }

    static int32_t GetDPI();

    virtual gfxImageFormat GetOffscreenFormat() MOZ_OVERRIDE;
#ifdef MOZ_X11
    static Display* GetXDisplay(QWindow* aWindow = 0);
    static Screen* GetXScreen(QWindow* aWindow = 0);
#endif

    virtual int GetScreenDepth() const MOZ_OVERRIDE;

    virtual bool SupportsOffMainThreadCompositing() MOZ_OVERRIDE;

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:

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

    virtual void GetPlatformCMSOutputProfile(void *&mem, size_t &size) MOZ_OVERRIDE;

    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<gfxFontEntry> > > mPrefFonts;

    int mScreenDepth;
#ifdef MOZ_X11
    static bool sUseXRender;
#endif
};

#endif 

