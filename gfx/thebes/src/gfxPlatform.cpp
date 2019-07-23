




































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
#include "gfxTextRunWordCache.h"

#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_ENABLE_GLITZ
#include <stdlib.h>
#endif

#include "cairo.h"
#include "lcms.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

gfxPlatform *gPlatform = nsnull;
int gGlitzState = -1;
static cmsHPROFILE gCMSOutputProfile = nsnull;
static cmsHTRANSFORM gCMSRGBTransform = nsnull;
static cmsHTRANSFORM gCMSRGBATransform = nsnull;

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

    rv = gfxTextRunWordCache::Init();
    if (NS_FAILED(rv)) {
        NS_ERROR("Could not initialize gfxTextRunWordCache");
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
    gfxTextRunWordCache::Shutdown();
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

already_AddRefed<gfxASurface>
gfxPlatform::OptimizeImage(gfxImageSurface *aSurface)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();

    gfxASurface::gfxImageFormat realFormat = aSurface->Format();

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
gfxPlatform::GetPrefFonts(const char *aLangGroup, nsString& aFonts, PRBool aAppendUnicode)
{
    aFonts.Truncate();

    AppendGenericFontFromPref(aFonts, aLangGroup, nsnull);
    if (aAppendUnicode)
        AppendGenericFontFromPref(aFonts, "x-unicode", nsnull);
}

PRBool
gfxPlatform::IsCMSEnabled()
{
    static PRBool sEnabled = -1;
    if (sEnabled == -1) {
        sEnabled = PR_TRUE;
        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            PRBool enabled;
            nsresult rv =
                prefs->GetBoolPref("gfx.color_management.enabled", &enabled);
            if (NS_SUCCEEDED(rv)) {
                sEnabled = enabled;
            }
        }
    }
    return sEnabled;
}

cmsHPROFILE
gfxPlatform::GetPlatformCMSOutputProfile()
{
    return nsnull;
}

cmsHPROFILE
gfxPlatform::GetCMSOutputProfile()
{
    if (!gCMSOutputProfile) {
        
#ifdef DEBUG_tor
        cmsErrorAction(LCMS_ERROR_SHOW);
#else
        cmsErrorAction(LCMS_ERROR_IGNORE);
#endif

        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            nsXPIDLCString fname;
            nsresult rv =
                prefs->GetCharPref("gfx.color_management.display_profile",
                                   getter_Copies(fname));
            if (NS_SUCCEEDED(rv) && !fname.IsEmpty()) {
                gCMSOutputProfile = cmsOpenProfileFromFile(fname, "r");
#ifdef DEBUG_tor
                if (gCMSOutputProfile)
                    fprintf(stderr,
                            "ICM profile read from %s successfully\n",
                            fname.get());
#endif
            }
        }

        if (!gCMSOutputProfile) {
            gCMSOutputProfile =
                gfxPlatform::GetPlatform()->GetPlatformCMSOutputProfile();
        }

        if (!gCMSOutputProfile) {
            gCMSOutputProfile = cmsCreate_sRGBProfile();
        }
    }

    return gCMSOutputProfile;
}

cmsHTRANSFORM
gfxPlatform::GetCMSRGBTransform()
{
    if (!gCMSRGBTransform) {
        cmsHPROFILE inProfile, outProfile;
        outProfile = GetCMSOutputProfile();
        inProfile = cmsCreate_sRGBProfile();

        if (!inProfile || !outProfile)
            return nsnull;

        gCMSRGBTransform = cmsCreateTransform(inProfile, TYPE_RGB_8,
                                              outProfile, TYPE_RGB_8,
                                              INTENT_PERCEPTUAL, 0);
    }

    return gCMSRGBTransform;
}

cmsHTRANSFORM
gfxPlatform::GetCMSRGBATransform()
{
    if (!gCMSRGBATransform) {
        cmsHPROFILE inProfile, outProfile;
        outProfile = GetCMSOutputProfile();
        inProfile = cmsCreate_sRGBProfile();

        if (!inProfile || !outProfile)
            return nsnull;

        gCMSRGBATransform = cmsCreateTransform(inProfile, TYPE_RGBA_8,
                                               outProfile, TYPE_RGBA_8,
                                               INTENT_PERCEPTUAL, 0);
    }

    return gCMSRGBATransform;
}
