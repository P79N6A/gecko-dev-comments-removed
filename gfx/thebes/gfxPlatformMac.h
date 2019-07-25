




#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArray.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

#define MAC_OS_X_MAJOR_VERSION_MASK 0xFFFFFFF0U

class gfxTextRun;
class gfxFontFamily;
class mozilla::gfx::DrawTarget;

class THEBES_API gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();
    virtual ~gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxContentType contentType);
    virtual already_AddRefed<gfxASurface>
      CreateOffscreenImageSurface(const gfxIntSize& aSize,
                                  gfxASurface::gfxContentType aContentType)
    {
        nsRefPtr<gfxASurface> surface = CreateOffscreenSurface(aSize, aContentType);
#ifdef DEBUG
        nsRefPtr<gfxImageSurface> imageSurface = surface->GetAsImageSurface();
        NS_ASSERTION(imageSurface, "Surface cannot be converted to a gfxImageSurface");
#endif
        return surface;
    }
    
    already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                gfxASurface::gfxImageFormat format);
    
    mozilla::RefPtr<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(gfxFont *aFont);

    virtual bool SupportsAzure(mozilla::gfx::BackendType& aBackend);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, bool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxPlatformFontList* CreatePlatformFontList();

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    bool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    virtual void GetCommonFallbackFonts(const PRUint32 aCh,
                                        PRInt32 aRunScript,
                                        nsTArray<const char*>& aFontList);

    
    
    PRInt32 OSXVersion();

    
    PRUint32 GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

    virtual already_AddRefed<gfxASurface>
    GetThebesSurfaceForDrawTarget(mozilla::gfx::DrawTarget *aTarget);
private:
    virtual qcms_profile* GetPlatformCMSOutputProfile();
    
    
    static PRUint32 ReadAntiAliasingThreshold();    
    
    PRInt32 mOSXVersion;
    PRUint32 mFontAntiAliasingThreshold;
};

#endif 
