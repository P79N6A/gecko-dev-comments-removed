





































#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"

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
                         nsStringArray& aListOfFonts);

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
                                           nsISupports *aLoader,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    



    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI,
                                         PRUint32 aFormatFlags);
#endif

#ifndef MOZ_PANGO
    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aFamilyName, const gfxFontStyle& aFontStyle);
#endif

    static double DPI() {
        if (sDPI < 0.0) {
            InitDPI();
        }
        NS_ASSERTION(sDPI > 0.0, "Something is wrong");
        return sDPI;
    }

#ifndef MOZ_PANGO
    FT_Library GetFTLibrary();
#endif

    void SetGdkDrawable(gfxASurface *target,
                        GdkDrawable *drawable);
    GdkDrawable *GetGdkDrawable(gfxASurface *target);

protected:
    static void InitDPI();

    static double sDPI;
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
};

#endif 
