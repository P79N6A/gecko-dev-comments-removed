




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "mozilla/layers/SharedBufferManagerChild.h"
#include "mozilla/layers/ISurfaceAllocator.h"     

#include "prlog.h"

#include "gfxPlatform.h"
#include "gfxPrefs.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include "nsXULAppAPI.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"

#if defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#include "gfxD2DSurface.h"
#elif defined(XP_MACOSX)
#include "gfxPlatformMac.h"
#include "gfxQuartzSurface.h"
#elif defined(MOZ_WIDGET_GTK)
#include "gfxPlatformGtk.h"
#elif defined(MOZ_WIDGET_QT)
#include "gfxQtPlatform.h"
#elif defined(ANDROID)
#include "gfxAndroidPlatform.h"
#endif

#include "nsGkAtoms.h"
#include "gfxPlatformFontList.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsUnicodeProperties.h"
#include "harfbuzz/hb.h"
#include "gfxGraphiteShaper.h"
#include "gfx2DGlue.h"
#include "gfxGradientCache.h"

#include "nsUnicodeRange.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsILocaleService.h"
#include "nsIObserverService.h"
#include "MainThreadUtils.h"

#include "nsWeakReference.h"

#include "cairo.h"
#include "qcms.h"

#include "plstr.h"
#include "nsCRT.h"
#include "GLContext.h"
#include "GLContextProvider.h"

#ifdef MOZ_WIDGET_ANDROID
#include "TexturePoolOGL.h"
#endif

#include "mozilla/Hal.h"
#ifdef USE_SKIA
#include "skia/SkGraphics.h"

#include "SkiaGLGlue.h"
#else
class mozilla::gl::SkiaGLGlue : public GenericAtomicRefCounted {
};
#endif

#include "mozilla/Preferences.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

#include "nsIGfxInfo.h"
#include "nsIXULRuntime.h"

#ifdef MOZ_WIDGET_GONK
namespace mozilla {
namespace layers {
void InitGralloc();
}
}
#endif

using namespace mozilla;
using namespace mozilla::layers;

gfxPlatform *gPlatform = nullptr;
static bool gEverInitialized = false;

static Mutex* gGfxPlatformPrefsLock = nullptr;


static qcms_profile *gCMSOutputProfile = nullptr;
static qcms_profile *gCMSsRGBProfile = nullptr;

static qcms_transform *gCMSRGBTransform = nullptr;
static qcms_transform *gCMSInverseRGBTransform = nullptr;
static qcms_transform *gCMSRGBATransform = nullptr;

static bool gCMSInitialized = false;
static eCMSMode gCMSMode = eCMSMode_Off;

static bool gCMSIntentInitialized = false;
static int gCMSIntent = QCMS_INTENT_DEFAULT;


static void ShutdownCMS();

#include "mozilla/gfx/2D.h"
using namespace mozilla::gfx;



class SRGBOverrideObserver MOZ_FINAL : public nsIObserver,
                                       public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(SRGBOverrideObserver, nsIObserver, nsISupportsWeakReference)

#define GFX_DOWNLOADABLE_FONTS_ENABLED "gfx.downloadable_fonts.enabled"

#define GFX_PREF_FALLBACK_USE_CMAPS  "gfx.font_rendering.fallback.always_use_cmaps"

#define GFX_PREF_OPENTYPE_SVG "gfx.font_rendering.opentype_svg.enabled"

#define GFX_PREF_WORD_CACHE_CHARLIMIT "gfx.font_rendering.wordcache.charlimit"
#define GFX_PREF_WORD_CACHE_MAXENTRIES "gfx.font_rendering.wordcache.maxentries"

#define GFX_PREF_GRAPHITE_SHAPING "gfx.font_rendering.graphite.enabled"

#define BIDI_NUMERAL_PREF "bidi.numeral"

#define GFX_PREF_CMS_FORCE_SRGB "gfx.color_management.force_srgb"

NS_IMETHODIMP
SRGBOverrideObserver::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const char16_t* someData)
{
    NS_ASSERTION(NS_strcmp(someData,
                           MOZ_UTF16(GFX_PREF_CMS_FORCE_SRGB)) == 0,
                 "Restarting CMS on wrong pref!");
    ShutdownCMS();
    return NS_OK;
}

static const char* kObservedPrefs[] = {
    "gfx.downloadable_fonts.",
    "gfx.font_rendering.",
    BIDI_NUMERAL_PREF,
    nullptr
};

class FontPrefsObserver MOZ_FINAL : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(FontPrefsObserver, nsIObserver)

NS_IMETHODIMP
FontPrefsObserver::Observe(nsISupports *aSubject,
                           const char *aTopic,
                           const char16_t *someData)
{
    if (!someData) {
        NS_ERROR("font pref observer code broken");
        return NS_ERROR_UNEXPECTED;
    }
    NS_ASSERTION(gfxPlatform::GetPlatform(), "the singleton instance has gone");
    gfxPlatform::GetPlatform()->FontsPrefsChanged(NS_ConvertUTF16toUTF8(someData).get());

    return NS_OK;
}

class MemoryPressureObserver MOZ_FINAL : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(MemoryPressureObserver, nsIObserver)

NS_IMETHODIMP
MemoryPressureObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const char16_t *someData)
{
    NS_ASSERTION(strcmp(aTopic, "memory-pressure") == 0, "unexpected event topic");
    Factory::PurgeAllCaches();

    gfxPlatform::GetPlatform()->PurgeSkiaCache();
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
};

gfxPlatform::gfxPlatform()
  : mAzureCanvasBackendCollector(MOZ_THIS_IN_INITIALIZER_LIST(),
                                 &gfxPlatform::GetAzureBackendInfo)
{
    mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    mFallbackUsesCmaps = UNINITIALIZED_VALUE;

    mWordCacheCharLimit = UNINITIALIZED_VALUE;
    mWordCacheMaxEntries = UNINITIALIZED_VALUE;
    mGraphiteShapingEnabled = UNINITIALIZED_VALUE;
    mOpenTypeSVGEnabled = UNINITIALIZED_VALUE;
    mBidiNumeralOption = UNINITIALIZED_VALUE;

    mLayersPreferMemoryOverShmem = XRE_GetProcessType() == GeckoProcessType_Default;

    mSkiaGlue = nullptr;

    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO) | BackendTypeBit(BackendType::SKIA);
    uint32_t contentMask = BackendTypeBit(BackendType::CAIRO);
    InitBackendPrefs(canvasMask, BackendType::CAIRO,
                     contentMask, BackendType::CAIRO);

    mUsesOffMainThreadCompositing = ComputeUsesOffMainThreadCompositing();
    mAlreadyShutDownLayersIPC = false;
}

gfxPlatform*
gfxPlatform::GetPlatform()
{
    if (!gPlatform) {
        Init();
    }
    return gPlatform;
}

