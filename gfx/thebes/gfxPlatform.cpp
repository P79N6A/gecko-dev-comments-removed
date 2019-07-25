




































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxPlatform.h"

#if defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#elif defined(XP_MACOSX)
#include "gfxPlatformMac.h"
#elif defined(MOZ_WIDGET_GTK2)
#include "gfxPlatformGtk.h"
#elif defined(MOZ_WIDGET_QT)
#include "gfxQtPlatform.h"
#elif defined(XP_OS2)
#include "gfxOS2Platform.h"
#elif defined(ANDROID)
#include "gfxAndroidPlatform.h"
#endif

#include "gfxAtoms.h"
#include "gfxPlatformFontList.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxTextRunCache.h"
#include "gfxTextRunWordCache.h"
#include "gfxUserFontSet.h"
#include "gfxUnicodeProperties.h"
#include "harfbuzz/hb-unicode.h"

#include "nsUnicodeRange.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsIUGenCategory.h"
#include "nsUnicharUtilCIID.h"
#include "nsILocaleService.h"

#include "nsWeakReference.h"

#include "cairo.h"
#include "qcms.h"

#include "plstr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsCRT.h"
#include "GLContext.h"
#include "GLContextProvider.h"

#include "mozilla/FunctionTimer.h"

#include "nsIGfxInfo.h"

gfxPlatform *gPlatform = nsnull;
static bool gEverInitialized = false;


static qcms_profile *gCMSOutputProfile = nsnull;
static qcms_profile *gCMSsRGBProfile = nsnull;

static qcms_transform *gCMSRGBTransform = nsnull;
static qcms_transform *gCMSInverseRGBTransform = nsnull;
static qcms_transform *gCMSRGBATransform = nsnull;

static PRBool gCMSInitialized = PR_FALSE;
static eCMSMode gCMSMode = eCMSMode_Off;
static int gCMSIntent = -2;

static const char *CMPrefName = "gfx.color_management.mode";
static const char *CMPrefNameOld = "gfx.color_management.enabled";
static const char *CMIntentPrefName = "gfx.color_management.rendering_intent";
static const char *CMProfilePrefName = "gfx.color_management.display_profile";
static const char *CMForceSRGBPrefName = "gfx.color_management.force_srgb";

static void ShutdownCMS();
static void MigratePrefs();



#ifdef PR_LOGGING
static PRLogModuleInfo *sFontlistLog = nsnull;
static PRLogModuleInfo *sFontInitLog = nsnull;
static PRLogModuleInfo *sTextrunLog = nsnull;
static PRLogModuleInfo *sTextrunuiLog = nsnull;
#endif



class SRGBOverrideObserver : public nsIObserver,
                             public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS2(SRGBOverrideObserver, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
SRGBOverrideObserver::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *someData)
{
    NS_ASSERTION(NS_strcmp(someData,
                   NS_LITERAL_STRING("gfx.color_mangement.force_srgb").get()),
                 "Restarting CMS on wrong pref!");
    ShutdownCMS();
    return NS_OK;
}

#define GFX_DOWNLOADABLE_FONTS_ENABLED "gfx.downloadable_fonts.enabled"
#define GFX_DOWNLOADABLE_FONTS_SANITIZE "gfx.downloadable_fonts.sanitize"

#define GFX_PREF_HARFBUZZ_SCRIPTS "gfx.font_rendering.harfbuzz.scripts"
#define HARFBUZZ_SCRIPTS_DEFAULT  gfxUnicodeProperties::SHAPING_DEFAULT

class FontPrefsObserver : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(FontPrefsObserver, nsIObserver)

NS_IMETHODIMP
FontPrefsObserver::Observe(nsISupports *aSubject,
                           const char *aTopic,
                           const PRUnichar *someData)
{
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(aSubject);
    if (!branch || someData == nsnull) {
        NS_ERROR("font pref observer code broken");
        return NS_ERROR_UNEXPECTED;
    }
    
    gfxPlatform::GetPlatform()->FontsPrefsChanged(branch, 
        NS_ConvertUTF16toUTF8(someData).get());

    return NS_OK;
}





static const char *gPrefLangNames[] = {
    "x-western",
    "x-central-euro",
    "ja",
    "zh-TW",
    "zh-CN",
    "zh-HK",
    "ko",
    "x-cyrillic",
    "x-baltic",
    "el",
    "tr",
    "th",
    "he",
    "ar",
    "x-devanagari",
    "x-tamil",
    "x-armn",
    "x-beng",
    "x-cans",
    "x-ethi",
    "x-geor",
    "x-gujr",
    "x-guru",
    "x-khmr",
    "x-mlym",
    "x-orya",
    "x-telu",
    "x-knda",
    "x-sinh",
    "x-tibt",
    "x-unicode",
    "x-user-def"
};

