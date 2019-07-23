





































#ifndef GFX_WINDOWS_PLATFORM_H
#define GFX_WINDOWS_PLATFORM_H

#include "gfxFontUtils.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsFonts.h"
#include "gfxPlatform.h"

#include "nsVoidArray.h"
#include "nsDataHashtable.h"

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

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);

    nsresult UpdateFontList();

    void GetFontFamilyList(nsTArray<nsRefPtr<FontFamily> >& aFamilyArray);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle);

    






    FontEntry *FindFontForChar(PRUint32 aCh, gfxWindowsFont *aFont);

    
    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle);

    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> > *array);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> >& array);

    typedef nsDataHashtable<nsStringHashKey, nsRefPtr<FontFamily> > FontTable;

private:
    void Init();

    void InitBadUnderlineList();

    static int CALLBACK FontEnumProc(const ENUMLOGFONTEXW *lpelfe,
                                     const NEWTEXTMETRICEXW *metrics,
                                     DWORD fontType, LPARAM data);
    static int CALLBACK FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                            const NEWTEXTMETRICEXW *nmetrics,
                                            DWORD fontType, LPARAM data);

    static PLDHashOperator PR_CALLBACK FontGetStylesProc(nsStringHashKey::KeyType aKey,
                                                         nsRefPtr<FontFamily>& aFontFamily,
                                                         void* userArg);

    static PLDHashOperator PR_CALLBACK FontGetCMapDataProc(nsStringHashKey::KeyType aKey,
                                                           nsRefPtr<FontFamily>& aFontFamily,
                                                           void* userArg);

    static int CALLBACK FontResolveProc(const ENUMLOGFONTEXW *lpelfe,
                                        const NEWTEXTMETRICEXW *metrics,
                                        DWORD fontType, LPARAM data);

    static PLDHashOperator PR_CALLBACK HashEnumFunc(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<FontFamily>& aData,
                                                    void* userArg);

    static PLDHashOperator PR_CALLBACK FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                                             nsRefPtr<FontFamily>& aFontFamily,
                                                             void* userArg);

    virtual cmsHPROFILE GetPlatformCMSOutputProfile();

    static int PR_CALLBACK PrefChangedCallback(const char*, void*);

    
    virtual void InitLoader();
    virtual PRBool RunLoader();
    virtual void FinishLoader();

    FontTable mFonts;
    FontTable mFontAliases;
    FontTable mFontSubstitutes;
    nsStringArray mNonExistingFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;
    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<FontEntry> > > mPrefFonts;

    
    nsTArray<nsRefPtr<FontFamily> > mFontFamilies;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
};

#endif 