void RecordingPrefChanged(const char *aPrefName, void *aClosure)
{
  if (Preferences::GetBool("gfx.2d.recording", false)) {
    nsAutoCString fileName;
    nsAdoptingString prefFileName = Preferences::GetString("gfx.2d.recordingfile");

    if (prefFileName) {
      fileName.Append(NS_ConvertUTF16toUTF8(prefFileName));
    } else {
      nsCOMPtr<nsIFile> tmpFile;
      if (NS_FAILED(NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile)))) {
        return;
      }
      fileName.AppendPrintf("moz2drec_%i_%i.aer", XRE_GetProcessType(), getpid());

      nsresult rv = tmpFile->AppendNative(fileName);
      if (NS_FAILED(rv))
        return;

      rv = tmpFile->GetNativePath(fileName);
      if (NS_FAILED(rv))
        return;
    }

    gPlatform->mRecorder = Factory::CreateEventRecorderForFile(fileName.BeginReading());
    printf_stderr("Recording to %s\n", fileName.get());
    Factory::SetGlobalEventRecorder(gPlatform->mRecorder);
  } else {
    Factory::SetGlobalEventRecorder(nullptr);
  }
}

void
gfxPlatform::Init()
{
    if (gEverInitialized) {
        NS_RUNTIMEABORT("Already started???");
    }
    gEverInitialized = true;

    
    gfxPrefs::GetSingleton();

    gGfxPlatformPrefsLock = new Mutex("gfxPlatform::gGfxPlatformPrefsLock");

    AsyncTransactionTrackersHolder::Initialize();

    






    nsCOMPtr<nsIGfxInfo> gfxInfo;
    
    gfxInfo = do_GetService("@mozilla.org/gfx/info;1");

#if defined(XP_WIN)
    gPlatform = new gfxWindowsPlatform;
#elif defined(XP_MACOSX)
    gPlatform = new gfxPlatformMac;
#elif defined(MOZ_WIDGET_GTK)
    gPlatform = new gfxPlatformGtk;
#elif defined(MOZ_WIDGET_QT)
    gPlatform = new gfxQtPlatform;
#elif defined(ANDROID)
    gPlatform = new gfxAndroidPlatform;
#else
    #error "No gfxPlatform implementation available"
#endif

#ifdef DEBUG
    mozilla::gl::GLContext::StaticInit();
#endif

    if (UsesOffMainThreadCompositing() &&
        XRE_GetProcessType() == GeckoProcessType_Default)
    {
        mozilla::layers::CompositorParent::StartUpCompositorThread();
        if (gfxPrefs::AsyncVideoEnabled()) {
            mozilla::layers::ImageBridgeChild::StartUp();
        }
#ifdef MOZ_WIDGET_GONK
        SharedBufferManagerChild::StartUp();
#endif
    }

    nsresult rv;

#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID) 
    rv = gfxPlatformFontList::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxPlatformFontList");
    }
#endif

    gPlatform->mScreenReferenceSurface =
        gPlatform->CreateOffscreenSurface(IntSize(1, 1),
                                          gfxContentType::COLOR_ALPHA);
    if (!gPlatform->mScreenReferenceSurface) {
        NS_RUNTIMEABORT("Could not initialize mScreenReferenceSurface");
    }

    gPlatform->mScreenReferenceDrawTarget =
        gPlatform->CreateOffscreenContentDrawTarget(IntSize(1, 1),
                                                    SurfaceFormat::B8G8R8A8);
    if (!gPlatform->mScreenReferenceDrawTarget) {
      NS_RUNTIMEABORT("Could not initialize mScreenReferenceDrawTarget");
    }

    rv = gfxFontCache::Init();
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Could not initialize gfxFontCache");
    }

    
    gPlatform->mSRGBOverrideObserver = new SRGBOverrideObserver();
    Preferences::AddWeakObserver(gPlatform->mSRGBOverrideObserver, GFX_PREF_CMS_FORCE_SRGB);

    gPlatform->mFontPrefsObserver = new FontPrefsObserver();
    Preferences::AddStrongObservers(gPlatform->mFontPrefsObserver, kObservedPrefs);

    mozilla::gl::GLContext::PlatformStartup();

#ifdef MOZ_WIDGET_ANDROID
    
    mozilla::gl::TexturePoolOGL::Init();
#endif

#ifdef MOZ_WIDGET_GONK
    mozilla::layers::InitGralloc();
#endif

    Preferences::RegisterCallbackAndCall(RecordingPrefChanged, "gfx.2d.recording", nullptr);

    CreateCMSOutputProfile();

    
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        gPlatform->mMemoryPressureObserver = new MemoryPressureObserver();
        obs->AddObserver(gPlatform->mMemoryPressureObserver, "memory-pressure", false);
    }

    RegisterStrongMemoryReporter(new GfxMemoryImageReporter());
}

void
gfxPlatform::Shutdown()
{
    MOZ_ASSERT(gPlatform, "gfxPlatform already down!");
    MOZ_ASSERT(gPlatform->mAlreadyShutDownLayersIPC, "ShutdownLayersIPC should have been called before this point!");

    
    
    gfxFontCache::Shutdown();
    gfxFontGroup::Shutdown();
    gfxGradientCache::Shutdown();
    gfxGraphiteShaper::Shutdown();
#if defined(XP_MACOSX) || defined(XP_WIN) 
    gfxPlatformFontList::Shutdown();
#endif

    
    ShutdownCMS();

    
    
    if (gPlatform) {
        
        NS_ASSERTION(gPlatform->mSRGBOverrideObserver, "mSRGBOverrideObserver has alreay gone");
        Preferences::RemoveObserver(gPlatform->mSRGBOverrideObserver, GFX_PREF_CMS_FORCE_SRGB);
        gPlatform->mSRGBOverrideObserver = nullptr;

        NS_ASSERTION(gPlatform->mFontPrefsObserver, "mFontPrefsObserver has alreay gone");
        Preferences::RemoveObservers(gPlatform->mFontPrefsObserver, kObservedPrefs);
        gPlatform->mFontPrefsObserver = nullptr;

        NS_ASSERTION(gPlatform->mMemoryPressureObserver, "mMemoryPressureObserver has already gone");
        nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
        if (obs) {
            obs->RemoveObserver(gPlatform->mMemoryPressureObserver, "memory-pressure");
        }

        gPlatform->mMemoryPressureObserver = nullptr;
        gPlatform->mSkiaGlue = nullptr;
    }

#ifdef MOZ_WIDGET_ANDROID
    
    mozilla::gl::TexturePoolOGL::Shutdown();
#endif

    
    mozilla::gl::GLContextProvider::Shutdown();

#if defined(XP_WIN)
    
    
    
    
    
    
    mozilla::gl::GLContextProviderEGL::Shutdown();
#endif

    delete gGfxPlatformPrefsLock;

    gfxPrefs::DestroySingleton();
    gfxFont::DestroySingletons();

    delete gPlatform;
    gPlatform = nullptr;
}

 void