gfxPlatform::gfxPlatform()
{
    mUseHarfBuzzScripts = UNINITIALIZED_VALUE;
    mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    mDownloadableFontsSanitize = UNINITIALIZED_VALUE;
}

gfxPlatform*
gfxPlatform::GetPlatform()
{
    if (!gPlatform) {
        Init();
    }
    return gPlatform;
}

void
gfxPlatform::Init()
{
    if (gEverInitialized) {
        NS_RUNTIMEABORT("Already started???");
    }
    gEverInitialized = true;

    gfxAtoms::RegisterAtoms();

#ifdef PR_LOGGING
    sFontlistLog = PR_NewLogModule("fontlist");;
    sFontInitLog = PR_NewLogModule("fontinit");;
    sTextrunLog = PR_NewLogModule("textrun");;
    sTextrunuiLog = PR_NewLogModule("textrunui");;
#endif


    






    nsCOMPtr<nsIGfxInfo> gfxInfo;
    
    gfxInfo = do_GetService("@mozilla.org/gfx/info;1");

#if defined(XP_WIN)
    gPlatform = new gfxWindowsPlatform;
#elif defined(XP_MACOSX)
    gPlatform = new gfxPlatformMac;
#elif defined(MOZ_WIDGET_GTK2)
    gPlatform = new gfxPlatformGtk;
#elif defined(MOZ_WIDGET_QT)
    gPlatform = new gfxQtPlatform;
#elif defined(XP_OS2)
    gPlatform = new gfxOS2Platform;
#elif defined(ANDROID)
    gPlatform = new gfxAndroidPlatform;
#else
    #error "No gfxPlatform implementation available"
#endif

    nsresult rv;

#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID) 
    rv = gfxPlatformFontList::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxPlatformFontList");
    }
#endif

    gPlatform->mScreenReferenceSurface =
        gPlatform->CreateOffscreenSurface(gfxIntSize(1,1),
                                          gfxASurface::CONTENT_COLOR_ALPHA);
    if (!gPlatform->mScreenReferenceSurface) {
        NS_RUNTIMEABORT("Could not initialize mScreenReferenceSurface");
    }

    rv = gfxFontCache::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxFontCache");
    }

    rv = gfxTextRunWordCache::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxTextRunWordCache");
    }

    rv = gfxTextRunCache::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxTextRunCache");
    }

    
    MigratePrefs();

    
    gPlatform->overrideObserver = new SRGBOverrideObserver();
    FontPrefsObserver *fontPrefObserver = new FontPrefsObserver();

    nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        prefs->AddObserver(CMForceSRGBPrefName, gPlatform->overrideObserver, PR_TRUE);
        prefs->AddObserver("gfx.downloadable_fonts.", fontPrefObserver, PR_FALSE);
        prefs->AddObserver("gfx.font_rendering.", fontPrefObserver, PR_FALSE);
    }

    
    
    nsCOMPtr<nsISupports> forceReg
        = do_CreateInstance("@mozilla.org/gfx/init;1");
}

void
gfxPlatform::Shutdown()
{
    
    
    gfxTextRunCache::Shutdown();
    gfxTextRunWordCache::Shutdown();
    gfxFontCache::Shutdown();
    gfxFontGroup::Shutdown();
#if defined(XP_MACOSX) || defined(XP_WIN) 
    gfxPlatformFontList::Shutdown();
#endif

    
    ShutdownCMS();

    
    nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs)
        prefs->RemoveObserver(CMForceSRGBPrefName, gPlatform->overrideObserver);

    mozilla::gl::GLContextProvider::Shutdown();

    delete gPlatform;
    gPlatform = nsnull;
}

gfxPlatform::~gfxPlatform()
{
    
    
    
    
    
    
#if MOZ_TREE_CAIRO && (defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING) || defined(NS_TRACE_MALLOC))
    cairo_debug_reset_static_data();
#endif

#if 0
    
    
    
    
    FcFini();
#endif
}

already_AddRefed<gfxASurface>
gfxPlatform::OptimizeImage(gfxImageSurface *aSurface,
                           gfxASurface::gfxImageFormat format)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();

#ifdef XP_WIN
    if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() == 
        gfxWindowsPlatform::RENDER_DIRECT2D) {
        return nsnull;
    }
