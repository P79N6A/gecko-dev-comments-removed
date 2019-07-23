





































#include <QPixmap>
#include <QX11Info>
#include <QApplication>
#include <QWidget>

#include "gfxQtPlatform.h"

#include "gfxFontconfigUtils.h"

#include "cairo.h"

#include "gfxImageSurface.h"
#include "gfxQPainterSurface.h"

#include "gfxQtFonts.h"

#include "nsUnicharUtils.h"

#include <fontconfig/fontconfig.h>

#include "nsMathUtils.h"

#include "lcms.h"

#include <ft2build.h>
#include FT_FREETYPE_H

PRInt32 gfxQtPlatform::sDPI = -1;
gfxFontconfigUtils *gfxQtPlatform::sFontconfigUtils = nsnull;
static cairo_user_data_key_t cairo_qt_pixmap_key;
static void do_qt_pixmap_unref (void *data)
{
    QPixmap *pmap = (QPixmap*)data;
    delete pmap;
}

typedef nsDataHashtable<nsStringHashKey, nsRefPtr<FontFamily> > FontTable;
static FontTable *gPlatformFonts = NULL;
static FT_Library gPlatformFTLibrary = NULL;


gfxQtPlatform::gfxQtPlatform()
{
    if (!sFontconfigUtils)
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();


    FT_Init_FreeType(&gPlatformFTLibrary);

    gPlatformFonts = new FontTable();
    gPlatformFonts->Init(100);
    UpdateFontList();

    InitDPI();
}

gfxQtPlatform::~gfxQtPlatform()
{
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nsnull;

    delete gPlatformFonts;
    gPlatformFonts = NULL;

    cairo_debug_reset_static_data();

    FT_Done_FreeType(gPlatformFTLibrary);
    gPlatformFTLibrary = NULL;


#if 0
    
    
    
    
    FcFini();
#endif
}

already_AddRefed<gfxASurface>
gfxQtPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                      gfxASurface::gfxImageFormat imageFormat)
{
    
    nsRefPtr<gfxASurface> newSurface =
        new gfxQPainterSurface (size, imageFormat);

    return newSurface.forget();
}

already_AddRefed<gfxASurface>
gfxQtPlatform::OptimizeImage(gfxImageSurface *aSurface,
                             gfxASurface::gfxImageFormat format)
{
    NS_ADDREF(aSurface);
    return aSurface;

#if 0
    const gfxIntSize& surfaceSize = aSurface->GetSize();

    nsRefPtr<gfxASurface> optSurface = CreateOffscreenSurface(surfaceSize, format);
    if (!optSurface || optSurface->CairoStatus() != 0)
        return nsnull;

    gfxContext tmpCtx(optSurface);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx.SetSource(aSurface);
    tmpCtx.Paint();

    gfxASurface *ret = optSurface;
    NS_ADDREF(ret);
    return ret;
#endif
}

nsresult
gfxQtPlatform::GetFontList(const nsACString& aLangGroup,
                            const nsACString& aGenericFamily,
                            nsStringArray& aListOfFonts)
{
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
}

nsresult
gfxQtPlatform::UpdateFontList()
{
    FcPattern *pat = NULL;
    FcObjectSet *os = NULL;
    FcFontSet *fs = NULL;
    PRInt32 result = -1;

    pat = FcPatternCreate();
    os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_INDEX, FC_WEIGHT, FC_SLANT, FC_WIDTH, NULL);

    fs = FcFontList(NULL, pat, os);


    for (int i = 0; i < fs->nfont; i++) {
        char *str;

        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, (FcChar8 **) &str) != FcResultMatch)
            continue;

        printf("Family: %s\n", str);

        nsAutoString name(NS_ConvertUTF8toUTF16(nsDependentCString(str)).get());
        nsAutoString key(name);
        ToLowerCase(key);
        nsRefPtr<FontFamily> ff;
        if (!gPlatformFonts->Get(key, &ff)) {
            ff = new FontFamily(name);
            gPlatformFonts->Put(key, ff);
        }

        nsRefPtr<FontEntry> fe = new FontEntry(ff->mName);
        ff->mFaces.AppendElement(fe);

        if (FcPatternGetString(fs->fonts[i], FC_FILE, 0, (FcChar8 **) &str) == FcResultMatch) {
            fe->mFilename = nsDependentCString(str);
            printf(" - file: %s\n", str);
        }

        int x;
        if (FcPatternGetInteger(fs->fonts[i], FC_INDEX, 0, &x) == FcResultMatch) {
            printf(" - index: %d\n", x);
            fe->mFTFontIndex = x;
        } else {
            fe->mFTFontIndex = 0;
        }

        if (FcPatternGetInteger(fs->fonts[i], FC_WEIGHT, 0, &x) == FcResultMatch) {
            switch(x) {
            case 0:
                fe->mWeight = 100;
                break;
            case 40:
                fe->mWeight = 200;
                break;
            case 50:
                fe->mWeight = 300;
                break;
            case 75:
            case 80:
                fe->mWeight = 400;
                break;
            case 100:
                fe->mWeight = 500;
                break;
            case 180:
                fe->mWeight = 600;
                break;
            case 200:
                fe->mWeight = 700;
                break;
            case 205:
                fe->mWeight = 800;
                break;
            case 210:
                fe->mWeight = 900;
                break;
            default:
                
                fe->mWeight = (((x * 4) + 100) / 100) * 100;
                break;
            }
            printf(" - weight: %d\n", fe->mWeight);
        }

        fe->mItalic = PR_FALSE;
        if (FcPatternGetInteger(fs->fonts[i], FC_SLANT, 0, &x) == FcResultMatch) {
            switch (x) {
            case FC_SLANT_ITALIC:
            case FC_SLANT_OBLIQUE:
                fe->mItalic = PR_TRUE;
            }
            printf(" - slant: %d\n", x);
        }

        if (FcPatternGetInteger(fs->fonts[i], FC_WIDTH, 0, &x) == FcResultMatch)
            printf(" - width: %d\n", x);
        
    }

    if (pat)
        FcPatternDestroy(pat);
    if (os)
        FcObjectSetDestroy(os);
    if (fs)
        FcFontSetDestroy(fs);

    return sFontconfigUtils->UpdateFontList();
}

nsresult
gfxQtPlatform::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure,
                                PRBool& aAborted)
{
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback,
                                             aClosure, aAborted);
}

nsresult
gfxQtPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    return sFontconfigUtils->GetStandardFamilyName(aFontName, aFamilyName);
}

gfxFontGroup *
gfxQtPlatform::CreateFontGroup(const nsAString &aFamilies,
                               const gfxFontStyle *aStyle)
{
    return new gfxQtFontGroup(aFamilies, aStyle);
}


void
gfxQtPlatform::InitDPI()
{
    if (sDPI <= 0) {
        
        sDPI = 96;
    }
}

cmsHPROFILE
gfxQtPlatform::GetPlatformCMSOutputProfile()
{
    return nsnull;
}


FT_Library
gfxQtPlatform::GetFTLibrary()
{
    return gPlatformFTLibrary;
}

FontFamily *
gfxQtPlatform::FindFontFamily(const nsAString& aName)
{
    nsAutoString name(aName);
    ToLowerCase(name);

    nsRefPtr<FontFamily> ff;
    if (!gPlatformFonts->Get(name, &ff)) {
        return nsnull;
    }
    return ff.get();
}

FontEntry *
gfxQtPlatform::FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle)
{
    nsRefPtr<FontFamily> ff = FindFontFamily(aName);
    if (!ff)
        return nsnull;

    return ff->FindFontEntry(aFontStyle);
}
