




































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxPlatform.h"

#if defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#include "gfxD2DSurface.h"
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
#include "gfxUserFontSet.h"
#include "gfxUnicodeProperties.h"
#include "harfbuzz/hb-unicode.h"
#ifdef MOZ_GRAPHITE
#include "gfxGraphiteShaper.h"
#endif

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
#include "nsCRT.h"
#include "GLContext.h"
#include "GLContextProvider.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/Preferences.h"

#include "nsIGfxInfo.h"

using namespace mozilla;

gfxPlatform *gPlatform = nsnull;
static bool gEverInitialized = false;


static qcms_profile *gCMSOutputProfile = nsnull;
static qcms_profile *gCMSsRGBProfile = nsnull;

static qcms_transform *gCMSRGBTransform = nsnull;
static qcms_transform *gCMSInverseRGBTransform = nsnull;
static qcms_transform *gCMSRGBATransform = nsnull;

static bool gCMSInitialized = false;
static eCMSMode gCMSMode = eCMSMode_Off;
static int gCMSIntent = -2;

static void ShutdownCMS();
static void MigratePrefs();

#include "mozilla/gfx/2D.h"
using namespace mozilla::gfx;


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

#ifdef MOZ_GRAPHITE
#define GFX_PREF_GRAPHITE_SHAPING "gfx.font_rendering.graphite.enabled"
#endif

#define BIDI_NUMERAL_PREF "bidi.numeral"

static const char* kObservedPrefs[] = {
    "gfx.downloadable_fonts.",
    "gfx.font_rendering.",
    "bidi.numeral",
    nsnull
};

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
    if (!someData) {
        NS_ERROR("font pref observer code broken");
        return NS_ERROR_UNEXPECTED;
    }
    NS_ASSERTION(gfxPlatform::GetPlatform(), "the singleton instance has gone");
    gfxPlatform::GetPlatform()->FontsPrefsChanged(NS_ConvertUTF16toUTF8(someData).get());

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
  : mAzureBackendCollector(this, &gfxPlatform::GetAzureBackendInfo)
{
    mUseHarfBuzzScripts = UNINITIALIZED_VALUE;
    mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    mDownloadableFontsSanitize = UNINITIALIZED_VALUE;
#ifdef MOZ_GRAPHITE
    mGraphiteShapingEnabled = UNINITIALIZED_VALUE;
#endif
    mBidiNumeralOption = UNINITIALIZED_VALUE;
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

    
    MigratePrefs();

    
    gPlatform->mSRGBOverrideObserver = new SRGBOverrideObserver();
    Preferences::AddWeakObserver(gPlatform->mSRGBOverrideObserver, "gfx.color_management.force_srgb");

    gPlatform->mFontPrefsObserver = new FontPrefsObserver();
    Preferences::AddStrongObservers(gPlatform->mFontPrefsObserver, kObservedPrefs);

    
    
    nsCOMPtr<nsISupports> forceReg
        = do_CreateInstance("@mozilla.org/gfx/init;1");
}

