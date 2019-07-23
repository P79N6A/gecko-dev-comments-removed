




































#include "gfxPlatform.h"

#if defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#elif defined(XP_MACOSX)
#include "gfxPlatformMac.h"
#include "gfxQuartzFontCache.h"
#elif defined(MOZ_WIDGET_GTK2)
#include "gfxPlatformGtk.h"
#elif defined(XP_BEOS)
#include "gfxBeOSPlatform.h"
#elif defined(XP_OS2)
#include "gfxOS2Platform.h"
#endif

#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxTextRunCache.h"

#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_ENABLE_GLITZ
#include <stdlib.h>
#endif

#include "cairo.h"

gfxPlatform *gPlatform = nsnull;
int gGlitzState = -1;

gfxPlatform*
gfxPlatform::GetPlatform()
{
    return gPlatform;
}

nsresult
gfxPlatform::Init()
{
    NS_ASSERTION(!gPlatform, "Already started???");
#if defined(XP_WIN)
    gPlatform = new gfxWindowsPlatform;
#elif defined(XP_MACOSX)
    gPlatform = new gfxPlatformMac;
#elif defined(MOZ_WIDGET_GTK2)
    gPlatform = new gfxPlatformGtk;
#elif defined(XP_BEOS)
    gPlatform = new gfxBeOSPlatform;
#elif defined(XP_OS2)
    gPlatform = new gfxOS2Platform;
#endif
    if (!gPlatform)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;

#if defined(XP_MACOSX)
    rv = gfxQuartzFontCache::Init();
    if (NS_FAILED(rv)) {
        NS_ERROR("Could not initialize gfxQuartzFontCache");
        Shutdown();
        return rv;
    }
#endif

    rv = gfxFontCache::Init();
    if (NS_FAILED(rv)) {
        NS_ERROR("Could not initialize gfxFontCache");
        Shutdown();
        return rv;
    }

    rv = gfxTextRunCache::Init();
    if (NS_FAILED(rv)) {
        NS_ERROR("Could not initialize gfxTextRunCache");
        Shutdown();
        return rv;
    }

    return NS_OK;
}

void
gfxPlatform::Shutdown()
{
    
    
    gfxTextRunCache::Shutdown();
    gfxFontCache::Shutdown();
#if defined(XP_MACOSX)
    gfxQuartzFontCache::Shutdown();
#endif
    delete gPlatform;
    gPlatform = nsnull;
}

gfxPlatform::~gfxPlatform()
{
    
    
    
    
    cairo_debug_reset_static_data();
}

PRBool
gfxPlatform::UseGlitz()
{
#ifdef MOZ_ENABLE_GLITZ
    if (gGlitzState == -1) {
        if (getenv("MOZ_GLITZ"))
            gGlitzState = 1;
        else
            gGlitzState = 0;
    }

    if (gGlitzState)
        return PR_TRUE;
#endif

    return PR_FALSE;
}

void
gfxPlatform::SetUseGlitz(PRBool use)
{
    gGlitzState = (use ? 1 : 0);
}

PRBool
gfxPlatform::DoesARGBImageDataHaveAlpha(PRUint8* data,
                                        PRUint32 width,
                                        PRUint32 height,
                                        PRUint32 stride)
{
    PRUint32 *r;

    for (PRUint32 j = 0; j < height; j++) {
        r = (PRUint32*) (data + stride*j);
        for (PRUint32 i = 0; i < width; i++) {
            if ((*r++ & 0xff000000) != 0xff000000) {
                return PR_TRUE;
            }
        }
    }

    return PR_FALSE;    
}

already_AddRefed<gfxASurface>
gfxPlatform::OptimizeImage(gfxImageSurface *aSurface)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();

    gfxASurface::gfxImageFormat realFormat = aSurface->Format();

    if (realFormat == gfxASurface::ImageFormatARGB32) {
        
        if (!DoesARGBImageDataHaveAlpha(aSurface->Data(),
                                        surfaceSize.width,
                                        surfaceSize.height,
                                        aSurface->Stride()))
        {
            realFormat = gfxASurface::ImageFormatRGB24;
        }
    }

    nsRefPtr<gfxASurface> optSurface = CreateOffscreenSurface(surfaceSize, realFormat);

    if (!optSurface)
        return nsnull;

    nsRefPtr<gfxContext> tmpCtx(new gfxContext(optSurface));
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx->SetSource(aSurface);
    tmpCtx->Paint();

    gfxASurface *ret = optSurface;
    NS_ADDREF(ret);
    return ret;
}

nsresult
gfxPlatform::GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
gfxPlatform::UpdateFontList()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

static void
AppendGenericFontFromPref(nsString& aFonts, const char *aLangGroup, const char *aGenericName)
{
    nsresult rv;

    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
    if (!prefs)
        return;

    nsCAutoString prefName;
    nsXPIDLString value;

    nsXPIDLString genericName;
    if (aGenericName) {
        genericName = NS_ConvertASCIItoUTF16(aGenericName);
    } else {
        prefName.AssignLiteral("font.default.");
        prefName.Append(aLangGroup);
        prefs->CopyUnicharPref(prefName.get(), getter_Copies(genericName));
    }

    nsCAutoString genericDotLang;
    genericDotLang.Assign(NS_ConvertUTF16toUTF8(genericName));
    genericDotLang.AppendLiteral(".");
    genericDotLang.Append(aLangGroup);

    prefName.AssignLiteral("font.name.");
    prefName.Append(genericDotLang);
    rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts.Append(value);
    }

    prefName.AssignLiteral("font.name-list.");
    prefName.Append(genericDotLang);
    rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts.Append(value);
    }
}

void
gfxPlatform::GetPrefFonts(const char *aLangGroup, nsString& aFonts)
{
    aFonts.Truncate();

    AppendGenericFontFromPref(aFonts, aLangGroup, nsnull);
    AppendGenericFontFromPref(aFonts, "x-unicode", nsnull);
}

