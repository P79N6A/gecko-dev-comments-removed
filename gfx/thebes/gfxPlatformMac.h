




#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArrayForwardDeclare.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

#define MAC_OS_X_MAJOR_VERSION_MASK 0xFFFFFFF0U

namespace mozilla { namespace gfx { class DrawTarget; }}

class gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();
    virtual ~gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) MOZ_OVERRIDE;

    mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup*
    CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                    const gfxFontStyle *aStyle,
                    gfxUserFontSet *aUserFontSet);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxPlatformFontList* CreatePlatformFontList();

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    virtual void GetCommonFallbackFonts(const uint32_t aCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList);

    bool UseAcceleratedCanvas();

    
    uint32_t GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

private:
    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size);

    
    static uint32_t ReadAntiAliasingThreshold();

    uint32_t mFontAntiAliasingThreshold;
};

#endif 