void
gfxPlatform::Shutdown()
{
    
    
    gfxFontCache::Shutdown();
    gfxFontGroup::Shutdown();
#ifdef MOZ_GRAPHITE
    gfxGraphiteShaper::Shutdown();
#endif
#if defined(XP_MACOSX) || defined(XP_WIN) 
    gfxPlatformFontList::Shutdown();
#endif

    
    ShutdownCMS();

    
    
    if (gPlatform) {
        
        NS_ASSERTION(gPlatform->mSRGBOverrideObserver, "mSRGBOverrideObserver has alreay gone");
        Preferences::RemoveObserver(gPlatform->mSRGBOverrideObserver, "gfx.color_management.force_srgb");
        gPlatform->mSRGBOverrideObserver = nsnull;

        NS_ASSERTION(gPlatform->mFontPrefsObserver, "mFontPrefsObserver has alreay gone");
        Preferences::RemoveObservers(gPlatform->mFontPrefsObserver, kObservedPrefs);
        gPlatform->mFontPrefsObserver = nsnull;
    }

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

cairo_user_data_key_t kDrawTarget;

RefPtr<DrawTarget>
gfxPlatform::CreateDrawTargetForSurface(gfxASurface *aSurface)
{
#ifdef XP_WIN
  if (aSurface->GetType() == gfxASurface::SurfaceTypeD2D) {
    RefPtr<DrawTarget> drawTarget =
      Factory::CreateDrawTargetForD3D10Texture(static_cast<gfxD2DSurface*>(aSurface)->GetTexture(), FORMAT_B8G8R8A8);
    aSurface->SetData(&kDrawTarget, drawTarget, NULL);
    return drawTarget;
  }
#endif

  
  return NULL;
}

cairo_user_data_key_t kSourceSurface;

void SourceBufferDestroy(void *srcBuffer)
{
  static_cast<SourceSurface*>(srcBuffer)->Release();
}

RefPtr<SourceSurface>
gfxPlatform::GetSourceSurfaceForSurface(DrawTarget *aTarget, gfxASurface *aSurface)
{
  void *userData = aSurface->GetData(&kSourceSurface);

  if (userData) {
    return static_cast<SourceSurface*>(userData);
  }

  SurfaceFormat format;
  if (aSurface->GetContentType() == gfxASurface::CONTENT_ALPHA) {
    format = FORMAT_A8;
  } else if (aSurface->GetContentType() == gfxASurface::CONTENT_COLOR) {
    format = FORMAT_B8G8R8X8;
  } else {
    format = FORMAT_B8G8R8A8;
  }

  RefPtr<SourceSurface> srcBuffer;

#ifdef XP_WIN
  if (aSurface->GetType() == gfxASurface::SurfaceTypeD2D) {
    NativeSurface surf;
    surf.mFormat = format;
    surf.mType = NATIVE_SURFACE_D3D10_TEXTURE;
    surf.mSurface = static_cast<gfxD2DSurface*>(aSurface)->GetTexture();
    mozilla::gfx::DrawTarget *dt = static_cast<mozilla::gfx::DrawTarget*>(aSurface->GetData(&kDrawTarget));
    if (dt) {
      dt->Flush();
    }
    srcBuffer = aTarget->CreateSourceSurfaceFromNativeSurface(surf);
  }
#endif

  if (!srcBuffer) {
    nsRefPtr<gfxImageSurface> imgSurface = aSurface->GetAsImageSurface();

    if (!imgSurface) {
      imgSurface = new gfxImageSurface(aSurface->GetSize(), gfxASurface::FormatFromContent(aSurface->GetContentType()));
      nsRefPtr<gfxContext> ctx = new gfxContext(imgSurface);
      ctx->SetSource(aSurface);
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
      ctx->Paint();
    }

    gfxImageFormat cairoFormat = imgSurface->Format();
    switch(cairoFormat) {
      case gfxASurface::ImageFormatARGB32:
        format = FORMAT_B8G8R8A8;
        break;
      case gfxASurface::ImageFormatRGB24:
        format = FORMAT_B8G8R8X8;
        break;
      case gfxASurface::ImageFormatA8:
        format = FORMAT_A8;
        break;
      case gfxASurface::ImageFormatRGB16_565:
        format = FORMAT_R5G6B5;
        break;
      default:
        NS_RUNTIMEABORT("Invalid surface format!");
    }

    srcBuffer = aTarget->CreateSourceSurfaceFromData(imgSurface->Data(),
                                                     IntSize(imgSurface->GetSize().width, imgSurface->GetSize().height),
                                                     imgSurface->Stride(),
                                                     format);
  }

  srcBuffer->AddRef();
  aSurface->SetData(&kSourceSurface, srcBuffer, SourceBufferDestroy);

  return srcBuffer;
}

RefPtr<ScaledFont>
gfxPlatform::GetScaledFontForFont(gfxFont *aFont)
{
  return NULL;
}

cairo_user_data_key_t kDrawSourceSurface;
static void
DataSourceSurfaceDestroy(void *dataSourceSurface)
{
      static_cast<DataSourceSurface*>(dataSourceSurface)->Release();
}

already_AddRefed<gfxASurface>
gfxPlatform::GetThebesSurfaceForDrawTarget(DrawTarget *aTarget)
{
  RefPtr<SourceSurface> source = aTarget->Snapshot();
  RefPtr<DataSourceSurface> data = source->GetDataSurface();

  if (!data) {
    return NULL;
  }

  IntSize size = data->GetSize();
  gfxASurface::gfxImageFormat format = gfxASurface::FormatFromContent(ContentForFormat(data->GetFormat()));
  
  nsRefPtr<gfxImageSurface> image =
    new gfxImageSurface(data->GetData(), gfxIntSize(size.width, size.height),
                        data->Stride(), format);

  image->SetData(&kDrawSourceSurface, data.forget().drop(), DataSourceSurfaceDestroy);
  return image.forget();
}

RefPtr<DrawTarget>
gfxPlatform::CreateOffscreenDrawTarget(const IntSize& aSize, SurfaceFormat aFormat)
{
  BackendType backend;
  if (!SupportsAzure(backend)) {
    return NULL;
  }
  return Factory::CreateDrawTarget(backend, aSize, aFormat); 
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

bool
gfxPlatform::DownloadableFontsEnabled()
{
    if (mAllowDownloadableFonts == UNINITIALIZED_VALUE) {
        mAllowDownloadableFonts =
            Preferences::GetBool(GFX_DOWNLOADABLE_FONTS_ENABLED, false);
    }

    return mAllowDownloadableFonts;
}

bool
gfxPlatform::SanitizeDownloadedFonts()
{
    if (mDownloadableFontsSanitize == UNINITIALIZED_VALUE) {
        mDownloadableFontsSanitize =
            Preferences::GetBool(GFX_DOWNLOADABLE_FONTS_SANITIZE, true);
    }

    return mDownloadableFontsSanitize;
}

#ifdef MOZ_GRAPHITE
bool
gfxPlatform::UseGraphiteShaping()
{
    if (mGraphiteShapingEnabled == UNINITIALIZED_VALUE) {
        mGraphiteShapingEnabled =
            Preferences::GetBool(GFX_PREF_GRAPHITE_SHAPING, false);
    }

    return mGraphiteShapingEnabled;
}
#endif

bool
gfxPlatform::UseHarfBuzzForScript(PRInt32 aScriptCode)
{
    if (mUseHarfBuzzScripts == UNINITIALIZED_VALUE) {
        mUseHarfBuzzScripts = Preferences::GetInt(GFX_PREF_HARFBUZZ_SCRIPTS, HARFBUZZ_SCRIPTS_DEFAULT);
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
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), );

    nsCAutoString prefName, langGroupString;

    aLangGroup->ToUTF8String(langGroupString);

    nsCAutoString genericDotLang;
    if (aGenericName) {
        genericDotLang.Assign(aGenericName);
    } else {
        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroupString);
        genericDotLang = Preferences::GetCString(prefName.get());
    }

    genericDotLang.AppendLiteral(".");
    genericDotLang.Append(langGroupString);

    
    prefName.AssignLiteral("font.name.");
    prefName.Append(genericDotLang);
    nsAdoptingString nameValue = Preferences::GetString(prefName.get());
    if (nameValue) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts += nameValue;
    }

    
    prefName.AssignLiteral("font.name-list.");
    prefName.Append(genericDotLang);
    nsAdoptingString nameListValue = Preferences::GetString(prefName.get());
    if (nameListValue && !nameListValue.Equals(nameValue)) {
        if (!aFonts.IsEmpty())
            aFonts.AppendLiteral(", ");
        aFonts += nameListValue;
    }
}

