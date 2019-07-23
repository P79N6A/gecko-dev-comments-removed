





































#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArray.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060

#define MAC_OS_X_MAJOR_VERSION_MASK 0xFFFFFFF0U

class gfxTextRun;

class THEBES_API gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();
    virtual ~gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxImageFormat imageFormat);

    already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                gfxASurface::gfxImageFormat format);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

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

    PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    
    
    PRInt32 OSXVersion();

    
    PRUint32 GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

    
    virtual void SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString);

private:
    virtual qcms_profile* GetPlatformCMSOutputProfile();
    
    
    static PRUint32 ReadAntiAliasingThreshold();    
    
    PRInt32 mOSXVersion;
    PRUint32 mFontAntiAliasingThreshold;
};

#endif 
