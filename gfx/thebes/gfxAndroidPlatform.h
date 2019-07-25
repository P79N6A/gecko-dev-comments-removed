





































#ifndef GFX_PLATFORM_ANDROID_H
#define GFX_PLATFORM_ANDROID_H

#include "gfxFT2Fonts.h"
#include "gfxPlatform.h"
#include "gfxUserFontSet.h"
#include "nsTArray.h"

namespace mozilla {
    namespace dom {
        class FontListEntry;
    };
};
using mozilla::dom::FontListEntry;

typedef struct FT_LibraryRec_ *FT_Library;

class THEBES_API gfxAndroidPlatform : public gfxPlatform {
public:
    gfxAndroidPlatform();
    virtual ~gfxAndroidPlatform();

    static gfxAndroidPlatform *GetPlatform() {
        return (gfxAndroidPlatform*) gfxPlatform::GetPlatform();
    }

    virtual already_AddRefed<gfxASurface>
    CreateOffscreenSurface(const gfxIntSize& size,
                           gfxASurface::gfxContentType contentType);

    virtual gfxImageFormat GetOffscreenFormat() { return gfxASurface::ImageFormatRGB16_565; }
    
    mozilla::RefPtr<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(gfxFont *aFont);

    
    void GetFontList(InfallibleTArray<FontListEntry>* retValue);

    
    virtual bool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);
    virtual gfxPlatformFontList* CreatePlatformFontList();
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData, PRUint32 aLength);

    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts);

    virtual nsresult UpdateFontList();

    virtual nsresult ResolveFontName(const nsAString& aFontName,
                                     FontResolverCallback aCallback,
                                     void *aClosure, bool& aAborted);

    virtual nsresult GetStandardFamilyName(const nsAString& aFontName,
                                           nsAString& aFamilyName);

    virtual gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet* aUserFontSet);

    FT_Library GetFTLibrary();
};

#endif 

