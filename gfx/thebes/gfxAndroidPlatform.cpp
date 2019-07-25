





































#include "gfxAndroidPlatform.h"

#include "gfxFT2FontList.h"
#include "gfxImageSurface.h"

#include "cairo.h"

#include "ft2build.h"
#include FT_FREETYPE_H

static FT_Library gPlatformFTLibrary = NULL;

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoFonts" , ## args)

gfxAndroidPlatform::gfxAndroidPlatform()
{
    FT_Init_FreeType(&gPlatformFTLibrary);
}

gfxAndroidPlatform::~gfxAndroidPlatform()
{
    cairo_debug_reset_static_data();

    FT_Done_FreeType(gPlatformFTLibrary);
    gPlatformFTLibrary = NULL;
}

already_AddRefed<gfxASurface>
gfxAndroidPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                      gfxASurface::gfxContentType contentType)
{
    nsRefPtr<gfxASurface> newSurface;
    if (contentType == gfxImageSurface::CONTENT_COLOR)
        newSurface = new gfxImageSurface (size, GetOffscreenFormat());
    else
        newSurface = new gfxImageSurface (size, gfxASurface::FormatFromContent(contentType));

    return newSurface.forget();
}

nsresult
gfxAndroidPlatform::GetFontList(nsIAtom *aLangGroup,
                                const nsACString& aGenericFamily,
                                nsTArray<nsString>& aListOfFonts)
{
    gfxPlatformFontList::PlatformFontList()->GetFontList(aLangGroup,
                                                         aGenericFamily,
                                                         aListOfFonts);
    return NS_OK;
}

void
gfxAndroidPlatform::GetFontList(InfallibleTArray<FontListEntry>* retValue)
{
    gfxFT2FontList::PlatformFontList()->GetFontList(retValue);
}

nsresult
gfxAndroidPlatform::UpdateFontList()
{
    gfxPlatformFontList::PlatformFontList()->UpdateFontList();
    return NS_OK;
}

nsresult
gfxAndroidPlatform::ResolveFontName(const nsAString& aFontName,
                                    FontResolverCallback aCallback,
                                    void *aClosure,
                                    bool& aAborted)
{
    nsAutoString resolvedName;
    if (!gfxPlatformFontList::PlatformFontList()->
             ResolveFontName(aFontName, resolvedName)) {
        aAborted = PR_FALSE;
        return NS_OK;
    }
    aAborted = !(*aCallback)(resolvedName, aClosure);
    return NS_OK;
}

nsresult
gfxAndroidPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(aFontName, aFamilyName);
    return NS_OK;
}

gfxPlatformFontList*
gfxAndroidPlatform::CreatePlatformFontList()
{
    gfxPlatformFontList* list = new gfxFT2FontList();
    if (NS_SUCCEEDED(list->InitFontList())) {
        return list;
    }
    gfxPlatformFontList::Shutdown();
    return nsnull;
}

bool
gfxAndroidPlatform::IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_OPENTYPE |
                        gfxUserFontSet::FLAG_FORMAT_WOFF |
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE)) {
        return PR_TRUE;
    }

    
    if (aFormatFlags != 0) {
        return PR_FALSE;
    }

    
    return PR_TRUE;
}

gfxFontGroup *
gfxAndroidPlatform::CreateFontGroup(const nsAString &aFamilies,
                               const gfxFontStyle *aStyle,
                               gfxUserFontSet* aUserFontSet)
{
    return new gfxFontGroup(aFamilies, aStyle, aUserFontSet);
}

FT_Library
gfxAndroidPlatform::GetFTLibrary()
{
    return gPlatformFTLibrary;
}

gfxFontEntry* 
gfxAndroidPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                     const PRUint8 *aFontData, PRUint32 aLength)
{
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aProxyEntry,
                                                                     aFontData,
                                                                     aLength);
}