#endif
    nsRefPtr<gfxASurface> optSurface = CreateOffscreenSurface(surfaceSize, gfxASurface::ContentFromFormat(format));
    if (!optSurface || optSurface->CairoStatus() != 0)
        return nsnull;

    gfxContext tmpCtx(optSurface);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx.SetSource(aSurface);
    tmpCtx.Paint();

    gfxASurface *ret = optSurface;
    NS_ADDREF(ret);
    return ret;
}

nsresult
gfxPlatform::GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
gfxPlatform::UpdateFontList()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRBool 
gfxPlatform::GetBoolPref(const char *aPref, PRBool aDefault)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRBool allow;
        nsresult rv = prefs->GetBoolPref(aPref, &allow);
        if (NS_SUCCEEDED(rv))
            return allow;
    }

    return aDefault;
}

PRBool
gfxPlatform::DownloadableFontsEnabled()
{
    if (mAllowDownloadableFonts == UNINITIALIZED_VALUE) {
        mAllowDownloadableFonts =
            GetBoolPref(GFX_DOWNLOADABLE_FONTS_ENABLED, PR_FALSE);
    }

    return mAllowDownloadableFonts;
}

PRBool
gfxPlatform::SanitizeDownloadedFonts()
{
    if (mDownloadableFontsSanitize == UNINITIALIZED_VALUE) {
        mDownloadableFontsSanitize =
            GetBoolPref(GFX_DOWNLOADABLE_FONTS_SANITIZE, PR_TRUE);
    }

    return mDownloadableFontsSanitize;
}

PRBool
gfxPlatform::UseHarfBuzzForScript(PRInt32 aScriptCode)
{
    if (mUseHarfBuzzScripts == UNINITIALIZED_VALUE) {
        mUseHarfBuzzScripts = HARFBUZZ_SCRIPTS_DEFAULT;
        nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            PRInt32 scripts;
            nsresult rv = prefs->GetIntPref(GFX_PREF_HARFBUZZ_SCRIPTS, &scripts);
            if (NS_SUCCEEDED(rv)) {
                mUseHarfBuzzScripts = scripts;
            }
        }
    }

    PRInt32 shapingType = gfxUnicodeProperties::ScriptShapingType(aScriptCode);

    return (mUseHarfBuzzScripts & shapingType) != 0;
}

gfxFontEntry*
gfxPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                              const PRUint8 *aFontData,
                              PRUint32 aLength)
{
    
    
    
    
    
    if (aFontData) {
        NS_Free((void*)aFontData);
    }
    return nsnull;
}

static void
AppendGenericFontFromPref(nsString& aFonts, nsIAtom *aLangGroup, const char *aGenericName)
{
    nsresult rv;

    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefs)
        return;

    nsCAutoString prefName, langGroupString;
    nsXPIDLCString nameValue, nameListValue;

    aLangGroup->ToUTF8String(langGroupString);

    nsCAutoString genericDotLang;
    if (aGenericName) {
        genericDotLang.Assign(aGenericName);
    } else {
        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroupString);
        prefs->GetCharPref(prefName.get(), getter_Copies(genericDotLang));
    }

    genericDotLang.AppendLiteral(".");
    genericDotLang.Append(langGroupString);

    
    prefName.AssignLiteral("font.name.");
    prefName.Append(genericDotLang);
    rv = prefs->GetCharPref(prefName.get(), getter_Copies(nameValue));
    if (NS_SUCCEEDED(rv)) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts.Append(NS_ConvertUTF8toUTF16(nameValue));
    }

    
    prefName.AssignLiteral("font.name-list.");
    prefName.Append(genericDotLang);
    rv = prefs->GetCharPref(prefName.get(), getter_Copies(nameListValue));
    if (NS_SUCCEEDED(rv) && !nameListValue.Equals(nameValue)) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts.Append(NS_ConvertUTF8toUTF16(nameListValue));
    }
}

void
gfxPlatform::GetPrefFonts(nsIAtom *aLanguage, nsString& aFonts, PRBool aAppendUnicode)
{
    aFonts.Truncate();

    AppendGenericFontFromPref(aFonts, aLanguage, nsnull);
    if (aAppendUnicode)
        AppendGenericFontFromPref(aFonts, gfxAtoms::x_unicode, nsnull);
}

