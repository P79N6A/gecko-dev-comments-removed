




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
                             gfxContentType contentType) override;

    mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) override;

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) override;

    gfxFontGroup*
    CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                    const gfxFontStyle *aStyle,
                    gfxUserFontSet *aUserFontSet) override;

    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic) override;

    virtual gfxPlatformFontList* CreatePlatformFontList() override;

    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength) override;

    bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags) override;

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts) override;
    nsresult UpdateFontList() override;

    virtual void GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList) override;

    virtual bool CanRenderContentToDataSurface() const override {
      return true;
    }

    virtual bool SupportsApzWheelInput() const override {
      return true;
    }

    bool UseAcceleratedCanvas();

    virtual bool UseProgressivePaint() override;
    virtual already_AddRefed<mozilla::gfx::VsyncSource> CreateHardwareVsyncSource() override;

    
    uint32_t GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

private:
    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size) override;

    
    static uint32_t ReadAntiAliasingThreshold();

    uint32_t mFontAntiAliasingThreshold;
};

#endif 
