






































#include "gfxOS2Platform.h"
#include "gfxOS2Surface.h"
#include "gfxImageSurface.h"
#include "gfxOS2Fonts.h"
#include "nsTArray.h"

#include "gfxFontconfigUtils.h"





gfxFontconfigUtils *gfxOS2Platform::sFontconfigUtils = nsnull;

gfxOS2Platform::gfxOS2Platform()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::gfxOS2Platform()\n");
#endif
    
    
    cairo_os2_init();
#ifdef DEBUG_thebes
    printf("  cairo_os2_init() was called\n");
#endif
    if (!sFontconfigUtils) {
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();
    }
}

gfxOS2Platform::~gfxOS2Platform()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::~gfxOS2Platform()\n");
#endif
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nsnull;

    
    cairo_os2_fini();
#ifdef DEBUG_thebes
    printf("  cairo_os2_fini() was called\n");
#endif
}

already_AddRefed<gfxASurface>
gfxOS2Platform::CreateOffscreenSurface(const gfxIntSize& aSize,
                                       gfxASurface::gfxImageFormat aImageFormat)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Platform::CreateOffscreenSurface(%d/%d, %d)\n",
           aSize.width, aSize.height, aImageFormat);
#endif
    gfxASurface *newSurface = nsnull;

    
    
    if (aImageFormat == gfxASurface::ImageFormatARGB32 ||
        aImageFormat == gfxASurface::ImageFormatRGB24)
    {
        newSurface = new gfxOS2Surface(aSize, aImageFormat);
    } else if (aImageFormat == gfxASurface::ImageFormatA8 ||
               aImageFormat == gfxASurface::ImageFormatA1) {
        newSurface = new gfxImageSurface(aSize, aImageFormat);
    } else {
        return nsnull;
    }

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

nsresult
gfxOS2Platform::GetFontList(const nsACString& aLangGroup,
                            const nsACString& aGenericFamily,
                            nsTArray<nsString>& aListOfFonts)
{
#ifdef DEBUG_thebes
    char *langgroup = ToNewCString(aLangGroup),
         *family = ToNewCString(aGenericFamily);
    printf("gfxOS2Platform::GetFontList(%s, %s, ..)\n",
           langgroup, family);
    free(langgroup);
    free(family);
#endif
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
}

nsresult gfxOS2Platform::UpdateFontList()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::UpdateFontList()\n");
#endif
    mCodepointsWithNoFonts.reset();

    nsresult rv = sFontconfigUtils->UpdateFontList();

    
    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  
    return rv;
}

nsresult
gfxOS2Platform::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure, PRBool& aAborted)
{
#ifdef DEBUG_thebes
    char *fontname = ToNewCString(aFontName);
    printf("gfxOS2Platform::ResolveFontName(%s, ...)\n", fontname);
    free(fontname);
#endif
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback, aClosure,
                                             aAborted);
}

nsresult
gfxOS2Platform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    return sFontconfigUtils->GetStandardFamilyName(aFontName, aFamilyName);
}

gfxFontGroup *
gfxOS2Platform::CreateFontGroup(const nsAString &aFamilies,
                const gfxFontStyle *aStyle,
                gfxUserFontSet *aUserFontSet)
{
    return new gfxOS2FontGroup(aFamilies, aStyle, aUserFontSet);
}

already_AddRefed<gfxOS2Font>
gfxOS2Platform::FindFontForChar(PRUint32 aCh, gfxOS2Font *aFont)
{
#ifdef DEBUG_thebes
    printf("gfxOS2Platform::FindFontForChar(%d, ...)\n", aCh);
#endif

    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    
    

    
    nsTArray<nsString> fontList;
    nsCAutoString generic;
    nsresult rv = GetFontList(aFont->GetStyle()->langGroup, generic, fontList);
    if (NS_SUCCEEDED(rv)) {
        
        for (PRUint32 i = 3; i < fontList.Length(); i++) {
#ifdef DEBUG_thebes
            printf("searching in entry i=%d (%s)\n",
                   i, NS_LossyConvertUTF16toASCII(fontList[i]).get());
#endif
            nsRefPtr<gfxOS2Font> font =
                gfxOS2Font::GetOrMakeFont(fontList[i], aFont->GetStyle());
            if (!font)
                continue;
            FT_Face face = cairo_ft_scaled_font_lock_face(font->CairoScaledFont());
            if (!face || !face->charmap) {
                if (face)
                    cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
                continue;
            }

            FT_UInt gid = FT_Get_Char_Index(face, aCh); 
            if (gid != 0) {
                
                cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
                return font.forget();
            }
        }
    }

    
    mCodepointsWithNoFonts.set(aCh);
    return nsnull;
}

void
gfxOS2Platform::InitDisplayCaps()
{
    
    HDC dc = DevOpenDC((HAB)1, OD_MEMORY,"*",0L, NULL, NULLHANDLE);
    if (dc > 0) {
        
        LONG lDPI;
        if (DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &lDPI))
            gfxPlatform::sDPI = lDPI;
        DevCloseDC(dc);
    }

    if (gfxPlatform::sDPI <= 0) {
        
        gfxPlatform::sDPI = 96;
    }
}