PRBool gfxPlatform::ForEachPrefFont(eFontPrefLang aLangArray[], PRUint32 aLangArrayLen, PrefFontCallback aCallback,
                                    void *aClosure)
{
    nsresult rv;

    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefs)
        return PR_FALSE;

    PRUint32    i;
    
    for (i = 0; i < aLangArrayLen; i++) {
        eFontPrefLang prefLang = aLangArray[i];
        const char *langGroup = GetPrefLangName(prefLang);
        
        nsCAutoString prefName;
        nsXPIDLCString nameValue, nameListValue;
    
        nsCAutoString genericDotLang;
        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroup);
        prefs->GetCharPref(prefName.get(), getter_Copies(genericDotLang));
    
        genericDotLang.AppendLiteral(".");
        genericDotLang.Append(langGroup);
    
        
        prefName.AssignLiteral("font.name.");
        prefName.Append(genericDotLang);
        rv = prefs->GetCharPref(prefName.get(), getter_Copies(nameValue));
        if (NS_SUCCEEDED(rv)) {
            if (!aCallback(prefLang, NS_ConvertUTF8toUTF16(nameValue), aClosure))
                return PR_FALSE;
        }
    
        
        prefName.AssignLiteral("font.name-list.");
        prefName.Append(genericDotLang);
        rv = prefs->GetCharPref(prefName.get(), getter_Copies(nameListValue));
        if (NS_SUCCEEDED(rv) && !nameListValue.Equals(nameValue)) {
            const char kComma = ',';
            const char *p, *p_end;
            nsCAutoString list(nameListValue);
            list.BeginReading(p);
            list.EndReading(p_end);
            while (p < p_end) {
                while (nsCRT::IsAsciiSpace(*p)) {
                    if (++p == p_end)
                        break;
                }
                if (p == p_end)
                    break;
                const char *start = p;
                while (++p != p_end && *p != kComma)
                     ;
                nsCAutoString fontName(Substring(start, p));
                fontName.CompressWhitespace(PR_FALSE, PR_TRUE);
                if (!aCallback(prefLang, NS_ConvertUTF8toUTF16(fontName), aClosure))
                    return PR_FALSE;
                p++;
            }
        }
    }

    return PR_TRUE;
}

eFontPrefLang
gfxPlatform::GetFontPrefLangFor(const char* aLang)
{
    if (!aLang || !aLang[0])
        return eFontPrefLang_Others;
    for (PRUint32 i = 0; i < PRUint32(eFontPrefLang_LangCount); ++i) {
        if (!PL_strcasecmp(gPrefLangNames[i], aLang))
            return eFontPrefLang(i);
    }
    return eFontPrefLang_Others;
}

eFontPrefLang
gfxPlatform::GetFontPrefLangFor(nsIAtom *aLang)
{
    if (!aLang)
        return eFontPrefLang_Others;
    nsCAutoString lang;
    aLang->ToUTF8String(lang);
    return GetFontPrefLangFor(lang.get());
}

const char*
gfxPlatform::GetPrefLangName(eFontPrefLang aLang)
{
    if (PRUint32(aLang) < PRUint32(eFontPrefLang_AllCount))
        return gPrefLangNames[PRUint32(aLang)];
    return nsnull;
}

eFontPrefLang
gfxPlatform::GetFontPrefLangFor(PRUint8 aUnicodeRange)
{
    switch (aUnicodeRange) {
        case kRangeSetLatin:   return eFontPrefLang_Western;
        case kRangeCyrillic:   return eFontPrefLang_Cyrillic;
        case kRangeGreek:      return eFontPrefLang_Greek;
        case kRangeTurkish:    return eFontPrefLang_Turkish;
        case kRangeHebrew:     return eFontPrefLang_Hebrew;
        case kRangeArabic:     return eFontPrefLang_Arabic;
        case kRangeBaltic:     return eFontPrefLang_Baltic;
        case kRangeThai:       return eFontPrefLang_Thai;
        case kRangeKorean:     return eFontPrefLang_Korean;
        case kRangeJapanese:   return eFontPrefLang_Japanese;
        case kRangeSChinese:   return eFontPrefLang_ChineseCN;
        case kRangeTChinese:   return eFontPrefLang_ChineseTW;
        case kRangeDevanagari: return eFontPrefLang_Devanagari;
        case kRangeTamil:      return eFontPrefLang_Tamil;
        case kRangeArmenian:   return eFontPrefLang_Armenian;
        case kRangeBengali:    return eFontPrefLang_Bengali;
        case kRangeCanadian:   return eFontPrefLang_Canadian;
        case kRangeEthiopic:   return eFontPrefLang_Ethiopic;
        case kRangeGeorgian:   return eFontPrefLang_Georgian;
        case kRangeGujarati:   return eFontPrefLang_Gujarati;
        case kRangeGurmukhi:   return eFontPrefLang_Gurmukhi;
        case kRangeKhmer:      return eFontPrefLang_Khmer;
        case kRangeMalayalam:  return eFontPrefLang_Malayalam;
        case kRangeOriya:      return eFontPrefLang_Oriya;
        case kRangeTelugu:     return eFontPrefLang_Telugu;
        case kRangeKannada:    return eFontPrefLang_Kannada;
        case kRangeSinhala:    return eFontPrefLang_Sinhala;
        case kRangeTibetan:    return eFontPrefLang_Tibetan;
        case kRangeSetCJK:     return eFontPrefLang_CJKSet;
        default:               return eFontPrefLang_Others;
    }
}