gfxPlatform::ShutdownLayersIPC()
{
    MOZ_ASSERT(!gPlatform->mAlreadyShutDownLayersIPC);
    if (UsesOffMainThreadCompositing() &&
        XRE_GetProcessType() == GeckoProcessType_Default)
    {
        
        
        layers::ImageBridgeChild::ShutDown();
#ifdef MOZ_WIDGET_GONK
        layers::SharedBufferManagerChild::ShutDown();
#endif

        layers::CompositorParent::AllowShutdownOfCompositorThreadWhenUnreferenced();
    }
    gPlatform->mAlreadyShutDownLayersIPC = true;
}

gfxPlatform::~gfxPlatform()
{
    mScreenReferenceSurface = nullptr;
    mScreenReferenceDrawTarget = nullptr;

    
    
    
    
    
    
#if defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING) || defined(NS_TRACE_MALLOC) || defined(MOZ_VALGRIND)
#ifdef USE_SKIA
    
    
    SkGraphics::Term();
#endif

#if MOZ_TREE_CAIRO
    cairo_debug_reset_static_data();
#endif
#endif
}

bool
gfxPlatform::PreferMemoryOverShmem() const {
  return mLayersPreferMemoryOverShmem;
}

already_AddRefed<gfxASurface>
gfxPlatform::OptimizeImage(gfxImageSurface *aSurface,
                           gfxImageFormat format)
{
    IntSize surfaceSize = aSurface->GetSize().ToIntSize();

#ifdef XP_WIN
    if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
        gfxWindowsPlatform::RENDER_DIRECT2D) {
        return nullptr;
    }
#endif
    nsRefPtr<gfxASurface> optSurface = CreateOffscreenSurface(surfaceSize, gfxASurface::ContentFromFormat(format));
    if (!optSurface || optSurface->CairoStatus() != 0)
        return nullptr;

    gfxContext tmpCtx(optSurface);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx.SetSource(aSurface);
    tmpCtx.Paint();

    return optSurface.forget();
}

cairo_user_data_key_t kDrawTarget;

RefPtr<DrawTarget>
gfxPlatform::CreateDrawTargetForSurface(gfxASurface *aSurface, const IntSize& aSize)
{
  SurfaceFormat format = Optimal2DFormatForContent(aSurface->GetContentType());
  RefPtr<DrawTarget> drawTarget = Factory::CreateDrawTargetForCairoSurface(aSurface->CairoSurface(), aSize, &format);
  aSurface->SetData(&kDrawTarget, drawTarget, nullptr);
  return drawTarget;
}




RefPtr<DrawTarget>
gfxPlatform::CreateDrawTargetForUpdateSurface(gfxASurface *aSurface, const IntSize& aSize)
{
#ifdef XP_MACOSX
  
  
  if (aSurface->GetType() == gfxSurfaceType::Quartz) {
    return Factory::CreateDrawTargetForCairoCGContext(static_cast<gfxQuartzSurface*>(aSurface)->GetCGContext(), aSize);
  }
#endif
  MOZ_CRASH();
  return nullptr;
}


cairo_user_data_key_t kSourceSurface;








struct SourceSurfaceUserData
{
  RefPtr<SourceSurface> mSrcSurface;
  BackendType mBackendType;
};

void SourceBufferDestroy(void *srcSurfUD)
{
  delete static_cast<SourceSurfaceUserData*>(srcSurfUD);
}

UserDataKey kThebesSurface;

struct DependentSourceSurfaceUserData
{
  nsRefPtr<gfxASurface> mSurface;
};

void SourceSurfaceDestroyed(void *aData)
{
  delete static_cast<DependentSourceSurfaceUserData*>(aData);
}

#if MOZ_TREE_CAIRO
void SourceSnapshotDetached(cairo_surface_t *nullSurf)
{
  gfxImageSurface* origSurf =
    static_cast<gfxImageSurface*>(cairo_surface_get_user_data(nullSurf, &kSourceSurface));

  origSurf->SetData(&kSourceSurface, nullptr, nullptr);
}
#else
void SourceSnapshotDetached(void *nullSurf)
{
  gfxImageSurface* origSurf = static_cast<gfxImageSurface*>(nullSurf);
  origSurf->SetData(&kSourceSurface, nullptr, nullptr);
}
#endif

void
gfxPlatform::ClearSourceSurfaceForSurface(gfxASurface *aSurface)
{
  aSurface->SetData(&kSourceSurface, nullptr, nullptr);
}

static TemporaryRef<DataSourceSurface>
CopySurface(gfxASurface* aSurface)
{
  const nsIntSize size = aSurface->GetSize();
  gfxImageFormat format = gfxPlatform::GetPlatform()->OptimalFormatForContent(aSurface->GetContentType());
  RefPtr<DataSourceSurface> data =
    Factory::CreateDataSourceSurface(ToIntSize(size),
                                     ImageFormatToSurfaceFormat(format));
  if (!data) {
    return nullptr;
  }

  DataSourceSurface::MappedSurface map;
  DebugOnly<bool> result = data->Map(DataSourceSurface::WRITE, &map);
  MOZ_ASSERT(result, "Should always succeed mapping raw data surfaces!");

  nsRefPtr<gfxImageSurface> image = new gfxImageSurface(map.mData, size, map.mStride, format);
  nsRefPtr<gfxContext> ctx = new gfxContext(image);

  ctx->SetSource(aSurface);
  ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx->Paint();

  data->Unmap();

  return data;
}

 RefPtr<SourceSurface>
