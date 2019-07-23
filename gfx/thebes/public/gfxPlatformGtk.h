





































#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"
#include "nsTArray.h"

extern "C" {
    typedef struct _GdkDrawable GdkDrawable;
}

class gfxFontconfigUtils;
#ifndef MOZ_PANGO
class FontFamily;
class FontEntry;
typedef struct FT_LibraryRec_ *FT_Library;
#endif

template <class T>
class gfxGObjectRefTraits : public nsPointerRefTraits<T> {
public:
    static void Release(T *aPtr) { g_object_unref(aPtr); }
    static void AddRef(T *aPtr) { g_object_ref(aPtr); }
};

class THEBES_API gfxPlatformGtk : public gfxPlatform {
public:
    gfxPlatformGtk();
    virtual ~gfxPlatformGtk();

    static gfxPlatformGtk *GetPlatform() {
        return (gfxPlatformGtk*) gfxPlatform::GetPlatform();
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
                                  gfxUserFontSet *aUserFontSet);

#ifdef MOZ_PANGO
    



    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    



    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    



    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI,
                                         PRUint32 aFormatFlags);
#endif

#ifndef MOZ_PANGO
    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aFamilyName, const gfxFontStyle& aFontStyle);
    already_AddRefed<gfxFont> FindFontForChar(PRUint32 aCh, gfxFont *aFont);
    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> > *aFontEntryList);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<FontEntry> >& aFontEntryList);
#endif

#ifndef MOZ_PANGO
    FT_Library GetFTLibrary();
#endif

    void SetGdkDrawable(gfxASurface *target,
                        GdkDrawable *drawable);
    GdkDrawable *GetGdkDrawable(gfxASurface *target);

    static PRInt32 GetPlatformDPI() {
        if (sPlatformDPI < 0) {
            gfxPlatformGtk::GetPlatform()->InitDisplayCaps();
        }
        NS_ASSERTION(sPlatformDPI > 0, "Something is wrong");
        return sPlatformDPI;
    }

protected:
    void InitDisplayCaps();

    static PRInt32 sPlatformDPI;
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual qcms_profile *GetPlatformCMSOutputProfile();
};

#endif 
