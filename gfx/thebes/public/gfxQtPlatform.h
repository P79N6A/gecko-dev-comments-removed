





































#ifndef GFX_PLATFORM_QT_H
#define GFX_PLATFORM_QT_H

#include "gfxPlatform.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"

typedef struct FT_LibraryRec_ *FT_Library;

class gfxFontconfigUtils;
class FontFamily;
class FontEntry;

class THEBES_API gfxQtPlatform : public gfxPlatform {
public:
    gfxQtPlatform();
    virtual ~gfxQtPlatform();

    static gfxQtPlatform *GetPlatform() {
        return (gfxQtPlatform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxImageFormat imageFormat);

    nsresult GetFontList(const nsACString& aLangGroup,
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
    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> > *aFontEntryList);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> >& aFontEntryList);

    static PRInt32 DPI() {
        if (sDPI == -1) {
            InitDPI();
        }
        NS_ASSERTION(sDPI > 0, "Something is wrong");
        return sDPI;
    }

    FT_Library GetFTLibrary();

protected:
    static void InitDPI();

    static PRInt32 sDPI;
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual qcms_profile *GetPlatformCMSOutputProfile();
};

#endif 