gfxPlatform::GetSourceSurfaceForSurface(DrawTarget *aTarget, gfxASurface *aSurface)
{
  if (!aSurface->CairoSurface() || aSurface->CairoStatus()) {
    return nullptr;
  }

  if (!aTarget) {
    if (gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget()) {
      aTarget = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
    } else {
      return nullptr;
    }
  }

  void *userData = aSurface->GetData(&kSourceSurface);

  if (userData) {
    SourceSurfaceUserData *surf = static_cast<SourceSurfaceUserData*>(userData);

    if (surf->mSrcSurface->IsValid() && surf->mBackendType == aTarget->GetType()) {
      return surf->mSrcSurface;
    }
    
    
  }

  SurfaceFormat format;
  if (aSurface->GetContentType() == gfxContentType::ALPHA) {
    format = SurfaceFormat::A8;
  } else if (aSurface->GetContentType() == gfxContentType::COLOR) {
    format = SurfaceFormat::B8G8R8X8;
  } else {
    format = SurfaceFormat::B8G8R8A8;
  }

  RefPtr<SourceSurface> srcBuffer;

#ifdef XP_WIN
  if (aSurface->GetType() == gfxSurfaceType::D2D &&
      format != SurfaceFormat::A8) {
    NativeSurface surf;
    surf.mFormat = format;
    surf.mType = NativeSurfaceType::D3D10_TEXTURE;
    surf.mSurface = static_cast<gfxD2DSurface*>(aSurface)->GetTexture();
    surf.mSize = ToIntSize(aSurface->GetSize());
    mozilla::gfx::DrawTarget *dt = static_cast<mozilla::gfx::DrawTarget*>(aSurface->GetData(&kDrawTarget));
    if (dt) {
      dt->Flush();
    }
    srcBuffer = aTarget->CreateSourceSurfaceFromNativeSurface(surf);
  } else
#endif
  if (aSurface->CairoSurface() && aTarget->GetType() == BackendType::CAIRO) {
    
    
    NativeSurface surf;
    surf.mFormat = format;
    surf.mType = NativeSurfaceType::CAIRO_SURFACE;
    surf.mSurface = aSurface->CairoSurface();
    surf.mSize = ToIntSize(aSurface->GetSize());
    srcBuffer = aTarget->CreateSourceSurfaceFromNativeSurface(surf);

    if (srcBuffer) {
      
      
      return srcBuffer;
    }
  }

  if (!srcBuffer) {
    nsRefPtr<gfxImageSurface> imgSurface = aSurface->GetAsImageSurface();

    RefPtr<DataSourceSurface> dataSurf;

    if (imgSurface) {
      dataSurf = GetWrappedDataSourceSurface(aSurface);
    } else {
      dataSurf = CopySurface(aSurface);
    }

    if (!dataSurf) {
      return nullptr;
    }

    srcBuffer = aTarget->OptimizeSourceSurface(dataSurf);

    if (imgSurface && srcBuffer == dataSurf) {
      
      
      
     return srcBuffer;
    }
  }

  
  SourceSurfaceUserData *srcSurfUD = new SourceSurfaceUserData;
  srcSurfUD->mBackendType = aTarget->GetType();
  srcSurfUD->mSrcSurface = srcBuffer;
  aSurface->SetData(&kSourceSurface, srcSurfUD, SourceBufferDestroy);

  return srcBuffer;
}

RefPtr<DataSourceSurface>
gfxPlatform::GetWrappedDataSourceSurface(gfxASurface* aSurface)
{
  nsRefPtr<gfxImageSurface> image = aSurface->GetAsImageSurface();
  if (!image) {
    return nullptr;
  }
  RefPtr<DataSourceSurface> result =
    Factory::CreateWrappingDataSourceSurface(image->Data(),
                                             image->Stride(),
                                             ToIntSize(image->GetSize()),
                                             ImageFormatToSurfaceFormat(image->Format()));

  if (!result) {
    return nullptr;
  }

  
  
  DependentSourceSurfaceUserData *srcSurfUD = new DependentSourceSurfaceUserData;
  srcSurfUD->mSurface = aSurface;
  result->AddUserData(&kThebesSurface, srcSurfUD, SourceSurfaceDestroyed);

  return result;
}

TemporaryRef<ScaledFont>
gfxPlatform::GetScaledFontForFont(DrawTarget* aTarget, gfxFont *aFont)
{
  NativeFont nativeFont;
  nativeFont.mType = NativeFontType::CAIRO_FONT_FACE;
  nativeFont.mFont = aFont->GetCairoScaledFont();
  RefPtr<ScaledFont> scaledFont =
    Factory::CreateScaledFontForNativeFont(nativeFont,
                                           aFont->GetAdjustedSize());
  return scaledFont;
}

cairo_user_data_key_t kDrawSourceSurface;
static void
DataSourceSurfaceDestroy(void *dataSourceSurface)
{
  static_cast<DataSourceSurface*>(dataSourceSurface)->Release();
}

cairo_user_data_key_t kDrawTargetForSurface;
static void
DataDrawTargetDestroy(void *aTarget)
{
  static_cast<DrawTarget*>(aTarget)->Release();
}

bool
gfxPlatform::SupportsAzureContentForDrawTarget(DrawTarget* aTarget)
{
  if (!aTarget) {
    return false;
  }

  return SupportsAzureContentForType(aTarget->GetType());
}

bool
gfxPlatform::UseAcceleratedSkiaCanvas()
{
  return gfxPrefs::CanvasAzureAccelerated() &&
         mPreferredCanvasBackend == BackendType::SKIA;
}

void
gfxPlatform::InitializeSkiaCacheLimits()
{
  if (UseAcceleratedSkiaCanvas()) {
    bool usingDynamicCache = gfxPrefs::CanvasSkiaGLDynamicCache();
    int cacheItemLimit = gfxPrefs::CanvasSkiaGLCacheItems();
    int cacheSizeLimit = gfxPrefs::CanvasSkiaGLCacheSize();

    
    cacheSizeLimit *= 1024*1024;

    if (usingDynamicCache) {
      uint32_t totalMemory = mozilla::hal::GetTotalSystemMemory();

      if (totalMemory <= 256*1024*1024) {
        
        cacheSizeLimit = 2*1024*1024;
      } else if (totalMemory > 0) {
        cacheSizeLimit = totalMemory / 16;
      }
    }

  #ifdef DEBUG
    printf_stderr("Determined SkiaGL cache limits: Size %i, Items: %i\n", cacheSizeLimit, cacheItemLimit);
  #endif

#ifdef USE_SKIA_GPU
    mSkiaGlue->GetGrContext()->setTextureCacheLimits(cacheItemLimit, cacheSizeLimit);
#endif
  }
}

mozilla::gl::SkiaGLGlue*
gfxPlatform::GetSkiaGLGlue()
{
#ifdef USE_SKIA_GPU
  if (!mSkiaGlue) {
    




    mozilla::gfx::SurfaceCaps caps = mozilla::gfx::SurfaceCaps::ForRGBA();
    nsRefPtr<mozilla::gl::GLContext> glContext = mozilla::gl::GLContextProvider::CreateOffscreen(gfxIntSize(16, 16), caps);
    if (!glContext) {
      printf_stderr("Failed to create GLContext for SkiaGL!\n");
      return nullptr;
    }
    mSkiaGlue = new mozilla::gl::SkiaGLGlue(glContext);
    MOZ_ASSERT(mSkiaGlue->GetGrContext(), "No GrContext");
    InitializeSkiaCacheLimits();
  }
#endif

  return mSkiaGlue;
}

void
gfxPlatform::PurgeSkiaCache()
{
#ifdef USE_SKIA_GPU
  if (!mSkiaGlue)
      return;

  mSkiaGlue->GetGrContext()->freeGpuResources();
#endif
}