void
gfxPlatform::GetPrefFonts(nsIAtom *aLanguage, nsString& aFonts, bool aAppendUnicode)
{
    aFonts.Truncate();

    AppendGenericFontFromPref(aFonts, aLanguage, nsnull);
    if (aAppendUnicode)
        AppendGenericFontFromPref(aFonts, gfxAtoms::x_unicode, nsnull);
}

bool gfxPlatform::ForEachPrefFont(eFontPrefLang aLangArray[], PRUint32 aLangArrayLen, PrefFontCallback aCallback,
                                    void *aClosure)
{
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), false);

    PRUint32    i;
    for (i = 0; i < aLangArrayLen; i++) {
        eFontPrefLang prefLang = aLangArray[i];
        const char *langGroup = GetPrefLangName(prefLang);
        
        nsCAutoString prefName;
    
        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroup);
        nsAdoptingCString genericDotLang = Preferences::GetCString(prefName.get());
    
        genericDotLang.AppendLiteral(".");
        genericDotLang.Append(langGroup);
    
        
        prefName.AssignLiteral("font.name.");
        prefName.Append(genericDotLang);
        nsAdoptingCString nameValue = Preferences::GetCString(prefName.get());
        if (nameValue) {
            if (!aCallback(prefLang, NS_ConvertUTF8toUTF16(nameValue), aClosure))
                return false;
        }
    
        
        prefName.AssignLiteral("font.name-list.");
        prefName.Append(genericDotLang);
        nsAdoptingCString nameListValue = Preferences::GetCString(prefName.get());
        if (nameListValue && !nameListValue.Equals(nameValue)) {
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
                fontName.CompressWhitespace(false, true);
                if (!aCallback(prefLang, NS_ConvertUTF8toUTF16(fontName), aClosure))
                    return false;
                p++;
            }
        }
    }

    return true;
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

