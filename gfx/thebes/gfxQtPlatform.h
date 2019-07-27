




#ifndef GFX_PLATFORM_QT_H
#define GFX_PLATFORM_QT_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"
#ifdef MOZ_X11
#include "X11/Xlib.h"
#endif

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
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) override;

    virtual already_AddRefed<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) override;

    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts) override;

    virtual nsresult UpdateFontList() override;

    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) override;

    virtual gfxFontGroup *CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet* aUserFontSet) override;

    



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

    static int32_t GetDPI();

    virtual gfxImageFormat GetOffscreenFormat() override;
#ifdef MOZ_X11
    static Display* GetXDisplay(QWindow* aWindow = 0);
    static Screen* GetXScreen(QWindow* aWindow = 0);
#endif

    virtual int GetScreenDepth() const override;

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual void GetPlatformCMSOutputProfile(void *&mem, size_t &size) override;

    int mScreenDepth;
};

#endif 