already_AddRefed<gfxASurface>
gfxPlatform::GetThebesSurfaceForDrawTarget(DrawTarget *aTarget)
{
  if (aTarget->GetType() == BackendType::CAIRO) {
    cairo_surface_t* csurf =
      static_cast<cairo_surface_t*>(aTarget->GetNativeSurface(NativeSurfaceType::CAIRO_SURFACE));
    if (csurf) {
      return gfxASurface::Wrap(csurf);
    }
  }

  
  
  
  
  
  RefPtr<SourceSurface> source = aTarget->Snapshot();
  RefPtr<DataSourceSurface> data = source->GetDataSurface();

  if (!data) {
    return nullptr;
  }

  IntSize size = data->GetSize();
  gfxImageFormat format = SurfaceFormatToImageFormat(data->GetFormat());


  nsRefPtr<gfxASurface> surf =
    new gfxImageSurface(data->GetData(), gfxIntSize(size.width, size.height),
                        data->Stride(), format);

  if (surf->CairoStatus()) {
    return nullptr;
  }

  surf->SetData(&kDrawSourceSurface, data.forget().drop(), DataSourceSurfaceDestroy);
  
  aTarget->AddRef();
  surf->SetData(&kDrawTargetForSurface, aTarget, DataDrawTargetDestroy);

  return surf.forget();
}

RefPtr<DrawTarget>
gfxPlatform::CreateDrawTargetForBackend(BackendType aBackend, const IntSize& aSize, SurfaceFormat aFormat)
{
  
  
  
  
  
  
  
  
  if (aBackend == BackendType::CAIRO) {
    nsRefPtr<gfxASurface> surf = CreateOffscreenSurface(aSize,
                                                        ContentForFormat(aFormat));
    if (!surf || surf->CairoStatus()) {
      return nullptr;
    }

    return CreateDrawTargetForSurface(surf, aSize);
  } else {
    return Factory::CreateDrawTarget(aBackend, aSize, aFormat);
  }
}

RefPtr<DrawTarget>
gfxPlatform::CreateOffscreenCanvasDrawTarget(const IntSize& aSize, SurfaceFormat aFormat)
{
  NS_ASSERTION(mPreferredCanvasBackend != BackendType::NONE, "No backend.");
  RefPtr<DrawTarget> target = CreateDrawTargetForBackend(mPreferredCanvasBackend, aSize, aFormat);
  if (target ||
      mFallbackCanvasBackend == BackendType::NONE) {
    return target;
  }

  return CreateDrawTargetForBackend(mFallbackCanvasBackend, aSize, aFormat);
}

RefPtr<DrawTarget>
gfxPlatform::CreateOffscreenContentDrawTarget(const IntSize& aSize, SurfaceFormat aFormat)
{
  NS_ASSERTION(mPreferredCanvasBackend != BackendType::NONE, "No backend.");
  return CreateDrawTargetForBackend(mContentBackend, aSize, aFormat);
}

RefPtr<DrawTarget>
gfxPlatform::CreateDrawTargetForData(unsigned char* aData, const IntSize& aSize, int32_t aStride, SurfaceFormat aFormat)
{
  NS_ASSERTION(mContentBackend != BackendType::NONE, "No backend.");
  if (mContentBackend == BackendType::CAIRO) {
    nsRefPtr<gfxImageSurface> image = new gfxImageSurface(aData, gfxIntSize(aSize.width, aSize.height), aStride, SurfaceFormatToImageFormat(aFormat));
    return Factory::CreateDrawTargetForCairoSurface(image->CairoSurface(), aSize);
  }
  return Factory::CreateDrawTargetForData(mContentBackend, aData, aSize, aStride, aFormat);
}

 BackendType
gfxPlatform::BackendTypeForName(const nsCString& aName)
{
  if (aName.EqualsLiteral("cairo"))
    return BackendType::CAIRO;
  if (aName.EqualsLiteral("skia"))
    return BackendType::SKIA;
  if (aName.EqualsLiteral("direct2d"))
    return BackendType::DIRECT2D;
  if (aName.EqualsLiteral("cg"))
    return BackendType::COREGRAPHICS;
  return BackendType::NONE;
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
gfxPlatform::UseCmapsDuringSystemFallback()
{
    if (mFallbackUsesCmaps == UNINITIALIZED_VALUE) {
        mFallbackUsesCmaps =
            Preferences::GetBool(GFX_PREF_FALLBACK_USE_CMAPS, false);
    }

    return mFallbackUsesCmaps;
}

bool
gfxPlatform::OpenTypeSVGEnabled()
{
    if (mOpenTypeSVGEnabled == UNINITIALIZED_VALUE) {
        mOpenTypeSVGEnabled =
            Preferences::GetBool(GFX_PREF_OPENTYPE_SVG, false);
    }

    return mOpenTypeSVGEnabled > 0;
}

uint32_t
gfxPlatform::WordCacheCharLimit()
{
    if (mWordCacheCharLimit == UNINITIALIZED_VALUE) {
        mWordCacheCharLimit =
            Preferences::GetInt(GFX_PREF_WORD_CACHE_CHARLIMIT, 32);
        if (mWordCacheCharLimit < 0) {
            mWordCacheCharLimit = 32;
        }
    }

    return uint32_t(mWordCacheCharLimit);
}

uint32_t
gfxPlatform::WordCacheMaxEntries()
{
    if (mWordCacheMaxEntries == UNINITIALIZED_VALUE) {
        mWordCacheMaxEntries =
            Preferences::GetInt(GFX_PREF_WORD_CACHE_MAXENTRIES, 10000);
        if (mWordCacheMaxEntries < 0) {
            mWordCacheMaxEntries = 10000;
        }
    }

    return uint32_t(mWordCacheMaxEntries);
}

bool
gfxPlatform::UseGraphiteShaping()
{
    if (mGraphiteShapingEnabled == UNINITIALIZED_VALUE) {
        mGraphiteShapingEnabled =
            Preferences::GetBool(GFX_PREF_GRAPHITE_SHAPING, false);
    }

    return mGraphiteShapingEnabled;
}

gfxFontEntry*
gfxPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                              const uint8_t *aFontData,
                              uint32_t aLength)
{
    
    
    
    
    
    if (aFontData) {
        NS_Free((void*)aFontData);
    }
    return nullptr;
}

static void
AppendGenericFontFromPref(nsString& aFonts, nsIAtom *aLangGroup, const char *aGenericName)
{
    NS_ENSURE_TRUE_VOID(Preferences::GetRootBranch());

    nsAutoCString prefName, langGroupString;

    aLangGroup->ToUTF8String(langGroupString);

    nsAutoCString genericDotLang;
    if (aGenericName) {
        genericDotLang.Assign(aGenericName);
    } else {
        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroupString);
        genericDotLang = Preferences::GetCString(prefName.get());
    }

    genericDotLang.Append('.');
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

    AppendGenericFontFromPref(aFonts, aLanguage, nullptr);
    if (aAppendUnicode)
        AppendGenericFontFromPref(aFonts, nsGkAtoms::Unicode, nullptr);
}

