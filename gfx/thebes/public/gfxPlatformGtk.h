





































#ifndef GFX_PLATFORM_GTK_H
#define GFX_PLATFORM_GTK_H

#include "gfxPlatform.h"

extern "C" {
    typedef struct _GdkDrawable GdkDrawable;
}

class gfxFontconfigUtils;
#ifndef MOZ_PANGO
class FontFamily;
class FontEntry;
typedef struct FT_LibraryRec_ *FT_Library;
#endif



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
                                  const gfxFontStyle *aStyle);

#ifndef MOZ_PANGO
    FontFamily *FindFontFamily(const nsAString& aName);
    FontEntry *FindFontEntry(const nsAString& aFamilyName, const gfxFontStyle& aFontStyle);
#endif

    static PRInt32 DPI() {
        if (sDPI == -1) {
            InitDPI();
        }
        NS_ASSERTION(sDPI > 0, "Something is wrong");
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

    static PRInt32 sDPI;
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
};

#endif 