bool 
gfxPlatform::IsLangCJK(eFontPrefLang aLang)
{
    switch (aLang) {
        case eFontPrefLang_Japanese:
        case eFontPrefLang_ChineseTW:
        case eFontPrefLang_ChineseCN:
        case eFontPrefLang_ChineseHK:
        case eFontPrefLang_Korean:
        case eFontPrefLang_CJKSet:
            return true;
        default:
            return false;
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
    
    if (IsLangCJK(aPageLang)) {
        AppendPrefLang(aPrefLangs, aLen, aPageLang);
    }
    
    
    if (mCJKPrefLangs.Length() == 0) {
    
        
        eFontPrefLang tempPrefLangs[kMaxLenPrefLangList];
        PRUint32 tempLen = 0;
        
        
        nsAdoptingCString list = Preferences::GetLocalizedCString("intl.accept_languages");
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
                lang.CompressWhitespace(false, true);
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

bool
gfxPlatform::UseAzureContentDrawing()
{
  static bool sAzureContentDrawingEnabled;
  static bool sAzureContentDrawingPrefCached = false;

  if (!sAzureContentDrawingPrefCached) {
    sAzureContentDrawingPrefCached = true;
    mozilla::Preferences::AddBoolVarCache(&sAzureContentDrawingEnabled, 
                                          "gfx.content.azure.enabled");
  }

  return sAzureContentDrawingEnabled;
}

eCMSMode
gfxPlatform::GetCMSMode()
{
    if (gCMSInitialized == false) {
        gCMSInitialized = true;
        nsresult rv;

        PRInt32 mode;
        rv = Preferences::GetInt("gfx.color_management.mode", &mode);
        if (NS_SUCCEEDED(rv) && (mode >= 0) && (mode < eCMSMode_AllCount)) {
            gCMSMode = static_cast<eCMSMode>(mode);
        }

        bool enableV4;
        rv = Preferences::GetBool("gfx.color_management.enablev4", &enableV4);
        if (NS_SUCCEEDED(rv) && enableV4) {
            qcms_enable_iccv4();
        }
    }
    return gCMSMode;
}




#define INTENT_DEFAULT QCMS_INTENT_PERCEPTUAL
#define INTENT_MIN 0
#define INTENT_MAX 3

int
gfxPlatform::GetRenderingIntent()
{
    if (gCMSIntent == -2) {

        
        PRInt32 pIntent;
        if (NS_SUCCEEDED(Preferences::GetInt("gfx.color_management.rendering_intent", &pIntent))) {
            
            if ((pIntent >= INTENT_MIN) && (pIntent <= INTENT_MAX)) {
                gCMSIntent = pIntent;
            }
            
            else {
                gCMSIntent = -1;
            }
        }
        
        else {
            gCMSIntent = INTENT_DEFAULT;
        }
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

        






        if (Preferences::GetBool("gfx.color_management.force_srgb", false)) {
            gCMSOutputProfile = GetCMSsRGBProfile();
        }

        if (!gCMSOutputProfile) {
            nsAdoptingCString fname = Preferences::GetCString("gfx.color_management.display_profile");
            if (!fname.IsEmpty()) {
                gCMSOutputProfile = qcms_profile_from_path(fname);
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
    gCMSInitialized = false;
}

static void MigratePrefs()
{
    

    if (Preferences::HasUserValue("gfx.color_management.enabled")) {
        if (Preferences::GetBool("gfx.color_management.enabled", false)) {
            Preferences::SetInt("gfx.color_management.mode", static_cast<PRInt32>(eCMSMode_All));
        }
        Preferences::ClearUser("gfx.color_management.enabled");
    }
}



void
gfxPlatform::SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) {
        
        
        
        
        
        
        return;
    }

    gfxShapedWord::SetupClusterBoundaries(aTextRun->GetCharacterGlyphs(),
                                          aString, aTextRun->GetLength());
}

PRInt32
gfxPlatform::GetBidiNumeralOption()
{
    if (mBidiNumeralOption == UNINITIALIZED_VALUE) {
        mBidiNumeralOption = Preferences::GetInt(BIDI_NUMERAL_PREF, 0);
    }
    return mBidiNumeralOption;
}

void
gfxPlatform::FontsPrefsChanged(const char *aPref)
{
    NS_ASSERTION(aPref != nsnull, "null preference");
    if (!strcmp(GFX_DOWNLOADABLE_FONTS_ENABLED, aPref)) {
        mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_DOWNLOADABLE_FONTS_SANITIZE, aPref)) {
        mDownloadableFontsSanitize = UNINITIALIZED_VALUE;
#ifdef MOZ_GRAPHITE
    } else if (!strcmp(GFX_PREF_GRAPHITE_SHAPING, aPref)) {
        mGraphiteShapingEnabled = UNINITIALIZED_VALUE;
        gfxFontCache::GetCache()->AgeAllGenerations();
#endif
    } else if (!strcmp(GFX_PREF_HARFBUZZ_SCRIPTS, aPref)) {
        mUseHarfBuzzScripts = UNINITIALIZED_VALUE;
        gfxFontCache::GetCache()->AgeAllGenerations();
    } else if (!strcmp(BIDI_NUMERAL_PREF, aPref)) {
        mBidiNumeralOption = UNINITIALIZED_VALUE;
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