PRBool 
gfxPlatform::IsLangCJK(eFontPrefLang aLang)
{
    switch (aLang) {
        case eFontPrefLang_Japanese:
        case eFontPrefLang_ChineseTW:
        case eFontPrefLang_ChineseCN:
        case eFontPrefLang_ChineseHK:
        case eFontPrefLang_Korean:
        case eFontPrefLang_CJKSet:
            return PR_TRUE;
        default:
            return PR_FALSE;
    }
}

void 
gfxPlatform::GetLangPrefs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang)
{
    if (IsLangCJK(aCharLang)) {
        AppendCJKPrefLangs(aPrefLangs, aLen, aCharLang, aPageLang);
    } else {
        AppendPrefLang(aPrefLangs, aLen, aCharLang);
    }

    AppendPrefLang(aPrefLangs, aLen, eFontPrefLang_Others);
}

void
gfxPlatform::AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang)
{
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));

    
    if (IsLangCJK(aPageLang)) {
        AppendPrefLang(aPrefLangs, aLen, aPageLang);
    }
    
    
    if (mCJKPrefLangs.Length() == 0) {
    
        
        eFontPrefLang tempPrefLangs[kMaxLenPrefLangList];
        PRUint32 tempLen = 0;
        
        
        nsCAutoString list;
        if (prefs) {
            nsCOMPtr<nsIPrefLocalizedString> prefString;
            nsresult rv =
                prefs->GetComplexValue("intl.accept_languages",
                                       NS_GET_IID(nsIPrefLocalizedString),
                                       getter_AddRefs(prefString));
            if (NS_SUCCEEDED(rv) && prefString) {
                nsAutoString temp;
                prefString->ToString(getter_Copies(temp));
                LossyCopyUTF16toASCII(temp, list);
            }
        }
        
        if (!list.IsEmpty()) {
            const char kComma = ',';
            const char *p, *p_end;
            list.BeginReading(p);
            list.EndReading(p_end);
            while (p < p_end) {
                while (nsCRT::IsAsciiSpace(*p)) {
                    if (++p == p_end)
                        break;
                }
                if (p == p_end)
                    break;
                const char *start = p;
                while (++p != p_end && *p != kComma)
                     ;
                nsCAutoString lang(Substring(start, p));
                lang.CompressWhitespace(PR_FALSE, PR_TRUE);
                eFontPrefLang fpl = gfxPlatform::GetFontPrefLangFor(lang.get());
                switch (fpl) {
                    case eFontPrefLang_Japanese:
                    case eFontPrefLang_Korean:
                    case eFontPrefLang_ChineseCN:
                    case eFontPrefLang_ChineseHK:
                    case eFontPrefLang_ChineseTW:
                        AppendPrefLang(tempPrefLangs, tempLen, fpl);
                        break;
                    default:
                        break;
                }
                p++;
            }
        }

        do { 
            nsresult rv;
            nsCOMPtr<nsILocaleService> ls =
                do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
            if (NS_FAILED(rv))
                break;

            nsCOMPtr<nsILocale> appLocale;
            rv = ls->GetApplicationLocale(getter_AddRefs(appLocale));
            if (NS_FAILED(rv))
                break;

            nsString localeStr;
            rv = appLocale->
                GetCategory(NS_LITERAL_STRING(NSILOCALE_MESSAGE), localeStr);
            if (NS_FAILED(rv))
                break;

            const nsAString& lang = Substring(localeStr, 0, 2);
            if (lang.EqualsLiteral("ja")) {
                AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_Japanese);
            } else if (lang.EqualsLiteral("zh")) {
                const nsAString& region = Substring(localeStr, 3, 2);
                if (region.EqualsLiteral("CN")) {
                    AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseCN);
                } else if (region.EqualsLiteral("TW")) {
                    AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseTW);
                } else if (region.EqualsLiteral("HK")) {
                    AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseHK);
                }
            } else if (lang.EqualsLiteral("ko")) {
                AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_Korean);
            }
        } while (0);

        
        AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_Japanese);
        AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_Korean);
        AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseCN);
        AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseHK);
        AppendPrefLang(tempPrefLangs, tempLen, eFontPrefLang_ChineseTW);
        
        
        PRUint32 j;
        for (j = 0; j < tempLen; j++) {
            mCJKPrefLangs.AppendElement(tempPrefLangs[j]);
        }
    }
    
    
    PRUint32  i, numCJKlangs = mCJKPrefLangs.Length();
    
    for (i = 0; i < numCJKlangs; i++) {
        AppendPrefLang(aPrefLangs, aLen, (eFontPrefLang) (mCJKPrefLangs[i]));
    }
        
}

