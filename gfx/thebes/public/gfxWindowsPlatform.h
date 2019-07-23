





































#ifndef GFX_WINDOWS_PLATFORM_H
#define GFX_WINDOWS_PLATFORM_H

#include "gfxWindowsSurface.h"
#include "gfxWindowsFonts.h"
#include "gfxPlatform.h"

#include "nsVoidArray.h"
#include "nsDataHashtable.h"

#include <windows.h>

class THEBES_API gfxWindowsPlatform : public gfxPlatform {
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

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle);

    






    FontEntry *FindFontForChar(PRUint32 aCh, gfxWindowsFont *aFont);

    
    FontEntry *FindFontEntry(const nsAString& aName);

    PRBool GetPrefFontEntries(const char *aLangGroup, nsTArray<nsRefPtr<FontEntry> > *array);
    void SetPrefFontEntries(const char *aLangGroup, nsTArray<nsRefPtr<FontEntry> >& array);

private:
    void Init();

    static int CALLBACK FontEnumProc(const ENUMLOGFONTEXW *lpelfe,
                                     const NEWTEXTMETRICEXW *metrics,
                                     DWORD fontType, LPARAM data);

    static PLDHashOperator PR_CALLBACK FontGetCMapDataProc(nsStringHashKey::KeyType aKey,
                                                           nsRefPtr<FontEntry>& aFontEntry,
                                                           void* userArg);

    static int CALLBACK FontResolveProc(const ENUMLOGFONTEXW *lpelfe,
                                        const NEWTEXTMETRICEXW *metrics,
                                        DWORD fontType, LPARAM data);

    static PLDHashOperator PR_CALLBACK HashEnumFunc(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<FontEntry>& aData,
                                                    void* userArg);

    static PLDHashOperator PR_CALLBACK FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                                             nsRefPtr<FontEntry>& aFontEntry,
                                                             void* userArg);

    virtual cmsHPROFILE GetPlatformCMSOutputProfile();

    static int PR_CALLBACK PrefChangedCallback(const char*, void*);

    nsDataHashtable<nsStringHashKey, nsRefPtr<FontEntry> > mFonts;
    nsDataHashtable<nsStringHashKey, nsRefPtr<FontEntry> > mFontAliases;
    nsDataHashtable<nsStringHashKey, nsRefPtr<FontEntry> > mFontSubstitutes;
    nsStringArray mNonExistingFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;
    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<FontEntry> > > mPrefFonts;
};

#endif 