bool gfxPlatform::ForEachPrefFont(eFontPrefLang aLangArray[], uint32_t aLangArrayLen, PrefFontCallback aCallback,
                                    void *aClosure)
{
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), false);

    uint32_t    i;
    for (i = 0; i < aLangArrayLen; i++) {
        eFontPrefLang prefLang = aLangArray[i];
        const char *langGroup = GetPrefLangName(prefLang);

        nsAutoCString prefName;

        prefName.AssignLiteral("font.default.");
        prefName.Append(langGroup);
        nsAdoptingCString genericDotLang = Preferences::GetCString(prefName.get());

        genericDotLang.Append('.');
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
            nsAutoCString list(nameListValue);
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
                nsAutoCString fontName(Substring(start, p));
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
    if (!aLang || !aLang[0]) {
        return eFontPrefLang_Others;
    }
    for (uint32_t i = 0; i < ArrayLength(gPrefLangNames); ++i) {
        if (!PL_strcasecmp(gPrefLangNames[i], aLang)) {
            return eFontPrefLang(i);
        }
    }
    return eFontPrefLang_Others;
}

eFontPrefLang
gfxPlatform::GetFontPrefLangFor(nsIAtom *aLang)
{
    if (!aLang)
        return eFontPrefLang_Others;
    nsAutoCString lang;
    aLang->ToUTF8String(lang);
    return GetFontPrefLangFor(lang.get());
}

const char*
gfxPlatform::GetPrefLangName(eFontPrefLang aLang)
{
    if (uint32_t(aLang) < ArrayLength(gPrefLangNames)) {
        return gPrefLangNames[uint32_t(aLang)];
    }
    return nullptr;
}

eFontPrefLang
gfxPlatform::GetFontPrefLangFor(uint8_t aUnicodeRange)
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

mozilla::layers::DiagnosticTypes
gfxPlatform::GetLayerDiagnosticTypes()
{
  mozilla::layers::DiagnosticTypes type = DiagnosticTypes::NO_DIAGNOSTIC;
  if (gfxPrefs::DrawLayerBorders()) {
    type |= mozilla::layers::DiagnosticTypes::LAYER_BORDERS;
  }
  if (gfxPrefs::DrawTileBorders()) {
    type |= mozilla::layers::DiagnosticTypes::TILE_BORDERS;
  }
  if (gfxPrefs::DrawBigImageBorders()) {
    type |= mozilla::layers::DiagnosticTypes::BIGIMAGE_BORDERS;
  }
  if (gfxPrefs::FlashLayerBorders()) {
    type |= mozilla::layers::DiagnosticTypes::FLASH_BORDERS;
  }
  return type;
}

void
gfxPlatform::GetLangPrefs(eFontPrefLang aPrefLangs[], uint32_t &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang)
{
    if (IsLangCJK(aCharLang)) {
        AppendCJKPrefLangs(aPrefLangs, aLen, aCharLang, aPageLang);
    } else {
        AppendPrefLang(aPrefLangs, aLen, aCharLang);
    }

    AppendPrefLang(aPrefLangs, aLen, eFontPrefLang_Others);
}

void
gfxPlatform::AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], uint32_t &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang)
{
    
    if (IsLangCJK(aPageLang)) {
        AppendPrefLang(aPrefLangs, aLen, aPageLang);
    }

    
    if (mCJKPrefLangs.Length() == 0) {

        
        eFontPrefLang tempPrefLangs[kMaxLenPrefLangList];
        uint32_t tempLen = 0;

        
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
                nsAutoCString lang(Substring(start, p));
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

        
        uint32_t j;
        for (j = 0; j < tempLen; j++) {
            mCJKPrefLangs.AppendElement(tempPrefLangs[j]);
        }
    }

    
    uint32_t  i, numCJKlangs = mCJKPrefLangs.Length();

    for (i = 0; i < numCJKlangs; i++) {
        AppendPrefLang(aPrefLangs, aLen, (eFontPrefLang) (mCJKPrefLangs[i]));
    }

}

void
gfxPlatform::AppendPrefLang(eFontPrefLang aPrefLangs[], uint32_t& aLen, eFontPrefLang aAddLang)
{
    if (aLen >= kMaxLenPrefLangList) return;

    
    uint32_t  i = 0;
    while (i < aLen && aPrefLangs[i] != aAddLang) {
        i++;
    }

    if (i == aLen) {
        aPrefLangs[aLen] = aAddLang;
        aLen++;
    }
}

void
gfxPlatform::InitBackendPrefs(uint32_t aCanvasBitmask, BackendType aCanvasDefault,
                              uint32_t aContentBitmask, BackendType aContentDefault)
{
    mPreferredCanvasBackend = GetCanvasBackendPref(aCanvasBitmask);
    if (mPreferredCanvasBackend == BackendType::NONE) {
        mPreferredCanvasBackend = aCanvasDefault;
    }
    mFallbackCanvasBackend =
        GetCanvasBackendPref(aCanvasBitmask & ~BackendTypeBit(mPreferredCanvasBackend));

    mContentBackendBitmask = aContentBitmask;
    mContentBackend = GetContentBackendPref(mContentBackendBitmask);
    if (mContentBackend == BackendType::NONE) {
        mContentBackend = aContentDefault;
        
        
        
        mContentBackendBitmask |= BackendTypeBit(aContentDefault);
    }
}

 BackendType
gfxPlatform::GetCanvasBackendPref(uint32_t aBackendBitmask)
{
    return GetBackendPref("gfx.canvas.azure.backends", aBackendBitmask);
}

 BackendType
gfxPlatform::GetContentBackendPref(uint32_t &aBackendBitmask)
{
    return GetBackendPref("gfx.content.azure.backends", aBackendBitmask);
}

 BackendType
gfxPlatform::GetBackendPref(const char* aBackendPrefName, uint32_t &aBackendBitmask)
{
    nsTArray<nsCString> backendList;
    nsCString prefString;
    if (NS_SUCCEEDED(Preferences::GetCString(aBackendPrefName, &prefString))) {
        ParseString(prefString, ',', backendList);
    }

    uint32_t allowedBackends = 0;
    BackendType result = BackendType::NONE;
    for (uint32_t i = 0; i < backendList.Length(); ++i) {
        BackendType type = BackendTypeForName(backendList[i]);
        if (BackendTypeBit(type) & aBackendBitmask) {
            allowedBackends |= BackendTypeBit(type);
            if (result == BackendType::NONE) {
                result = type;
            }
        }
    }

    aBackendBitmask = allowedBackends;
    return result;
}

bool
gfxPlatform::OffMainThreadCompositingEnabled()
{
  return UsesOffMainThreadCompositing();
}