void 
gfxPlatform::AppendPrefLang(eFontPrefLang aPrefLangs[], PRUint32& aLen, eFontPrefLang aAddLang)
{
    if (aLen >= kMaxLenPrefLangList) return;
    
    
    PRUint32  i = 0;
    while (i < aLen && aPrefLangs[i] != aAddLang) {
        i++;
    }
    
    if (i == aLen) {
        aPrefLangs[aLen] = aAddLang;
        aLen++;
    }
}

eCMSMode
gfxPlatform::GetCMSMode()
{
    if (gCMSInitialized == PR_FALSE) {
        gCMSInitialized = PR_TRUE;
        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            PRInt32 mode;
            nsresult rv =
                prefs->GetIntPref(CMPrefName, &mode);
            if (NS_SUCCEEDED(rv) && (mode >= 0) && (mode < eCMSMode_AllCount)) {
                gCMSMode = static_cast<eCMSMode>(mode);
            }
        }
    }
    return gCMSMode;
}




#define INTENT_DEFAULT QCMS_INTENT_PERCEPTUAL
#define INTENT_MIN 0
#define INTENT_MAX 3

PRBool
gfxPlatform::GetRenderingIntent()
{
    if (gCMSIntent == -2) {

        
        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            PRInt32 pIntent;
            nsresult rv = prefs->GetIntPref(CMIntentPrefName, &pIntent);
            if (NS_SUCCEEDED(rv)) {
              
                
                if ((pIntent >= INTENT_MIN) && (pIntent <= INTENT_MAX))
                    gCMSIntent = pIntent;

                
                else
                    gCMSIntent = -1;
            }
        }

        
        if (gCMSIntent == -2) 
            gCMSIntent = INTENT_DEFAULT;
    }
    return gCMSIntent;
}

void 
gfxPlatform::TransformPixel(const gfxRGBA& in, gfxRGBA& out, qcms_transform *transform)
{

    if (transform) {
        
#ifdef IS_LITTLE_ENDIAN
        
        PRUint32 packed = in.Packed(gfxRGBA::PACKED_ABGR);
        qcms_transform_data(transform,
                       (PRUint8 *)&packed, (PRUint8 *)&packed,
                       1);
        out.~gfxRGBA();
        new (&out) gfxRGBA(packed, gfxRGBA::PACKED_ABGR);
#else
        
        PRUint32 packed = in.Packed(gfxRGBA::PACKED_ARGB);
        
        qcms_transform_data(transform,
                       (PRUint8 *)&packed + 1, (PRUint8 *)&packed + 1,
                       1);
        out.~gfxRGBA();
        new (&out) gfxRGBA(packed, gfxRGBA::PACKED_ARGB);
#endif
    }

    else if (&out != &in)
        out = in;
}

qcms_profile *
gfxPlatform::GetPlatformCMSOutputProfile()
{
    return nsnull;
}

qcms_profile *
gfxPlatform::GetCMSOutputProfile()
{
    if (!gCMSOutputProfile) {
        NS_TIME_FUNCTION;

        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {

            nsresult rv;

            






            PRBool doSRGBOverride;
            rv = prefs->GetBoolPref(CMForceSRGBPrefName, &doSRGBOverride);
            if (NS_SUCCEEDED(rv) && doSRGBOverride)
                gCMSOutputProfile = GetCMSsRGBProfile();

            if (!gCMSOutputProfile) {

                nsXPIDLCString fname;
                rv = prefs->GetCharPref(CMProfilePrefName,
                                        getter_Copies(fname));
                if (NS_SUCCEEDED(rv) && !fname.IsEmpty()) {
                    gCMSOutputProfile = qcms_profile_from_path(fname);
                }
            }
        }

        if (!gCMSOutputProfile) {
            gCMSOutputProfile =
                gfxPlatform::GetPlatform()->GetPlatformCMSOutputProfile();
        }

        

        if (gCMSOutputProfile && qcms_profile_is_bogus(gCMSOutputProfile)) {
            NS_ASSERTION(gCMSOutputProfile != GetCMSsRGBProfile(),
                         "Builtin sRGB profile tagged as bogus!!!");
            qcms_profile_release(gCMSOutputProfile);
            gCMSOutputProfile = nsnull;
        }

        if (!gCMSOutputProfile) {
            gCMSOutputProfile = GetCMSsRGBProfile();
        }
        

        qcms_profile_precache_output_transform(gCMSOutputProfile);
    }

    return gCMSOutputProfile;
}

