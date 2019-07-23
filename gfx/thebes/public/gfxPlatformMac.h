





































#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArray.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050

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
                                           nsISupports *aLoader,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    
    void GetLangPrefs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang);
    
    
    
    PRInt32 OSXVersion();

    
    PRUint32 GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

    
    PRBool UsingCoreText()
#ifdef __LP64__
        { return PR_TRUE; }
#else
        { return mUseCoreText; }
#endif

    
    static void SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString);

    
    static eFontPrefLang GetFontPrefLangFor(PRUint8 aUnicodeRange);

private:
    void AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, 
                            eFontPrefLang aCharLang, eFontPrefLang aPageLang);
                                               
    virtual qcms_profile* GetPlatformCMSOutputProfile();
    
    
    static PRUint32 ReadAntiAliasingThreshold();    
    
    nsTArray<PRUint32> mCJKPrefLangs;
    PRInt32 mOSXVersion;
    PRUint32 mFontAntiAliasingThreshold;

#ifndef __LP64__
    
    
    PRBool mUseCoreText;
#endif
};

#endif 