eCMSMode
gfxPlatform::GetCMSMode()
{
    if (gCMSInitialized == false) {
        gCMSInitialized = true;

        int32_t mode = gfxPrefs::CMSMode();
        if (mode >= 0 && mode < eCMSMode_AllCount) {
            gCMSMode = static_cast<eCMSMode>(mode);
        }

        bool enableV4 = gfxPrefs::CMSEnableV4();
        if (enableV4) {
            qcms_enable_iccv4();
        }
    }
    return gCMSMode;
}

int
gfxPlatform::GetRenderingIntent()
{
    if (!gCMSIntentInitialized) {
        gCMSIntentInitialized = true;

        
        
        
        
        MOZ_ASSERT(QCMS_INTENT_DEFAULT == 0);

        
        int32_t pIntent = gfxPrefs::CMSRenderingIntent();
        if ((pIntent >= QCMS_INTENT_MIN) && (pIntent <= QCMS_INTENT_MAX)) {
            gCMSIntent = pIntent;
        } else {
            
            gCMSIntent = -1;
        }
    }
    return gCMSIntent;
}

void
gfxPlatform::TransformPixel(const gfxRGBA& in, gfxRGBA& out, qcms_transform *transform)
{

    if (transform) {
        
#ifdef IS_LITTLE_ENDIAN
        
        uint32_t packed = in.Packed(gfxRGBA::PACKED_ABGR);
        qcms_transform_data(transform,
                       (uint8_t *)&packed, (uint8_t *)&packed,
                       1);
        out.~gfxRGBA();
        new (&out) gfxRGBA(packed, gfxRGBA::PACKED_ABGR);
#else
        
        uint32_t packed = in.Packed(gfxRGBA::PACKED_ARGB);
        
        qcms_transform_data(transform,
                       (uint8_t *)&packed + 1, (uint8_t *)&packed + 1,
                       1);
        out.~gfxRGBA();
        new (&out) gfxRGBA(packed, gfxRGBA::PACKED_ARGB);
#endif
    }

    else if (&out != &in)
        out = in;
}

void
gfxPlatform::GetPlatformCMSOutputProfile(void *&mem, size_t &size)
{
    mem = nullptr;
    size = 0;
}

void
gfxPlatform::GetCMSOutputProfileData(void *&mem, size_t &size)
{
    nsAdoptingCString fname = Preferences::GetCString("gfx.color_management.display_profile");
    if (!fname.IsEmpty()) {
        qcms_data_from_path(fname, &mem, &size);
    }
    else {
        gfxPlatform::GetPlatform()->GetPlatformCMSOutputProfile(mem, size);
    }
}

void
gfxPlatform::CreateCMSOutputProfile()
{
    if (!gCMSOutputProfile) {
        






        if (Preferences::GetBool(GFX_PREF_CMS_FORCE_SRGB, false)) {
            gCMSOutputProfile = GetCMSsRGBProfile();
        }

        if (!gCMSOutputProfile) {
            void* mem = nullptr;
            size_t size = 0;

            GetCMSOutputProfileData(mem, size);
            if ((mem != nullptr) && (size > 0)) {
                gCMSOutputProfile = qcms_profile_from_memory(mem, size);
                free(mem);
            }
        }

        

        if (gCMSOutputProfile && qcms_profile_is_bogus(gCMSOutputProfile)) {
            NS_ASSERTION(gCMSOutputProfile != GetCMSsRGBProfile(),
                         "Builtin sRGB profile tagged as bogus!!!");
            qcms_profile_release(gCMSOutputProfile);
            gCMSOutputProfile = nullptr;
        }

        if (!gCMSOutputProfile) {
            gCMSOutputProfile = GetCMSsRGBProfile();
        }
        

        qcms_profile_precache_output_transform(gCMSOutputProfile);
    }
}

qcms_profile *
gfxPlatform::GetCMSOutputProfile()
{
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
            return nullptr;

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
            return nullptr;

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
            return nullptr;

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
        gCMSRGBTransform = nullptr;
    }
    if (gCMSInverseRGBTransform) {
        qcms_transform_release(gCMSInverseRGBTransform);
        gCMSInverseRGBTransform = nullptr;
    }
    if (gCMSRGBATransform) {
        qcms_transform_release(gCMSRGBATransform);
        gCMSRGBATransform = nullptr;
    }
    if (gCMSOutputProfile) {
        qcms_profile_release(gCMSOutputProfile);

        
        if (gCMSsRGBProfile == gCMSOutputProfile)
            gCMSsRGBProfile = nullptr;
        gCMSOutputProfile = nullptr;
    }
    if (gCMSsRGBProfile) {
        qcms_profile_release(gCMSsRGBProfile);
        gCMSsRGBProfile = nullptr;
    }

    
    gCMSIntent = -2;
    gCMSMode = eCMSMode_Off;
    gCMSInitialized = false;
}



void
gfxPlatform::SetupClusterBoundaries(gfxTextRun *aTextRun, const char16_t *aString)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) {
        
        
        
        
        
        
        return;
    }

    aTextRun->SetupClusterBoundaries(0, aString, aTextRun->GetLength());
}

int32_t
gfxPlatform::GetBidiNumeralOption()
{
    if (mBidiNumeralOption == UNINITIALIZED_VALUE) {
        mBidiNumeralOption = Preferences::GetInt(BIDI_NUMERAL_PREF, 0);
    }
    return mBidiNumeralOption;
}

static void
FlushFontAndWordCaches()
{
    gfxFontCache *fontCache = gfxFontCache::GetCache();
    if (fontCache) {
        fontCache->AgeAllGenerations();
        fontCache->FlushShapedWordCaches();
    }
}

void
gfxPlatform::FontsPrefsChanged(const char *aPref)
{
    NS_ASSERTION(aPref != nullptr, "null preference");
    if (!strcmp(GFX_DOWNLOADABLE_FONTS_ENABLED, aPref)) {
        mAllowDownloadableFonts = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_PREF_FALLBACK_USE_CMAPS, aPref)) {
        mFallbackUsesCmaps = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_PREF_WORD_CACHE_CHARLIMIT, aPref)) {
        mWordCacheCharLimit = UNINITIALIZED_VALUE;
        FlushFontAndWordCaches();
    } else if (!strcmp(GFX_PREF_WORD_CACHE_MAXENTRIES, aPref)) {
        mWordCacheMaxEntries = UNINITIALIZED_VALUE;
        FlushFontAndWordCaches();
    } else if (!strcmp(GFX_PREF_GRAPHITE_SHAPING, aPref)) {
        mGraphiteShapingEnabled = UNINITIALIZED_VALUE;
        FlushFontAndWordCaches();
    } else if (!strcmp(BIDI_NUMERAL_PREF, aPref)) {
        mBidiNumeralOption = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_PREF_OPENTYPE_SVG, aPref)) {
        mOpenTypeSVGEnabled = UNINITIALIZED_VALUE;
        gfxFontCache::GetCache()->AgeAllGenerations();
    }
}