qcms_profile *
gfxPlatform::GetCMSsRGBProfile()
{
    if (!gCMSsRGBProfile) {

        
        gCMSsRGBProfile = qcms_profile_sRGB();
    }
    return gCMSsRGBProfile;
}

qcms_transform *
gfxPlatform::GetCMSRGBTransform()
{
    if (!gCMSRGBTransform) {
        qcms_profile *inProfile, *outProfile;
        outProfile = GetCMSOutputProfile();
        inProfile = GetCMSsRGBProfile();

        if (!inProfile || !outProfile)
            return nsnull;

        gCMSRGBTransform = qcms_transform_create(inProfile, QCMS_DATA_RGB_8,
                                              outProfile, QCMS_DATA_RGB_8,
                                             QCMS_INTENT_PERCEPTUAL);
    }

    return gCMSRGBTransform;
}

qcms_transform *
gfxPlatform::GetCMSInverseRGBTransform()
{
    if (!gCMSInverseRGBTransform) {
        qcms_profile *inProfile, *outProfile;
        inProfile = GetCMSOutputProfile();
        outProfile = GetCMSsRGBProfile();

        if (!inProfile || !outProfile)
            return nsnull;

        gCMSInverseRGBTransform = qcms_transform_create(inProfile, QCMS_DATA_RGB_8,
                                                     outProfile, QCMS_DATA_RGB_8,
                                                     QCMS_INTENT_PERCEPTUAL);
    }

    return gCMSInverseRGBTransform;
}

qcms_transform *
gfxPlatform::GetCMSRGBATransform()
{
    if (!gCMSRGBATransform) {
        qcms_profile *inProfile, *outProfile;
        outProfile = GetCMSOutputProfile();
        inProfile = GetCMSsRGBProfile();

        if (!inProfile || !outProfile)
            return nsnull;

        gCMSRGBATransform = qcms_transform_create(inProfile, QCMS_DATA_RGBA_8,
                                               outProfile, QCMS_DATA_RGBA_8,
                                               QCMS_INTENT_PERCEPTUAL);
    }

    return gCMSRGBATransform;
}


static void ShutdownCMS()
{

    if (gCMSRGBTransform) {
        qcms_transform_release(gCMSRGBTransform);
        gCMSRGBTransform = nsnull;
    }
    if (gCMSInverseRGBTransform) {
        qcms_transform_release(gCMSInverseRGBTransform);
        gCMSInverseRGBTransform = nsnull;
    }
    if (gCMSRGBATransform) {
        qcms_transform_release(gCMSRGBATransform);
        gCMSRGBATransform = nsnull;
    }
    if (gCMSOutputProfile) {
        qcms_profile_release(gCMSOutputProfile);

        
        if (gCMSsRGBProfile == gCMSOutputProfile)
            gCMSsRGBProfile = nsnull;
        gCMSOutputProfile = nsnull;
    }
    if (gCMSsRGBProfile) {
        qcms_profile_release(gCMSsRGBProfile);
        gCMSsRGBProfile = nsnull;
    }

    
    gCMSIntent = -2;
    gCMSMode = eCMSMode_Off;
    gCMSInitialized = PR_FALSE;
}

static void MigratePrefs()
{

    

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return;

    

    PRBool hasOldCMPref;
    nsresult rv =
        prefs->PrefHasUserValue(CMPrefNameOld, &hasOldCMPref);
    if (NS_SUCCEEDED(rv) && (hasOldCMPref == PR_TRUE)) {
        PRBool CMWasEnabled;
        rv = prefs->GetBoolPref(CMPrefNameOld, &CMWasEnabled);
        if (NS_SUCCEEDED(rv) && (CMWasEnabled == PR_TRUE))
            prefs->SetIntPref(CMPrefName, eCMSMode_All);
        prefs->ClearUserPref(CMPrefNameOld);
    }

}



