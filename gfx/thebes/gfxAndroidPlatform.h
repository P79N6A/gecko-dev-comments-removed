





































#ifndef GFX_PLATFORM_ANDROID_H
#define GFX_PLATFORM_ANDROID_H

#include "gfxFontUtils.h"
#include "gfxFT2Fonts.h"
#include "gfxPlatform.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"

typedef struct FT_LibraryRec_ *FT_Library;

class FontFamily;
class FontEntry;
namespace mozilla {
    namespace dom {
        class FontListEntry;
    };
};

using namespace mozilla;
using namespace dom;

class FontNameCache;

class THEBES_API gfxAndroidPlatform : public gfxPlatform {
public:
    gfxAndroidPlatform();
    virtual ~gfxAndroidPlatform();

    static gfxAndroidPlatform *GetPlatform() {
        return (gfxAndroidPlatform*) gfxPlatform::GetPlatform();
    }

    void GetFontList(InfallibleTArray<FontListEntry>* retValue);

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxContentType contentType);

    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);
    virtual gfxPlatformFontList* CreatePlatformFontList();
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                     const PRUint8 *aFontData, PRUint32 aLength);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet* aUserFontSet);

    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aFamilyName, const gfxFontStyle& aFontStyle);
    already_AddRefed<gfxFont> FindFontForChar(PRUint32 aCh, gfxFont *aFont);
    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> > *aFontEntryList);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList);

    FT_Library GetFTLibrary();

    virtual gfxImageFormat GetOffscreenFormat() { return gfxASurface::ImageFormatRGB16_565; }

protected:
    void AppendFacesFromFontFile(const char *aFileName, FontNameCache* aFontCache, InfallibleTArray<FontListEntry>* retValue);
    void FindFontsInDirectory(const nsCString& aFontsDir, FontNameCache* aFontCache);

    typedef nsDataHashtable<nsStringHashKey, nsRefPtr<FontFamily> > FontTable;

    FontTable mFonts;
    FontTable mFontAliases;
    FontTable mFontSubstitutes;
    InfallibleTArray<FontListEntry> mFontList;

    
    gfxSparseBitSet mCodepointsWithNoFonts;
    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<gfxFontEntry> > > mPrefFonts;
};

#endif 