PRLogModuleInfo*
gfxPlatform::GetLog(eGfxLog aWhichLog)
{
    
#ifdef PR_LOGGING
    static PRLogModuleInfo *sFontlistLog = nullptr;
    static PRLogModuleInfo *sFontInitLog = nullptr;
    static PRLogModuleInfo *sTextrunLog = nullptr;
    static PRLogModuleInfo *sTextrunuiLog = nullptr;
    static PRLogModuleInfo *sCmapDataLog = nullptr;
    static PRLogModuleInfo *sTextPerfLog = nullptr;

    
    if (!sFontlistLog) {
        sFontlistLog = PR_NewLogModule("fontlist");
        sFontInitLog = PR_NewLogModule("fontinit");
        sTextrunLog = PR_NewLogModule("textrun");
        sTextrunuiLog = PR_NewLogModule("textrunui");
        sCmapDataLog = PR_NewLogModule("cmapdata");
        sTextPerfLog = PR_NewLogModule("textperf");
    }

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
    case eGfxLog_cmapdata:
        return sCmapDataLog;
        break;
    case eGfxLog_textperf:
        return sTextPerfLog;
        break;
    default:
        break;
    }

    return nullptr;
#else
    return nullptr;
#endif
}

int
gfxPlatform::GetScreenDepth() const
{
    NS_WARNING("GetScreenDepth not implemented on this platform -- returning 0!");
    return 0;
}

mozilla::gfx::SurfaceFormat
gfxPlatform::Optimal2DFormatForContent(gfxContentType aContent)
{
  switch (aContent) {
  case gfxContentType::COLOR:
    switch (GetOffscreenFormat()) {
    case gfxImageFormat::ARGB32:
      return mozilla::gfx::SurfaceFormat::B8G8R8A8;
    case gfxImageFormat::RGB24:
      return mozilla::gfx::SurfaceFormat::B8G8R8X8;
    case gfxImageFormat::RGB16_565:
      return mozilla::gfx::SurfaceFormat::R5G6B5;
    default:
      NS_NOTREACHED("unknown gfxImageFormat for gfxContentType::COLOR");
      return mozilla::gfx::SurfaceFormat::B8G8R8A8;
    }
  case gfxContentType::ALPHA:
    return mozilla::gfx::SurfaceFormat::A8;
  case gfxContentType::COLOR_ALPHA:
    return mozilla::gfx::SurfaceFormat::B8G8R8A8;
  default:
    NS_NOTREACHED("unknown gfxContentType");
    return mozilla::gfx::SurfaceFormat::B8G8R8A8;
  }
}

gfxImageFormat
gfxPlatform::OptimalFormatForContent(gfxContentType aContent)
{
  switch (aContent) {
  case gfxContentType::COLOR:
    return GetOffscreenFormat();
  case gfxContentType::ALPHA:
    return gfxImageFormat::A8;
  case gfxContentType::COLOR_ALPHA:
    return gfxImageFormat::ARGB32;
  default:
    NS_NOTREACHED("unknown gfxContentType");
    return gfxImageFormat::ARGB32;
  }
}








static bool sLayersSupportsD3D9 = false;
static bool sLayersSupportsD3D11 = false;
static bool sBufferRotationCheckPref = true;
static bool sPrefBrowserTabsRemoteAutostart = false;

static bool sLayersAccelerationPrefsInitialized = false;

void
InitLayersAccelerationPrefs()
{
  if (!sLayersAccelerationPrefsInitialized)
  {
    
    
    
    
    MOZ_ASSERT(NS_IsMainThread(), "can only initialize prefs on the main thread");

    sPrefBrowserTabsRemoteAutostart = Preferences::GetBool("browser.tabs.remote.autostart", false);

#ifdef XP_WIN
    if (gfxPrefs::LayersAccelerationForceEnabled()) {
      sLayersSupportsD3D9 = true;
      sLayersSupportsD3D11 = true;
    } else {
      nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
      if (gfxInfo) {
        int32_t status;
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT3D_9_LAYERS, &status))) {
          if (status == nsIGfxInfo::FEATURE_NO_INFO) {
            sLayersSupportsD3D9 = true;
          }
        }
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT3D_11_LAYERS, &status))) {
          if (status == nsIGfxInfo::FEATURE_NO_INFO) {
            sLayersSupportsD3D11 = true;
          }
        }
      }
    }
#endif

    sLayersAccelerationPrefsInitialized = true;
  }
}

bool
gfxPlatform::CanUseDirect3D9()
{
  
  
  MOZ_ASSERT(sLayersAccelerationPrefsInitialized);
  return sLayersSupportsD3D9;
}

bool
gfxPlatform::CanUseDirect3D11()
{
  
  
  MOZ_ASSERT(sLayersAccelerationPrefsInitialized);
  return sLayersSupportsD3D11;
}

bool
gfxPlatform::BufferRotationEnabled()
{
  MutexAutoLock autoLock(*gGfxPlatformPrefsLock);

  return sBufferRotationCheckPref && gfxPrefs::BufferRotationEnabled();
}

void
gfxPlatform::DisableBufferRotation()
{
  MutexAutoLock autoLock(*gGfxPlatformPrefsLock);

  sBufferRotationCheckPref = false;
}

TemporaryRef<ScaledFont>
gfxPlatform::GetScaledFontForFontWithCairoSkia(DrawTarget* aTarget, gfxFont* aFont)
{
    NativeFont nativeFont;
    if (aTarget->GetType() == BackendType::CAIRO || aTarget->GetType() == BackendType::SKIA) {
        nativeFont.mType = NativeFontType::CAIRO_FONT_FACE;
        nativeFont.mFont = aFont->GetCairoScaledFont();
        return Factory::CreateScaledFontForNativeFont(nativeFont, aFont->GetAdjustedSize());
    }

    return nullptr;
}

bool
gfxPlatform::ComputeUsesOffMainThreadCompositing()
{
  InitLayersAccelerationPrefs();
  bool result =
    sPrefBrowserTabsRemoteAutostart ||
    gfxPrefs::LayersOffMainThreadCompositionEnabled() ||
    gfxPrefs::LayersOffMainThreadCompositionForceEnabled() ||
    gfxPrefs::LayersOffMainThreadCompositionTestingEnabled();
#if defined(MOZ_WIDGET_GTK) && defined(NIGHTLY_BUILD)
  
  result |=
    gfxPrefs::LayersAccelerationForceEnabled() ||
    PR_GetEnv("MOZ_USE_OMTC") ||
    PR_GetEnv("MOZ_OMTC_ENABLED"); 
                                   
                                   
                                   
#endif
  return result;
}