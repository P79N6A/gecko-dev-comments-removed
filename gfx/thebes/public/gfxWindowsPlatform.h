





































#ifndef GFX_WINDOWS_PLATFORM_H
#define GFX_WINDOWS_PLATFORM_H

#if defined(WINCE)
#define MOZ_FT2_FONTS 1
#endif

#include "gfxFontUtils.h"
#include "gfxWindowsSurface.h"
#ifdef MOZ_FT2_FONTS
#include "gfxFT2Fonts.h"
#else
#include "gfxWindowsFonts.h"
#endif
#include "gfxPlatform.h"

#include "nsTArray.h"
#include "nsDataHashtable.h"

#ifdef MOZ_FT2_FONTS
typedef struct FT_LibraryRec_ *FT_Library;
#endif

#include <windows.h>

class THEBES_API gfxWindowsPlatform : public gfxPlatform, private gfxFontInfoLoader {
public:
    gfxWindowsPlatform();
    virtual ~gfxWindowsPlatform();
    static gfxWindowsPlatform *GetPlatform() {
        return (gfxWindowsPlatform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxImageFormat imageFormat);

    enum RenderMode {
        
        RENDER_GDI = 0,

        
        RENDER_IMAGE_STRETCH32,

        
        RENDER_IMAGE_STRETCH24,

        
        RENDER_DDRAW,

        
        RENDER_IMAGE_DDRAW16,

        
        RENDER_DDRAW_GL,

        
        RENDER_MODE_MAX
    };

    RenderMode GetRenderMode() { return mRenderMode; }
    void SetRenderMode(RenderMode rmode) { mRenderMode = rmode; }

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    void GetFontFamilyList(nsTArray<nsRefPtr<FontFamily> >& aFamilyArray);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    


    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    


    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    


    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);

    






    already_AddRefed<gfxFont>
    FindFontForChar(PRUint32 aCh, gfxFont *aFont);

    
    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle);

    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> > *array);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> >& array);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    typedef nsDataHashtable<nsStringHashKey, nsRefPtr<FontFamily> > FontTable;

#ifdef MOZ_FT2_FONTS
    FT_Library GetFTLibrary();
private:
    void AppendFacesFromFontFile(const PRUnichar *aFileName);
    void FindFonts();
#endif

protected:
    void InitDisplayCaps();

    RenderMode mRenderMode;

private:
    void Init();

    void InitBadUnderlineList();

    static int CALLBACK FontEnumProc(const ENUMLOGFONTEXW *lpelfe,
                                     const NEWTEXTMETRICEXW *metrics,
                                     DWORD fontType, LPARAM data);
    static int CALLBACK FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                            const NEWTEXTMETRICEXW *nmetrics,
                                            DWORD fontType, LPARAM data);

    static PLDHashOperator FontGetStylesProc(nsStringHashKey::KeyType aKey,
                                             nsRefPtr<FontFamily>& aFontFamily,
                                             void* userArg);

    static PLDHashOperator FontGetCMapDataProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<FontFamily>& aFontFamily,
                                               void* userArg);

    static int CALLBACK FontResolveProc(const ENUMLOGFONTEXW *lpelfe,
                                        const NEWTEXTMETRICEXW *metrics,
                                        DWORD fontType, LPARAM data);

    static PLDHashOperator HashEnumFunc(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontFamily>& aData,
                                        void* userArg);

    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<FontFamily>& aFontFamily,
                                               void* userArg);

    virtual qcms_profile* GetPlatformCMSOutputProfile();

    static int PrefChangedCallback(const char*, void*);

    
    virtual void InitLoader();
    virtual PRBool RunLoader();
    virtual void FinishLoader();

    FontTable mFonts;
    FontTable mFontAliases;
    FontTable mFontSubstitutes;
    nsTArray<nsString> mNonExistingFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;
    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<FontEntry> > > mPrefFonts;

    
    nsTArray<nsRefPtr<FontFamily> > mFontFamilies;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
};

#endif 
