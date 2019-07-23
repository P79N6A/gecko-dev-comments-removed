





































#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArray.h"
#include "gfxPlatform.h"

class THEBES_API gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxImageFormat imageFormat);

    already_AddRefed<gfxASurface> gfxPlatformMac::OptimizeImage(gfxImageSurface *aSurface,
                                                                gfxASurface::gfxImageFormat format);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle);

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);
    nsresult UpdateFontList();

    
    void GetLangPrefs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang);
    
private:
    void gfxPlatformMac::AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, 
                                            eFontPrefLang aCharLang, eFontPrefLang aPageLang);
                                               
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
    
    nsTArray<PRUint32> mCJKPrefLangs;
};

#endif 
