




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
                             gfxContentType contentType) MOZ_OVERRIDE;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont) MOZ_OVERRIDE;

    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts) MOZ_OVERRIDE;

    virtual nsresult UpdateFontList() MOZ_OVERRIDE;

    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) MOZ_OVERRIDE;

    virtual gfxFontGroup *CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet* aUserFontSet) MOZ_OVERRIDE;

    



    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic) MOZ_OVERRIDE;

    



    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
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

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual void GetPlatformCMSOutputProfile(void *&mem, size_t &size) MOZ_OVERRIDE;

    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<gfxFontEntry> > > mPrefFonts;

    int mScreenDepth;
};

#endif 