void
gfxPlatform::SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) {
        
        
        
        
        
        
        return;
    }

    gfxTextRun::CompressedGlyph extendCluster;
    extendCluster.SetComplex(PR_FALSE, PR_TRUE, 0);

    PRUint32 i, length = aTextRun->GetLength();
    gfxUnicodeProperties::HSType hangulState = gfxUnicodeProperties::HST_NONE;

    for (i = 0; i < length; ++i) {
        PRBool surrogatePair = PR_FALSE;
        PRUint32 ch = aString[i];
        if (NS_IS_HIGH_SURROGATE(ch) &&
            i < length - 1 && NS_IS_LOW_SURROGATE(aString[i+1]))
        {
            ch = SURROGATE_TO_UCS4(ch, aString[i+1]);
            surrogatePair = PR_TRUE;
        }

        PRUint8 category = gfxUnicodeProperties::GetGeneralCategory(ch);
        gfxUnicodeProperties::HSType hangulType = gfxUnicodeProperties::HST_NONE;

        
        if ((category >= HB_CATEGORY_COMBINING_MARK &&
             category <= HB_CATEGORY_NON_SPACING_MARK) ||
            (ch >= 0x200c && ch <= 0x200d) || 
            (ch >= 0xff9e && ch <= 0xff9f))   
        {
            if (i > 0) {
                aTextRun->SetGlyphs(i, extendCluster, nsnull);
            }
        } else if (category == HB_CATEGORY_OTHER_LETTER) {
            
#if 0
            
            
            
            
            

            if ((ch & ~0xff) == 0x0e00) {
                
                if ( ch == 0x0e30 ||
                    (ch >= 0x0e32 && ch <= 0x0e33) ||
                     ch == 0x0e45 ||
                     ch == 0x0eb0 ||
                    (ch >= 0x0eb2 && ch <= 0x0eb3))
                {
                    if (i > 0) {
                        aTextRun->SetGlyphs(i, extendCluster, nsnull);
                    }
                }
                else if ((ch >= 0x0e40 && ch <= 0x0e44) ||
                         (ch >= 0x0ec0 && ch <= 0x0ec4))
                {
                    
                    if (i < length - 1) {
                        aTextRun->SetGlyphs(i+1, extendCluster, nsnull);
                    }
                }
            } else
#endif
            if ((ch & ~0xff) == 0x1100 ||
                (ch >= 0xa960 && ch <= 0xa97f) ||
                (ch >= 0xac00 && ch <= 0xd7ff))
            {
                
                hangulType = gfxUnicodeProperties::GetHangulSyllableType(ch);
                switch (hangulType) {
                case gfxUnicodeProperties::HST_L:
                case gfxUnicodeProperties::HST_LV:
                case gfxUnicodeProperties::HST_LVT:
                    if (hangulState == gfxUnicodeProperties::HST_L) {
                        aTextRun->SetGlyphs(i, extendCluster, nsnull);
                    }
                    break;
                case gfxUnicodeProperties::HST_V:
                    if ( (hangulState != gfxUnicodeProperties::HST_NONE) &&
                        !(hangulState & gfxUnicodeProperties::HST_T))
                    {
                        aTextRun->SetGlyphs(i, extendCluster, nsnull);
                    }
                    break;
                case gfxUnicodeProperties::HST_T:
                    if (hangulState & (gfxUnicodeProperties::HST_V |
                                       gfxUnicodeProperties::HST_T))
                    {
                        aTextRun->SetGlyphs(i, extendCluster, nsnull);
                    }
                    break;
                default:
                    break;
                }
            }
        }

        if (surrogatePair) {
            ++i;
            aTextRun->SetGlyphs(i, extendCluster, nsnull);
        }

        hangulState = hangulType;
    }
}

void
gfxPlatform::FontsPrefsChanged(nsIPrefBranch *aPrefBranch, const char *aPref)
{
    NS_ASSERTION(aPref != nsnull, "null preference");
    if (!strcmp(GFX_DOWNLOADABLE_FONTS_ENABLED, aPref)) {
        mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_DOWNLOADABLE_FONTS_SANITIZE, aPref)) {
        mDownloadableFontsSanitize = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_PREF_HARFBUZZ_SCRIPTS, aPref)) {
        mUseHarfBuzzScripts = UNINITIALIZED_VALUE;
        gfxTextRunWordCache::Flush();
        gfxFontCache::GetCache()->AgeAllGenerations();
    }
}


PRLogModuleInfo*
gfxPlatform::GetLog(eGfxLog aWhichLog)
{
#ifdef PR_LOGGING
    switch (aWhichLog) {
    case eGfxLog_fontlist:
        return sFontlistLog;
        break;
    case eGfxLog_fontinit:
        return sFontInitLog;
        break;
    case eGfxLog_textrun:
        return sTextrunLog;
        break;
    case eGfxLog_textrunui:
        return sTextrunuiLog;
        break;
    default:
        break;
    }

    return nsnull;
#else
    return nsnull;
#endif
}
