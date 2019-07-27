




#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArrayForwardDeclare.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace gfx {
class DrawTarget;
class VsyncSource;
} 
} 

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
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) MOZ_OVERRIDE;

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) MOZ_OVERRIDE;

    gfxFontGroup*
    CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                    const gfxFontStyle *aStyle,
                    gfxUserFontSet *aUserFontSet) MOZ_OVERRIDE;

    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic) MOZ_OVERRIDE;

    virtual gfxPlatformFontList* CreatePlatformFontList() MOZ_OVERRIDE;

    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength) MOZ_OVERRIDE;

    bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags) MOZ_OVERRIDE;

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts) MOZ_OVERRIDE;
    nsresult UpdateFontList() MOZ_OVERRIDE;

    virtual void GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList) MOZ_OVERRIDE;

    virtual bool CanRenderContentToDataSurface() const MOZ_OVERRIDE {
      return true;
    }

    bool UseAcceleratedCanvas();

    virtual bool UseProgressivePaint() MOZ_OVERRIDE;
    virtual already_AddRefed<mozilla::gfx::VsyncSource> CreateHardwareVsyncSource() MOZ_OVERRIDE;

    
    uint32_t GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

private:
    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size) MOZ_OVERRIDE;

    
    static uint32_t ReadAntiAliasingThreshold();

    uint32_t mFontAntiAliasingThreshold;
};

#endif 
