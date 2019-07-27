





#include "gfxWindowsPlatform.h"

#include "cairo.h"
#include "mozilla/ArrayUtils.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"

#include "nsUnicharUtils.h"

#include "mozilla/Preferences.h"
#include "mozilla/WindowsVersion.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "mozilla/Telemetry.h"

#include "nsIWindowsRegKey.h"
#include "nsIFile.h"
#include "plbase64.h"
#include "nsIXULRuntime.h"
#include "imgLoader.h"

#include "nsIGfxInfo.h"
#include "GfxDriverInfo.h"

#include "gfxCrashReporterUtils.h"

#include "gfxGDIFontList.h"
#include "gfxGDIFont.h"

#include "mozilla/layers/CompositorParent.h"   
#include "DeviceManagerD3D9.h"
#include "mozilla/layers/ReadbackManagerD3D11.h"

#include "WinUtils.h"

#ifdef CAIRO_HAS_DWRITE_FONT
#include "gfxDWriteFontList.h"
#include "gfxDWriteFonts.h"
#include "gfxDWriteCommon.h"
#include <dwrite.h>
#endif

#include "gfxTextRun.h"
#include "gfxUserFontSet.h"
#include "nsWindowsHelpers.h"
#include "gfx2DGlue.h"

#include <string>

#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"

#include <d3d10_1.h>

#include "mozilla/gfx/2D.h"

#include "nsMemory.h"
#endif

#include <d3d11.h>

#include "nsIMemoryReporter.h"
#include <winternl.h>
#include "d3dkmtQueryStatistics.h"

#include "SurfaceCache.h"
#include "gfxPrefs.h"

#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#endif

#include "VsyncSource.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using namespace mozilla::widget;
using namespace mozilla::image;

DCFromDrawTarget::DCFromDrawTarget(DrawTarget& aDrawTarget)
{
  mDC = nullptr;
  if (aDrawTarget.GetBackendType() == BackendType::CAIRO) {
    cairo_surface_t *surf = (cairo_surface_t*)
        aDrawTarget.GetNativeSurface(NativeSurfaceType::CAIRO_SURFACE);
    if (surf) {
      cairo_surface_type_t surfaceType = cairo_surface_get_type(surf);
      if (surfaceType == CAIRO_SURFACE_TYPE_WIN32 ||
          surfaceType == CAIRO_SURFACE_TYPE_WIN32_PRINTING) {
        mDC = cairo_win32_surface_get_dc(surf);
        mNeedsRelease = false;
        SaveDC(mDC);
        cairo_t* ctx = (cairo_t*)
            aDrawTarget.GetNativeSurface(NativeSurfaceType::CAIRO_CONTEXT);
        cairo_scaled_font_t* scaled = cairo_get_scaled_font(ctx);
        cairo_win32_scaled_font_select_font(scaled, mDC);
      }
    }
    if (!mDC) {
      mDC = GetDC(nullptr);
      SetGraphicsMode(mDC, GM_ADVANCED);
      mNeedsRelease = true;
    }
  }
}

#ifdef CAIRO_HAS_D2D_SURFACE

static const char *kFeatureLevelPref =
  "gfx.direct3d.last_used_feature_level_idx";
static const int kSupportedFeatureLevels[] =
  { D3D10_FEATURE_LEVEL_10_1, D3D10_FEATURE_LEVEL_10_0,
    D3D10_FEATURE_LEVEL_9_3 };

class GfxD2DSurfaceReporter final : public nsIMemoryReporter
{
    ~GfxD2DSurfaceReporter() {}

public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData, bool aAnonymize)
    {
        nsresult rv;

        int64_t amount = cairo_d2d_get_image_surface_cache_usage();
        rv = MOZ_COLLECT_REPORT(
            "gfx-d2d-surface-cache", KIND_OTHER, UNITS_BYTES, amount,
            "Memory used by the Direct2D internal surface cache.");
        NS_ENSURE_SUCCESS(rv, rv);

        cairo_device_t *device =
            gfxWindowsPlatform::GetPlatform()->GetD2DDevice();
        amount = device ? cairo_d2d_get_surface_vram_usage(device) : 0;
        rv = MOZ_COLLECT_REPORT(
            "gfx-d2d-surface-vram", KIND_OTHER, UNITS_BYTES, amount,
            "Video memory used by D2D surfaces.");
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(GfxD2DSurfaceReporter, nsIMemoryReporter)

#endif

class GfxD2DVramReporter final : public nsIMemoryReporter
{
    ~GfxD2DVramReporter() {}

public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData, bool aAnonymize)
    {
        nsresult rv;

        rv = MOZ_COLLECT_REPORT(
            "gfx-d2d-vram-draw-target", KIND_OTHER, UNITS_BYTES,
            Factory::GetD2DVRAMUsageDrawTarget(),
            "Video memory used by D2D DrawTargets.");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = MOZ_COLLECT_REPORT(
            "gfx-d2d-vram-source-surface", KIND_OTHER, UNITS_BYTES,
            Factory::GetD2DVRAMUsageSourceSurface(),
            "Video memory used by D2D SourceSurfaces.");
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(GfxD2DVramReporter, nsIMemoryReporter)

#define GFX_USE_CLEARTYPE_ALWAYS "gfx.font_rendering.cleartype.always_use_for_content"
#define GFX_DOWNLOADABLE_FONTS_USE_CLEARTYPE "gfx.font_rendering.cleartype.use_for_downloadable_fonts"

#define GFX_CLEARTYPE_PARAMS           "gfx.font_rendering.cleartype_params."
#define GFX_CLEARTYPE_PARAMS_GAMMA     "gfx.font_rendering.cleartype_params.gamma"
#define GFX_CLEARTYPE_PARAMS_CONTRAST  "gfx.font_rendering.cleartype_params.enhanced_contrast"
#define GFX_CLEARTYPE_PARAMS_LEVEL     "gfx.font_rendering.cleartype_params.cleartype_level"
#define GFX_CLEARTYPE_PARAMS_STRUCTURE "gfx.font_rendering.cleartype_params.pixel_structure"
#define GFX_CLEARTYPE_PARAMS_MODE      "gfx.font_rendering.cleartype_params.rendering_mode"

class GPUAdapterReporter final : public nsIMemoryReporter
{
    
    static bool GetDXGIAdapter(IDXGIAdapter **DXGIAdapter)
    {
        ID3D10Device1 *D2D10Device;
        IDXGIDevice *DXGIDevice;
        bool result = false;

        if ((D2D10Device = mozilla::gfx::Factory::GetDirect3D10Device())) {
            if (D2D10Device->QueryInterface(__uuidof(IDXGIDevice), (void **)&DXGIDevice) == S_OK) {
                result = (DXGIDevice->GetAdapter(DXGIAdapter) == S_OK);
                DXGIDevice->Release();
            }
        }

        return result;
    }

    ~GPUAdapterReporter() {}

public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD
    CollectReports(nsIMemoryReporterCallback* aCb,
                   nsISupports* aClosure, bool aAnonymize)
    {
        HANDLE ProcessHandle = GetCurrentProcess();

        int64_t dedicatedBytesUsed = 0;
        int64_t sharedBytesUsed = 0;
        int64_t committedBytesUsed = 0;
        IDXGIAdapter *DXGIAdapter;

        HMODULE gdi32Handle;
        PFND3DKMTQS queryD3DKMTStatistics;

        
        if (!IsWin7OrLater())
            return NS_OK;

        if ((gdi32Handle = LoadLibrary(TEXT("gdi32.dll"))))
            queryD3DKMTStatistics = (PFND3DKMTQS)GetProcAddress(gdi32Handle, "D3DKMTQueryStatistics");

        if (queryD3DKMTStatistics && GetDXGIAdapter(&DXGIAdapter)) {
            

            DXGI_ADAPTER_DESC adapterDesc;
            D3DKMTQS queryStatistics;

            DXGIAdapter->GetDesc(&adapterDesc);
            DXGIAdapter->Release();

            memset(&queryStatistics, 0, sizeof(D3DKMTQS));
            queryStatistics.Type = D3DKMTQS_PROCESS;
            queryStatistics.AdapterLuid = adapterDesc.AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            if (NT_SUCCESS(queryD3DKMTStatistics(&queryStatistics))) {
                committedBytesUsed = queryStatistics.QueryResult.ProcessInfo.SystemMemory.BytesAllocated;
            }

            memset(&queryStatistics, 0, sizeof(D3DKMTQS));
            queryStatistics.Type = D3DKMTQS_ADAPTER;
            queryStatistics.AdapterLuid = adapterDesc.AdapterLuid;
            if (NT_SUCCESS(queryD3DKMTStatistics(&queryStatistics))) {
                ULONG i;
                ULONG segmentCount = queryStatistics.QueryResult.AdapterInfo.NbSegments;

                for (i = 0; i < segmentCount; i++) {
                    memset(&queryStatistics, 0, sizeof(D3DKMTQS));
                    queryStatistics.Type = D3DKMTQS_SEGMENT;
                    queryStatistics.AdapterLuid = adapterDesc.AdapterLuid;
                    queryStatistics.QuerySegment.SegmentId = i;

                    if (NT_SUCCESS(queryD3DKMTStatistics(&queryStatistics))) {
                        bool aperture;

                        
                        if (!IsWin8OrLater())
                            aperture = queryStatistics.QueryResult.SegmentInfoWin7.Aperture;
                        else
                            aperture = queryStatistics.QueryResult.SegmentInfoWin8.Aperture;

                        memset(&queryStatistics, 0, sizeof(D3DKMTQS));
                        queryStatistics.Type = D3DKMTQS_PROCESS_SEGMENT;
                        queryStatistics.AdapterLuid = adapterDesc.AdapterLuid;
                        queryStatistics.hProcess = ProcessHandle;
                        queryStatistics.QueryProcessSegment.SegmentId = i;
                        if (NT_SUCCESS(queryD3DKMTStatistics(&queryStatistics))) {
                            ULONGLONG bytesCommitted;
                            if (!IsWin8OrLater())
                                bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInfo.Win7.BytesCommitted;
                            else
                                bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInfo.Win8.BytesCommitted;
                            if (aperture)
                                sharedBytesUsed += bytesCommitted;
                            else
                                dedicatedBytesUsed += bytesCommitted;
                        }
                    }
                }
            }
        }

        FreeLibrary(gdi32Handle);

#define REPORT(_path, _amount, _desc)                                         \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),           \
                         KIND_OTHER, UNITS_BYTES, _amount,                    \
                         NS_LITERAL_CSTRING(_desc), aClosure);                \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

        REPORT("gpu-committed", committedBytesUsed,
               "Memory committed by the Windows graphics system.");

        REPORT("gpu-dedicated", dedicatedBytesUsed,
               "Out-of-process memory allocated for this process in a "
               "physical GPU adapter's memory.");

        REPORT("gpu-shared", sharedBytesUsed,
               "In-process memory that is shared with the GPU.");

#undef REPORT

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(GPUAdapterReporter, nsIMemoryReporter)


Atomic<size_t> gfxWindowsPlatform::sD3D11MemoryUsed;

class D3D11TextureReporter final : public nsIMemoryReporter
{
  ~D3D11TextureReporter() {}

public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CollectReports(nsIHandleReportCallback *aHandleReport,
                            nsISupports* aData, bool aAnonymize) override
  {
      return MOZ_COLLECT_REPORT("d3d11-shared-textures", KIND_OTHER, UNITS_BYTES,
                                gfxWindowsPlatform::sD3D11MemoryUsed,
                                "Memory used for D3D11 shared textures");
  }
};

NS_IMPL_ISUPPORTS(D3D11TextureReporter, nsIMemoryReporter)

Atomic<size_t> gfxWindowsPlatform::sD3D9MemoryUsed;

class D3D9TextureReporter final : public nsIMemoryReporter
{
  ~D3D9TextureReporter() {}

public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CollectReports(nsIHandleReportCallback *aHandleReport,
                            nsISupports* aData, bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT("d3d9-shared-textures", KIND_OTHER, UNITS_BYTES,
                              gfxWindowsPlatform::sD3D9MemoryUsed,
                              "Memory used for D3D9 shared textures");
  }
};

NS_IMPL_ISUPPORTS(D3D9TextureReporter, nsIMemoryReporter)

Atomic<size_t> gfxWindowsPlatform::sD3D9SurfaceImageUsed;

class D3D9SurfaceImageReporter final : public nsIMemoryReporter
{
  ~D3D9SurfaceImageReporter() {}

public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CollectReports(nsIHandleReportCallback *aHandleReport,
                            nsISupports* aData, bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT("d3d9-surface-image", KIND_OTHER, UNITS_BYTES,
                              gfxWindowsPlatform::sD3D9SurfaceImageUsed,
                              "Memory used for D3D9 surface images");
  }
};

NS_IMPL_ISUPPORTS(D3D9SurfaceImageReporter, nsIMemoryReporter)

Atomic<size_t> gfxWindowsPlatform::sD3D9SharedTextureUsed;

class D3D9SharedTextureReporter final : public nsIMemoryReporter
{
  ~D3D9SharedTextureReporter() {}

public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CollectReports(nsIHandleReportCallback *aHandleReport,
                            nsISupports* aData, bool aAnonymize) override
  {
    return MOZ_COLLECT_REPORT("d3d9-shared-texture", KIND_OTHER, UNITS_BYTES,
                              gfxWindowsPlatform::sD3D9SharedTextureUsed,
                              "Memory used for D3D9 shared textures");
  }
};

NS_IMPL_ISUPPORTS(D3D9SharedTextureReporter, nsIMemoryReporter)

gfxWindowsPlatform::gfxWindowsPlatform()
  : mD3D11DeviceInitialized(false)
  , mIsWARP(false)
  , mCanInitMediaDevice(false)
{
    mUseClearTypeForDownloadableFonts = UNINITIALIZED_VALUE;
    mUseClearTypeAlways = UNINITIALIZED_VALUE;

    mUsingGDIFonts = false;

    

 
    CoInitialize(nullptr); 

#ifdef CAIRO_HAS_D2D_SURFACE
    RegisterStrongMemoryReporter(new GfxD2DSurfaceReporter());
    mD2DDevice = nullptr;
#endif
    RegisterStrongMemoryReporter(new GfxD2DVramReporter());

    if (gfxPrefs::Direct2DUse1_1()) {
      InitD3D11Devices();
    }

    UpdateRenderMode();

    RegisterStrongMemoryReporter(new GPUAdapterReporter());
    RegisterStrongMemoryReporter(new D3D11TextureReporter());
    RegisterStrongMemoryReporter(new D3D9TextureReporter());
    RegisterStrongMemoryReporter(new D3D9SurfaceImageReporter());
    RegisterStrongMemoryReporter(new D3D9SharedTextureReporter());
}

gfxWindowsPlatform::~gfxWindowsPlatform()
{
    mDeviceManager = nullptr;

    
    
#ifdef CAIRO_HAS_D2D_SURFACE
    if (mD2DDevice) {
        cairo_release_device(mD2DDevice);
    }
#endif

    mozilla::gfx::Factory::D2DCleanup();

    

 
    CoUninitialize();
}

double
gfxWindowsPlatform::GetDPIScale()
{
  return WinUtils::LogToPhysFactor();
}

void
gfxWindowsPlatform::UpdateRenderMode()
{



    bool didReset = false;
    DeviceResetReason resetReason = DeviceResetReason::OK;
    if (DidRenderingDeviceReset(&resetReason)) {
      Telemetry::Accumulate(Telemetry::DEVICE_RESET_REASON, uint32_t(resetReason));
      mD3D11DeviceInitialized = false;
      mD3D11Device = nullptr;
      mD3D11ContentDevice = nullptr;
      mAdapter = nullptr;

      imgLoader::Singleton()->ClearCache(true);
      imgLoader::Singleton()->ClearCache(false);
      Factory::SetDirect3D11Device(nullptr);

      didReset = true;
    }

    mRenderMode = RENDER_GDI;

    bool isVistaOrHigher = IsVistaOrLater();

    bool safeMode = false;
    nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
    if (xr)
      xr->GetInSafeMode(&safeMode);

    mUseDirectWrite = Preferences::GetBool("gfx.font_rendering.directwrite.enabled", false);

#ifdef CAIRO_HAS_D2D_SURFACE
    bool d2dDisabled = false;
    bool d2dForceEnabled = false;
    bool d2dBlocked = false;

    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (gfxInfo) {
        int32_t status;
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT2D, &status))) {
            if (status != nsIGfxInfo::FEATURE_STATUS_OK) {
                d2dBlocked = true;
            }
        }
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT3D_11_LAYERS, &status))) {
            if (status != nsIGfxInfo::FEATURE_STATUS_OK) {
                d2dBlocked = true;
            }
        }
    }

    
    
    d2dDisabled = gfxPrefs::Direct2DDisabled();
    d2dForceEnabled = gfxPrefs::Direct2DForceEnabled();

    bool tryD2D = d2dForceEnabled || (!d2dBlocked && !gfxPrefs::LayersPreferD3D9());

    
    
    if (d2dDisabled || mUsingGDIFonts) {
        tryD2D = false;
    }

    ID3D11Device *device = GetD3D11Device();
    if (isVistaOrHigher && !safeMode && tryD2D &&
        device &&
        device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_10_0 &&
        DoesD3D11TextureSharingWork(device)) {

        VerifyD2DDevice(d2dForceEnabled);
        if (mD2DDevice && GetD3D11Device()) {
            mRenderMode = RENDER_DIRECT2D;
            mUseDirectWrite = true;
        }
    } else {
        mD2DDevice = nullptr;
    }
#endif

#ifdef CAIRO_HAS_DWRITE_FONT
    
    
    if (!mDWriteFactory && (mUseDirectWrite && isVistaOrHigher)) {
        mozilla::ScopedGfxFeatureReporter reporter("DWrite");
        decltype(DWriteCreateFactory)* createDWriteFactory = (decltype(DWriteCreateFactory)*)
            GetProcAddress(LoadLibraryW(L"dwrite.dll"), "DWriteCreateFactory");

        if (createDWriteFactory) {
            




            IDWriteFactory *factory;
            HRESULT hr = createDWriteFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&factory));

            if (SUCCEEDED(hr) && factory) {
                mDWriteFactory = factory;
                factory->Release();
                hr = mDWriteFactory->CreateTextAnalyzer(
                    getter_AddRefs(mDWriteAnalyzer));
            }

            SetupClearTypeParams();

            if (hr == S_OK)
              reporter.SetSuccessful();
        }
    }
#endif

    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO);
    uint32_t contentMask = BackendTypeBit(BackendType::CAIRO);
    BackendType defaultBackend = BackendType::CAIRO;
    if (mRenderMode == RENDER_DIRECT2D) {
      canvasMask |= BackendTypeBit(BackendType::DIRECT2D);
      contentMask |= BackendTypeBit(BackendType::DIRECT2D);
      if (gfxPrefs::Direct2DUse1_1() && Factory::SupportsD2D1() &&
          GetD3D11ContentDevice()) {
        contentMask |= BackendTypeBit(BackendType::DIRECT2D1_1);
        canvasMask |= BackendTypeBit(BackendType::DIRECT2D1_1);
        defaultBackend = BackendType::DIRECT2D1_1;
      } else {
        defaultBackend = BackendType::DIRECT2D;
      }
    } else {
      canvasMask |= BackendTypeBit(BackendType::SKIA);
    }
    contentMask |= BackendTypeBit(BackendType::SKIA);
    InitBackendPrefs(canvasMask, defaultBackend,
                     contentMask, defaultBackend);

    if (didReset) {
      mScreenReferenceDrawTarget = CreateOffscreenContentDrawTarget(IntSize(1, 1), SurfaceFormat::B8G8R8A8);
    }
}

#ifdef CAIRO_HAS_D2D_SURFACE
HRESULT
gfxWindowsPlatform::CreateDevice(nsRefPtr<IDXGIAdapter1> &adapter1,
                                 int featureLevelIndex)
{
  nsModuleHandle d3d10module(LoadLibrarySystem32(L"d3d10_1.dll"));
  if (!d3d10module)
    return E_FAIL;
  decltype(D3D10CreateDevice1)* createD3DDevice =
    (decltype(D3D10CreateDevice1)*) GetProcAddress(d3d10module, "D3D10CreateDevice1");
  if (!createD3DDevice)
    return E_FAIL;

  nsRefPtr<ID3D10Device1> device;
  HRESULT hr =
    createD3DDevice(adapter1, D3D10_DRIVER_TYPE_HARDWARE, nullptr,
#ifdef DEBUG
                    
                    
#endif
                    D3D10_CREATE_DEVICE_BGRA_SUPPORT |
                    D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                    static_cast<D3D10_FEATURE_LEVEL1>(kSupportedFeatureLevels[featureLevelIndex]),
                    D3D10_1_SDK_VERSION, getter_AddRefs(device));

  
  
  
  
  if (device) {
    mD2DDevice = cairo_d2d_create_device_from_d3d10device(device);

    
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      Preferences::SetInt(kFeatureLevelPref, featureLevelIndex);
    }
  }

  return device ? S_OK : hr;
}
#endif

void
gfxWindowsPlatform::VerifyD2DDevice(bool aAttemptForce)
{
#ifdef CAIRO_HAS_D2D_SURFACE
    if (mD2DDevice) {
        ID3D10Device1 *device = cairo_d2d_device_get_device(mD2DDevice);

        if (SUCCEEDED(device->GetDeviceRemovedReason())) {
            return;
        }
        mD2DDevice = nullptr;

        
        
        SurfaceCache::DiscardAll();
    }

    mozilla::ScopedGfxFeatureReporter reporter("D2D", aAttemptForce);

    nsRefPtr<ID3D10Device1> device;

    int supportedFeatureLevelsCount = ArrayLength(kSupportedFeatureLevels);
    
    if (!IsRunningInWindowsMetro()) {
      supportedFeatureLevelsCount--;
    }

    nsRefPtr<IDXGIAdapter1> adapter1 = GetDXGIAdapter();

    if (!adapter1) {
      
      return;
    }

    
    
    
    int featureLevelIndex = Preferences::GetInt(kFeatureLevelPref, 0);
    if (featureLevelIndex >= supportedFeatureLevelsCount || featureLevelIndex < 0)
      featureLevelIndex = 0;

    
    
    HRESULT hr = E_FAIL;
    for (int i = featureLevelIndex; i < supportedFeatureLevelsCount; i++) {
      hr = CreateDevice(adapter1, i);
      
      if (SUCCEEDED(hr))
        break;
    }

    
    
    if (SUCCEEDED(hr)) {
      for (int i = featureLevelIndex - 1; i >= 0; i--) {
        hr = CreateDevice(adapter1, i);
        
        if (FAILED(hr)) {
          break;
        }
      }
    }

    if (!mD2DDevice && aAttemptForce) {
        mD2DDevice = cairo_d2d_create_device();
    }

    if (mD2DDevice) {
        reporter.SetSuccessful();
        mozilla::gfx::Factory::SetDirect3D10Device(cairo_d2d_device_get_device(mD2DDevice));
    }

    ScopedGfxFeatureReporter reporter1_1("D2D1.1");

    if (Factory::SupportsD2D1()) {
      reporter1_1.SetSuccessful();
    }
#endif
}

gfxPlatformFontList*
gfxWindowsPlatform::CreatePlatformFontList()
{
    mUsingGDIFonts = false;
    gfxPlatformFontList *pfl;
#ifdef CAIRO_HAS_DWRITE_FONT
    
    
    if (IsNotWin7PreRTM() && GetDWriteFactory()) {
        pfl = new gfxDWriteFontList();
        if (NS_SUCCEEDED(pfl->InitFontList())) {
            return pfl;
        }
        
        
        
        gfxPlatformFontList::Shutdown();
        SetRenderMode(RENDER_GDI);
    }
#endif
    pfl = new gfxGDIFontList();
    mUsingGDIFonts = true;

    if (NS_SUCCEEDED(pfl->InitFontList())) {
        return pfl;
    }

    gfxPlatformFontList::Shutdown();
    return nullptr;
}

already_AddRefed<gfxASurface>
gfxWindowsPlatform::CreateOffscreenSurface(const IntSize& size,
                                           gfxContentType contentType)
{
    nsRefPtr<gfxASurface> surf = nullptr;

#ifdef CAIRO_HAS_WIN32_SURFACE
    if (mRenderMode == RENDER_GDI)
        surf = new gfxWindowsSurface(size,
                                     OptimalFormatForContent(contentType));
#endif

#ifdef CAIRO_HAS_D2D_SURFACE
    if (mRenderMode == RENDER_DIRECT2D)
        surf = new gfxD2DSurface(size,
                                 OptimalFormatForContent(contentType));
#endif

    if (!surf || surf->CairoStatus()) {
        surf = new gfxImageSurface(size,
                                   OptimalFormatForContent(contentType));
    }

    return surf.forget();
}

TemporaryRef<ScaledFont>
gfxWindowsPlatform::GetScaledFontForFont(DrawTarget* aTarget, gfxFont *aFont)
{
    if (aFont->GetType() == gfxFont::FONT_TYPE_DWRITE) {
        gfxDWriteFont *font = static_cast<gfxDWriteFont*>(aFont);

        NativeFont nativeFont;
        nativeFont.mType = NativeFontType::DWRITE_FONT_FACE;
        nativeFont.mFont = font->GetFontFace();

        if (aTarget->GetBackendType() == BackendType::CAIRO) {
          return Factory::CreateScaledFontWithCairo(nativeFont,
                                                    font->GetAdjustedSize(),
                                                    font->GetCairoScaledFont());
        }

        return Factory::CreateScaledFontForNativeFont(nativeFont,
                                                      font->GetAdjustedSize());
    }

    NS_ASSERTION(aFont->GetType() == gfxFont::FONT_TYPE_GDI,
        "Fonts on windows should be GDI or DWrite!");

    NativeFont nativeFont;
    nativeFont.mType = NativeFontType::GDI_FONT_FACE;
    LOGFONT lf;
    GetObject(static_cast<gfxGDIFont*>(aFont)->GetHFONT(), sizeof(LOGFONT), &lf);
    nativeFont.mFont = &lf;

    if (aTarget->GetBackendType() == BackendType::CAIRO) {
      return Factory::CreateScaledFontWithCairo(nativeFont,
                                                aFont->GetAdjustedSize(),
                                                aFont->GetCairoScaledFont());
    }

    return Factory::CreateScaledFontForNativeFont(nativeFont, aFont->GetAdjustedSize());
}

nsresult
gfxWindowsPlatform::GetFontList(nsIAtom *aLangGroup,
                                const nsACString& aGenericFamily,
                                nsTArray<nsString>& aListOfFonts)
{
    gfxPlatformFontList::PlatformFontList()->GetFontList(aLangGroup, aGenericFamily, aListOfFonts);

    return NS_OK;
}

nsresult
gfxWindowsPlatform::UpdateFontList()
{
    gfxPlatformFontList::PlatformFontList()->UpdateFontList();

    return NS_OK;
}

static const char kFontAparajita[] = "Aparajita";
static const char kFontArabicTypesetting[] = "Arabic Typesetting";
static const char kFontArial[] = "Arial";
static const char kFontArialUnicodeMS[] = "Arial Unicode MS";
static const char kFontCambria[] = "Cambria";
static const char kFontCambriaMath[] = "Cambria Math";
static const char kFontEbrima[] = "Ebrima";
static const char kFontEstrangeloEdessa[] = "Estrangelo Edessa";
static const char kFontEuphemia[] = "Euphemia";
static const char kFontGabriola[] = "Gabriola";
static const char kFontJavaneseText[] = "Javanese Text";
static const char kFontKhmerUI[] = "Khmer UI";
static const char kFontLaoUI[] = "Lao UI";
static const char kFontLeelawadeeUI[] = "Leelawadee UI";
static const char kFontLucidaSansUnicode[] = "Lucida Sans Unicode";
static const char kFontMVBoli[] = "MV Boli";
static const char kFontMalgunGothic[] = "Malgun Gothic";
static const char kFontMicrosoftJhengHei[] = "Microsoft JhengHei";
static const char kFontMicrosoftNewTaiLue[] = "Microsoft New Tai Lue";
static const char kFontMicrosoftPhagsPa[] = "Microsoft PhagsPa";
static const char kFontMicrosoftTaiLe[] = "Microsoft Tai Le";
static const char kFontMicrosoftUighur[] = "Microsoft Uighur";
static const char kFontMicrosoftYaHei[] = "Microsoft YaHei";
static const char kFontMicrosoftYiBaiti[] = "Microsoft Yi Baiti";
static const char kFontMeiryo[] = "Meiryo";
static const char kFontMongolianBaiti[] = "Mongolian Baiti";
static const char kFontMyanmarText[] = "Myanmar Text";
static const char kFontNirmalaUI[] = "Nirmala UI";
static const char kFontNyala[] = "Nyala";
static const char kFontPlantagenetCherokee[] = "Plantagenet Cherokee";
static const char kFontSegoeUI[] = "Segoe UI";
static const char kFontSegoeUIEmoji[] = "Segoe UI Emoji";
static const char kFontSegoeUISymbol[] = "Segoe UI Symbol";
static const char kFontSylfaen[] = "Sylfaen";
static const char kFontTraditionalArabic[] = "Traditional Arabic";
static const char kFontUtsaah[] = "Utsaah";
static const char kFontYuGothic[] = "Yu Gothic";

void
gfxWindowsPlatform::GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                           int32_t aRunScript,
                                           nsTArray<const char*>& aFontList)
{
    if (aNextCh == 0xfe0fu) {
        aFontList.AppendElement(kFontSegoeUIEmoji);
    }

    
    aFontList.AppendElement(kFontArial);

    if (!IS_IN_BMP(aCh)) {
        uint32_t p = aCh >> 16;
        if (p == 1) { 
            if (aNextCh == 0xfe0eu) {
                aFontList.AppendElement(kFontSegoeUISymbol);
                aFontList.AppendElement(kFontSegoeUIEmoji);
            } else {
                if (aNextCh != 0xfe0fu) {
                    aFontList.AppendElement(kFontSegoeUIEmoji);
                }
                aFontList.AppendElement(kFontSegoeUISymbol);
            }
            aFontList.AppendElement(kFontEbrima);
            aFontList.AppendElement(kFontNirmalaUI);
            aFontList.AppendElement(kFontCambriaMath);
        }
    } else {
        uint32_t b = (aCh >> 8) & 0xff;

        switch (b) {
        case 0x05:
            aFontList.AppendElement(kFontEstrangeloEdessa);
            aFontList.AppendElement(kFontCambria);
            break;
        case 0x06:
            aFontList.AppendElement(kFontMicrosoftUighur);
            break;
        case 0x07:
            aFontList.AppendElement(kFontEstrangeloEdessa);
            aFontList.AppendElement(kFontMVBoli);
            aFontList.AppendElement(kFontEbrima);
            break;
        case 0x09:
            aFontList.AppendElement(kFontNirmalaUI);
            aFontList.AppendElement(kFontUtsaah);
            aFontList.AppendElement(kFontAparajita);
            break;
        case 0x0e:
            aFontList.AppendElement(kFontLaoUI);
            break;
        case 0x10:
            aFontList.AppendElement(kFontMyanmarText);
            break;
        case 0x11:
            aFontList.AppendElement(kFontMalgunGothic);
            break;
        case 0x12:
        case 0x13:
            aFontList.AppendElement(kFontNyala);
            aFontList.AppendElement(kFontPlantagenetCherokee);
            break;
        case 0x14:
        case 0x15:
        case 0x16:
            aFontList.AppendElement(kFontEuphemia);
            aFontList.AppendElement(kFontSegoeUISymbol);
            break;
        case 0x17:
            aFontList.AppendElement(kFontKhmerUI);
            break;
        case 0x18:  
            aFontList.AppendElement(kFontMongolianBaiti);
            aFontList.AppendElement(kFontEuphemia);
            break;
        case 0x19:
            aFontList.AppendElement(kFontMicrosoftTaiLe);
            aFontList.AppendElement(kFontMicrosoftNewTaiLue);
            aFontList.AppendElement(kFontKhmerUI);
            break;
            break;
        case 0x1a:
            aFontList.AppendElement(kFontLeelawadeeUI);
            break;
        case 0x1c:
            aFontList.AppendElement(kFontNirmalaUI);
            break;
        case 0x20:  
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
            aFontList.AppendElement(kFontSegoeUI);
            aFontList.AppendElement(kFontSegoeUISymbol);
            aFontList.AppendElement(kFontCambria);
            aFontList.AppendElement(kFontMeiryo);
            aFontList.AppendElement(kFontArial);
            aFontList.AppendElement(kFontLucidaSansUnicode);
            aFontList.AppendElement(kFontEbrima);
            break;
        case 0x2d:
        case 0x2e:
        case 0x2f:
            aFontList.AppendElement(kFontEbrima);
            aFontList.AppendElement(kFontNyala);
            aFontList.AppendElement(kFontSegoeUI);
            aFontList.AppendElement(kFontSegoeUISymbol);
            aFontList.AppendElement(kFontMeiryo);
            break;
        case 0x28:  
            aFontList.AppendElement(kFontSegoeUISymbol);
            break;
        case 0x30:
        case 0x31:
            aFontList.AppendElement(kFontMicrosoftYaHei);
            break;
        case 0x32:
            aFontList.AppendElement(kFontMalgunGothic);
            break;
        case 0x4d:
            aFontList.AppendElement(kFontSegoeUISymbol);
            break;
        case 0x9f:
            aFontList.AppendElement(kFontMicrosoftYaHei);
            aFontList.AppendElement(kFontYuGothic);
            break;
        case 0xa0:  
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
            aFontList.AppendElement(kFontMicrosoftYiBaiti);
            aFontList.AppendElement(kFontSegoeUI);
            break;
        case 0xa5:
        case 0xa6:
        case 0xa7:
            aFontList.AppendElement(kFontEbrima);
            aFontList.AppendElement(kFontSegoeUI);
            aFontList.AppendElement(kFontCambriaMath);
            break;
        case 0xa8:
             aFontList.AppendElement(kFontMicrosoftPhagsPa);
             aFontList.AppendElement(kFontNirmalaUI);
             break;
        case 0xa9:
             aFontList.AppendElement(kFontMalgunGothic);
             aFontList.AppendElement(kFontJavaneseText);
             break;
        case 0xaa:
             aFontList.AppendElement(kFontMyanmarText);
             break;
        case 0xab:
             aFontList.AppendElement(kFontEbrima);
             aFontList.AppendElement(kFontNyala);
             break;
        case 0xd7:
             aFontList.AppendElement(kFontMalgunGothic);
             break;
        case 0xfb:
            aFontList.AppendElement(kFontMicrosoftUighur);
            aFontList.AppendElement(kFontGabriola);
            aFontList.AppendElement(kFontSylfaen);
            break;
        case 0xfc:
        case 0xfd:
            aFontList.AppendElement(kFontTraditionalArabic);
            aFontList.AppendElement(kFontArabicTypesetting);
            break;
        case 0xfe:
            aFontList.AppendElement(kFontTraditionalArabic);
            aFontList.AppendElement(kFontMicrosoftJhengHei);
           break;
       case 0xff:
            aFontList.AppendElement(kFontMicrosoftJhengHei);
            break;
        default:
            break;
        }
    }

    
    
    aFontList.AppendElement(kFontArialUnicodeMS);
}

nsresult
gfxWindowsPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(aFontName, aFamilyName);
    return NS_OK;
}

gfxFontGroup *
gfxWindowsPlatform::CreateFontGroup(const FontFamilyList& aFontFamilyList,
                                    const gfxFontStyle *aStyle,
                                    gfxUserFontSet *aUserFontSet)
{
    return new gfxFontGroup(aFontFamilyList, aStyle, aUserFontSet);
}

gfxFontEntry* 
gfxWindowsPlatform::LookupLocalFont(const nsAString& aFontName,
                                    uint16_t aWeight,
                                    int16_t aStretch,
                                    bool aItalic)
{
    return gfxPlatformFontList::PlatformFontList()->LookupLocalFont(aFontName,
                                                                    aWeight,
                                                                    aStretch,
                                                                    aItalic);
}

gfxFontEntry* 
gfxWindowsPlatform::MakePlatformFont(const nsAString& aFontName,
                                     uint16_t aWeight,
                                     int16_t aStretch,
                                     bool aItalic,
                                     const uint8_t* aFontData,
                                     uint32_t aLength)
{
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aFontName,
                                                                     aWeight,
                                                                     aStretch,
                                                                     aItalic,
                                                                     aFontData,
                                                                     aLength);
}

bool
gfxWindowsPlatform::IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & gfxUserFontSet::FLAG_FORMATS_COMMON) {
        return true;
    }

    
    if (aFormatFlags != 0) {
        return false;
    }

    
    return true;
}

static DeviceResetReason HResultToResetReason(HRESULT hr)
{
  switch (hr) {
  case DXGI_ERROR_DEVICE_HUNG:
    return DeviceResetReason::HUNG;
  case DXGI_ERROR_DEVICE_REMOVED:
    return DeviceResetReason::REMOVED;
  case DXGI_ERROR_DEVICE_RESET:
    return DeviceResetReason::RESET;
  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return DeviceResetReason::DRIVER_ERROR;
  case DXGI_ERROR_INVALID_CALL:
    return DeviceResetReason::INVALID_CALL;
  case E_OUTOFMEMORY:
    return DeviceResetReason::OUT_OF_MEMORY;
  default:
    MOZ_ASSERT(false);
  }
  return DeviceResetReason::UNKNOWN;
}

bool
gfxWindowsPlatform::DidRenderingDeviceReset(DeviceResetReason* aResetReason)
{
  if (aResetReason) {
    *aResetReason = DeviceResetReason::OK;
  }

  if (mD3D11Device) {
    HRESULT hr = mD3D11Device->GetDeviceRemovedReason();
    if (hr != S_OK) {
      if (aResetReason) {
        *aResetReason = HResultToResetReason(hr);
      }
      return true;
    }
  }
  if (mD3D11ContentDevice) {
    HRESULT hr = mD3D11ContentDevice->GetDeviceRemovedReason();
    if (hr != S_OK) {
      if (aResetReason) {
        *aResetReason = HResultToResetReason(hr);
      }
      return true;
    }
  }
  if (GetD3D10Device()) {
    HRESULT hr = GetD3D10Device()->GetDeviceRemovedReason();
    if (hr != S_OK) {
      if (aResetReason) {
        *aResetReason = HResultToResetReason(hr);
      }
      return true;
    }
  }
  return false;
}

void
gfxWindowsPlatform::GetPlatformCMSOutputProfile(void* &mem, size_t &mem_size)
{
    WCHAR str[MAX_PATH];
    DWORD size = MAX_PATH;
    BOOL res;

    mem = nullptr;
    mem_size = 0;

    HDC dc = GetDC(nullptr);
    if (!dc)
        return;

    MOZ_SEH_TRY {
        res = GetICMProfileW(dc, &size, (LPWSTR)&str);
    } MOZ_SEH_EXCEPT(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
        res = FALSE;
    }

    ReleaseDC(nullptr, dc);
    if (!res)
        return;

#ifdef _WIN32
    qcms_data_from_unicode_path(str, &mem, &mem_size);

#ifdef DEBUG_tor
    if (mem_size > 0)
        fprintf(stderr,
                "ICM profile read from %s successfully\n",
                NS_ConvertUTF16toUTF8(str).get());
#endif 
#endif 
}

bool
gfxWindowsPlatform::UseClearTypeForDownloadableFonts()
{
    if (mUseClearTypeForDownloadableFonts == UNINITIALIZED_VALUE) {
        mUseClearTypeForDownloadableFonts = Preferences::GetBool(GFX_DOWNLOADABLE_FONTS_USE_CLEARTYPE, true);
    }

    return mUseClearTypeForDownloadableFonts;
}

bool
gfxWindowsPlatform::UseClearTypeAlways()
{
    if (mUseClearTypeAlways == UNINITIALIZED_VALUE) {
        mUseClearTypeAlways = Preferences::GetBool(GFX_USE_CLEARTYPE_ALWAYS, false);
    }

    return mUseClearTypeAlways;
}

void 
gfxWindowsPlatform::GetDLLVersion(char16ptr_t aDLLPath, nsAString& aVersion)
{
    DWORD versInfoSize, vers[4] = {0};
    
    aVersion.AssignLiteral(MOZ_UTF16("0.0.0.0"));
    versInfoSize = GetFileVersionInfoSizeW(aDLLPath, nullptr);
    nsAutoTArray<BYTE,512> versionInfo;
    
    if (versInfoSize == 0 ||
        !versionInfo.AppendElements(uint32_t(versInfoSize)))
    {
        return;
    }

    if (!GetFileVersionInfoW(aDLLPath, 0, versInfoSize, 
           LPBYTE(versionInfo.Elements())))
    {
        return;
    } 

    UINT len = 0;
    VS_FIXEDFILEINFO *fileInfo = nullptr;
    if (!VerQueryValue(LPBYTE(versionInfo.Elements()), TEXT("\\"),
           (LPVOID *)&fileInfo, &len) ||
        len == 0 ||
        fileInfo == nullptr)
    {
        return;
    }

    DWORD fileVersMS = fileInfo->dwFileVersionMS; 
    DWORD fileVersLS = fileInfo->dwFileVersionLS;

    vers[0] = HIWORD(fileVersMS);
    vers[1] = LOWORD(fileVersMS);
    vers[2] = HIWORD(fileVersLS);
    vers[3] = LOWORD(fileVersLS);

    char buf[256];
    sprintf(buf, "%d.%d.%d.%d", vers[0], vers[1], vers[2], vers[3]);
    aVersion.Assign(NS_ConvertUTF8toUTF16(buf));
}

void 
gfxWindowsPlatform::GetCleartypeParams(nsTArray<ClearTypeParameterInfo>& aParams)
{
    HKEY  hKey, subKey;
    DWORD i, rv, size, type;
    WCHAR displayName[256], subkeyName[256];

    aParams.Clear();

    
    rv = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"Software\\Microsoft\\Avalon.Graphics",
                       0, KEY_READ, &hKey);

    if (rv != ERROR_SUCCESS) {
        return;
    }

    
    for (i = 0, rv = ERROR_SUCCESS; rv != ERROR_NO_MORE_ITEMS; i++) {
        size = ArrayLength(displayName);
        rv = RegEnumKeyExW(hKey, i, displayName, &size,
                           nullptr, nullptr, nullptr, nullptr);
        if (rv != ERROR_SUCCESS) {
            continue;
        }

        ClearTypeParameterInfo ctinfo;
        ctinfo.displayName.Assign(displayName);

        DWORD subrv, value;
        bool foundData = false;

        swprintf_s(subkeyName, ArrayLength(subkeyName),
                   L"Software\\Microsoft\\Avalon.Graphics\\%s", displayName);

        
        subrv = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              subkeyName, 0, KEY_QUERY_VALUE, &subKey);

        if (subrv == ERROR_SUCCESS) {
            size = sizeof(value);
            subrv = RegQueryValueExW(subKey, L"GammaLevel", nullptr, &type,
                                     (LPBYTE)&value, &size);
            if (subrv == ERROR_SUCCESS && type == REG_DWORD) {
                foundData = true;
                ctinfo.gamma = value;
            }

            size = sizeof(value);
            subrv = RegQueryValueExW(subKey, L"PixelStructure", nullptr, &type,
                                     (LPBYTE)&value, &size);
            if (subrv == ERROR_SUCCESS && type == REG_DWORD) {
                foundData = true;
                ctinfo.pixelStructure = value;
            }

            RegCloseKey(subKey);
        }

        
        subrv = RegOpenKeyExW(HKEY_CURRENT_USER,
                              subkeyName, 0, KEY_QUERY_VALUE, &subKey);

        if (subrv == ERROR_SUCCESS) {
            size = sizeof(value);
            subrv = RegQueryValueExW(subKey, L"ClearTypeLevel", nullptr, &type,
                                     (LPBYTE)&value, &size);
            if (subrv == ERROR_SUCCESS && type == REG_DWORD) {
                foundData = true;
                ctinfo.clearTypeLevel = value;
            }
      
            size = sizeof(value);
            subrv = RegQueryValueExW(subKey, L"EnhancedContrastLevel",
                                     nullptr, &type, (LPBYTE)&value, &size);
            if (subrv == ERROR_SUCCESS && type == REG_DWORD) {
                foundData = true;
                ctinfo.enhancedContrast = value;
            }

            RegCloseKey(subKey);
        }

        if (foundData) {
            aParams.AppendElement(ctinfo);
        }
    }

    RegCloseKey(hKey);
}

void
gfxWindowsPlatform::FontsPrefsChanged(const char *aPref)
{
    bool clearTextFontCaches = true;

    gfxPlatform::FontsPrefsChanged(aPref);

    if (!aPref) {
        mUseClearTypeForDownloadableFonts = UNINITIALIZED_VALUE;
        mUseClearTypeAlways = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_DOWNLOADABLE_FONTS_USE_CLEARTYPE, aPref)) {
        mUseClearTypeForDownloadableFonts = UNINITIALIZED_VALUE;
    } else if (!strcmp(GFX_USE_CLEARTYPE_ALWAYS, aPref)) {
        mUseClearTypeAlways = UNINITIALIZED_VALUE;
    } else if (!strncmp(GFX_CLEARTYPE_PARAMS, aPref, strlen(GFX_CLEARTYPE_PARAMS))) {
        SetupClearTypeParams();
    } else {
        clearTextFontCaches = false;
    }

    if (clearTextFontCaches) {    
        gfxFontCache *fc = gfxFontCache::GetCache();
        if (fc) {
            fc->Flush();
        }
    }
}

#define ENHANCED_CONTRAST_REGISTRY_KEY \
    HKEY_CURRENT_USER, "Software\\Microsoft\\Avalon.Graphics\\DISPLAY1\\EnhancedContrastLevel"

void
gfxWindowsPlatform::SetupClearTypeParams()
{
#if CAIRO_HAS_DWRITE_FONT
    if (GetDWriteFactory()) {
        
        
        FLOAT gamma = -1.0;
        FLOAT contrast = -1.0;
        FLOAT level = -1.0;
        int geometry = -1;
        int mode = -1;
        int32_t value;
        if (NS_SUCCEEDED(Preferences::GetInt(GFX_CLEARTYPE_PARAMS_GAMMA, &value))) {
            if (value >= 1000 && value <= 2200) {
                gamma = FLOAT(value / 1000.0);
            }
        }

        if (NS_SUCCEEDED(Preferences::GetInt(GFX_CLEARTYPE_PARAMS_CONTRAST, &value))) {
            if (value >= 0 && value <= 1000) {
                contrast = FLOAT(value / 100.0);
            }
        }

        if (NS_SUCCEEDED(Preferences::GetInt(GFX_CLEARTYPE_PARAMS_LEVEL, &value))) {
            if (value >= 0 && value <= 100) {
                level = FLOAT(value / 100.0);
            }
        }

        if (NS_SUCCEEDED(Preferences::GetInt(GFX_CLEARTYPE_PARAMS_STRUCTURE, &value))) {
            if (value >= 0 && value <= 2) {
                geometry = value;
            }
        }

        if (NS_SUCCEEDED(Preferences::GetInt(GFX_CLEARTYPE_PARAMS_MODE, &value))) {
            if (value >= 0 && value <= 5) {
                mode = value;
            }
        }

        cairo_dwrite_set_cleartype_params(gamma, contrast, level, geometry, mode);

        switch (mode) {
        case DWRITE_RENDERING_MODE_ALIASED:
        case DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC:
            mMeasuringMode = DWRITE_MEASURING_MODE_GDI_CLASSIC;
            break;
        case DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL:
            mMeasuringMode = DWRITE_MEASURING_MODE_GDI_NATURAL;
            break;
        default:
            mMeasuringMode = DWRITE_MEASURING_MODE_NATURAL;
            break;
        }

        nsRefPtr<IDWriteRenderingParams> defaultRenderingParams;
        GetDWriteFactory()->CreateRenderingParams(getter_AddRefs(defaultRenderingParams));
        
        
        if (contrast >= 0.0 && contrast <= 10.0) {
            contrast = contrast;
        } else {
            HKEY hKey;
            if (RegOpenKeyExA(ENHANCED_CONTRAST_REGISTRY_KEY,
                              0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                contrast = defaultRenderingParams->GetEnhancedContrast();
                RegCloseKey(hKey);
            } else {
                contrast = 1.0;
            }
        }

        
        
        if (gamma < 1.0 || gamma > 2.2) {
            gamma = defaultRenderingParams->GetGamma();
        }

        if (level < 0.0 || level > 1.0) {
            level = defaultRenderingParams->GetClearTypeLevel();
        }

        DWRITE_PIXEL_GEOMETRY dwriteGeometry =
          static_cast<DWRITE_PIXEL_GEOMETRY>(geometry);
        DWRITE_RENDERING_MODE renderMode =
          static_cast<DWRITE_RENDERING_MODE>(mode);

        if (dwriteGeometry < DWRITE_PIXEL_GEOMETRY_FLAT ||
            dwriteGeometry > DWRITE_PIXEL_GEOMETRY_BGR) {
            dwriteGeometry = defaultRenderingParams->GetPixelGeometry();
        }

        if (renderMode < DWRITE_RENDERING_MODE_DEFAULT ||
            renderMode > DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC) {
            renderMode = defaultRenderingParams->GetRenderingMode();
        }

        mRenderingParams[TEXT_RENDERING_NO_CLEARTYPE] = defaultRenderingParams;

        GetDWriteFactory()->CreateCustomRenderingParams(gamma, contrast, level,
            dwriteGeometry, renderMode,
            getter_AddRefs(mRenderingParams[TEXT_RENDERING_NORMAL]));

        GetDWriteFactory()->CreateCustomRenderingParams(gamma, contrast, level,
            dwriteGeometry, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC,
            getter_AddRefs(mRenderingParams[TEXT_RENDERING_GDI_CLASSIC]));
    }
#endif
}

void
gfxWindowsPlatform::OnDeviceManagerDestroy(DeviceManagerD3D9* aDeviceManager)
{
  if (aDeviceManager == mDeviceManager) {
    mDeviceManager = nullptr;
  }
}

IDirect3DDevice9*
gfxWindowsPlatform::GetD3D9Device()
{
  DeviceManagerD3D9* manager = GetD3D9DeviceManager();
  return manager ? manager->device() : nullptr;
}

DeviceManagerD3D9*
gfxWindowsPlatform::GetD3D9DeviceManager()
{
  
  
  if (!mDeviceManager &&
      (!gfxPlatform::UsesOffMainThreadCompositing() ||
       CompositorParent::IsInCompositorThread())) {
    mDeviceManager = new DeviceManagerD3D9();
    if (!mDeviceManager->Init()) {
      gfxCriticalError() << "[D3D9] Could not Initialize the DeviceManagerD3D9";
      mDeviceManager = nullptr;
    }
  }

  return mDeviceManager;
}

ID3D11Device*
gfxWindowsPlatform::GetD3D11Device()
{
  if (mD3D11DeviceInitialized) {
    return mD3D11Device;
  }

  InitD3D11Devices();

  return mD3D11Device;
}

ID3D11Device*
gfxWindowsPlatform::GetD3D11ContentDevice()
{
  if (mD3D11DeviceInitialized) {
    return mD3D11ContentDevice;
  }

  InitD3D11Devices();

  return mD3D11ContentDevice;
}

ID3D11Device*
gfxWindowsPlatform::GetD3D11MediaDevice()
{
  if (mD3D11MediaDevice) {
    return mD3D11MediaDevice;
  }

  if (!mCanInitMediaDevice) {
    return nullptr;
  }

  mCanInitMediaDevice = false;

  nsModuleHandle d3d11Module(LoadLibrarySystem32(L"d3d11.dll"));
  decltype(D3D11CreateDevice)* d3d11CreateDevice = (decltype(D3D11CreateDevice)*)
    GetProcAddress(d3d11Module, "D3D11CreateDevice");
  MOZ_ASSERT(d3d11CreateDevice);

  nsTArray<D3D_FEATURE_LEVEL> featureLevels;
  if (IsWin8OrLater()) {
    featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_1);
  }
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_1);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_9_3);

  RefPtr<IDXGIAdapter1> adapter = GetDXGIAdapter();
  MOZ_ASSERT(adapter);

  HRESULT hr = E_INVALIDARG;

  MOZ_SEH_TRY{
    hr = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                           featureLevels.Elements(), featureLevels.Length(),
                           D3D11_SDK_VERSION, byRef(mD3D11MediaDevice), nullptr, nullptr);
  } MOZ_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
    mD3D11MediaDevice = nullptr;
  }

  d3d11Module.disown();

  if (FAILED(hr)) {
    return nullptr;
  }

  mD3D11MediaDevice->SetExceptionMode(0);

  return mD3D11MediaDevice;
}


ReadbackManagerD3D11*
gfxWindowsPlatform::GetReadbackManager()
{
  if (!mD3D11ReadbackManager) {
    mD3D11ReadbackManager = new ReadbackManagerD3D11();
  }

  return mD3D11ReadbackManager;
}

bool
gfxWindowsPlatform::IsOptimus()
{
    static int knowIsOptimus = -1;
    if (knowIsOptimus == -1) {
        
        if (GetModuleHandleA("nvumdshim.dll") ||
            GetModuleHandleA("nvumdshimx.dll"))
        {
            knowIsOptimus = 1;
        } else {
            knowIsOptimus = 0;
        }
    }
    return knowIsOptimus;
}

int
gfxWindowsPlatform::GetScreenDepth() const
{
    
    
    if (!GetSystemMetrics(SM_SAMEDISPLAYFORMAT))
        return 24;

    HDC hdc = GetDC(nullptr);
    if (!hdc)
        return 24;

    int depth = GetDeviceCaps(hdc, BITSPIXEL) *
                GetDeviceCaps(hdc, PLANES);

    ReleaseDC(nullptr, hdc);

    return depth;
}

IDXGIAdapter1*
gfxWindowsPlatform::GetDXGIAdapter()
{
  if (mAdapter) {
    return mAdapter;
  }

  nsModuleHandle dxgiModule(LoadLibrarySystem32(L"dxgi.dll"));
  decltype(CreateDXGIFactory1)* createDXGIFactory1 = (decltype(CreateDXGIFactory1)*)
    GetProcAddress(dxgiModule, "CreateDXGIFactory1");

  
  
  if (createDXGIFactory1) {
    nsRefPtr<IDXGIFactory1> factory1;
    HRESULT hr = createDXGIFactory1(__uuidof(IDXGIFactory1),
                                    getter_AddRefs(factory1));

    if (FAILED(hr) || !factory1) {
      
      
      return nullptr;
    }

    hr = factory1->EnumAdapters1(0, byRef(mAdapter));
    if (FAILED(hr)) {
      
      
      return nullptr;
    }
  }

  
  dxgiModule.disown();

  return mAdapter;
}

bool DoesD3D11DeviceWork(ID3D11Device *device)
{
  static bool checked;
  static bool result;

  if (checked)
      return result;
  checked = true;

  if (gfxPrefs::Direct2DForceEnabled() ||
      gfxPrefs::LayersAccelerationForceEnabled())
  {
    result = true;
    return true;
  }

  if (GetModuleHandleW(L"dlumd32.dll") && GetModuleHandleW(L"igd10umd32.dll")) {
    nsString displayLinkModuleVersionString;
    gfxWindowsPlatform::GetDLLVersion(L"dlumd32.dll", displayLinkModuleVersionString);
    uint64_t displayLinkModuleVersion;
    if (!ParseDriverVersion(displayLinkModuleVersionString, &displayLinkModuleVersion)) {
#if defined(MOZ_CRASHREPORTER)
      CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("DisplayLink: could not parse version\n"));
#endif
      return false;
    }
    if (displayLinkModuleVersion <= V(8,6,1,36484)) {
#if defined(MOZ_CRASHREPORTER)
      CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("DisplayLink: too old version\n"));
#endif
      return false;
    }
  }
  result = true;
  return true;
}



bool DoesD3D11TextureSharingWork(ID3D11Device *device)
{
  static bool checked;
  static bool result;

  if (checked)
      return result;
  checked = true;

  if (gfxPrefs::Direct2DForceEnabled() ||
      gfxPrefs::LayersAccelerationForceEnabled())
  {
    result = true;
    return true;
  }

  if (GetModuleHandleW(L"atidxx32.dll")) {
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (gfxInfo) {
      nsString vendorID, vendorID2;
      gfxInfo->GetAdapterVendorID(vendorID);
      gfxInfo->GetAdapterVendorID2(vendorID2);
      if (vendorID.EqualsLiteral("0x8086") && vendorID2.IsEmpty()) {
#if defined(MOZ_CRASHREPORTER)
        CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("Unexpected Intel/AMD dual-GPU setup\n"));
#endif
        return false;
      }
    }
  }

  RefPtr<ID3D11Texture2D> texture;
  D3D11_TEXTURE2D_DESC desc;
  desc.Width = 32;
  desc.Height = 32;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
  desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  if (FAILED(device->CreateTexture2D(&desc, NULL, byRef(texture)))) {
    return false;
  }

  HANDLE shareHandle;
  nsRefPtr<IDXGIResource> otherResource;
  if (FAILED(texture->QueryInterface(__uuidof(IDXGIResource),
                                     getter_AddRefs(otherResource))))
  {
    return false;
  }

  if (FAILED(otherResource->GetSharedHandle(&shareHandle))) {
    return false;
  }

  nsRefPtr<ID3D11Resource> sharedResource;
  nsRefPtr<ID3D11Texture2D> sharedTexture;
  if (FAILED(device->OpenSharedResource(shareHandle, __uuidof(ID3D11Resource),
                                        getter_AddRefs(sharedResource))))
  {
    return false;
  }

  if (FAILED(sharedResource->QueryInterface(__uuidof(ID3D11Texture2D),
                                            getter_AddRefs(sharedTexture))))
  {
    return false;
  }

  RefPtr<ID3D11ShaderResourceView> sharedView;

  
  if (FAILED(device->CreateShaderResourceView(sharedTexture, NULL, byRef(sharedView)))) {
#if defined(MOZ_CRASHREPORTER)
    CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("CreateShaderResourceView failed\n"));
#endif
    return false;
  }

  result = true;
  return true;
}

void
gfxWindowsPlatform::InitD3D11Devices()
{
  
  
  
  
  
  

  mD3D11DeviceInitialized = true;

  MOZ_ASSERT(!mD3D11Device); 

  bool safeMode = false;
  nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
  if (xr) {
    xr->GetInSafeMode(&safeMode);
  }

  if (safeMode) {
    return;
  }

  bool useWARP = false;

  nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
  if (gfxInfo) {
    int32_t status;
    if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_DIRECT3D_11_LAYERS, &status))) {
      if (status != nsIGfxInfo::FEATURE_STATUS_OK) {

        if (gfxPrefs::LayersD3D11DisableWARP()) {
          return;
        }

        useWARP = true;
      }
    }
  }

  if (gfxPrefs::LayersD3D11ForceWARP()) {
    useWARP = true;
  }

  nsModuleHandle d3d11Module(LoadLibrarySystem32(L"d3d11.dll"));
  decltype(D3D11CreateDevice)* d3d11CreateDevice = (decltype(D3D11CreateDevice)*)
    GetProcAddress(d3d11Module, "D3D11CreateDevice");

  if (!d3d11CreateDevice) {
    
    return;
  }

  nsTArray<D3D_FEATURE_LEVEL> featureLevels;
  if (IsWin8OrLater()) {
    featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_1);
  }
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_1);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_9_3);

  RefPtr<IDXGIAdapter1> adapter;

  if (!useWARP) {
    adapter = GetDXGIAdapter();

    if (!adapter) {
      if (!gfxPrefs::LayersD3D11DisableWARP()) {
        return;
      }
      useWARP = true;
    }
  }

  HRESULT hr = E_INVALIDARG;

  if (!useWARP) {
    MOZ_SEH_TRY {
      hr = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                             
                             
                             
                             D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                             featureLevels.Elements(), featureLevels.Length(),
                             D3D11_SDK_VERSION, byRef(mD3D11Device), nullptr, nullptr);
    } MOZ_SEH_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
      if (gfxPrefs::LayersD3D11DisableWARP()) {
        return;
      }

      useWARP = true;
      adapter = nullptr;
    }

    if (FAILED(hr) || !DoesD3D11DeviceWork(mD3D11Device)) {
      if (gfxPrefs::LayersD3D11DisableWARP()) {
        return;
      }

      useWARP = true;
      adapter = nullptr;
    }
  }

  if (useWARP) {
    MOZ_ASSERT(!gfxPrefs::LayersD3D11DisableWARP());
    MOZ_ASSERT(!mD3D11Device);
    MOZ_ASSERT(!adapter);

    ScopedGfxFeatureReporter reporterWARP("D3D11-WARP", gfxPrefs::LayersD3D11ForceWARP());

    MOZ_SEH_TRY {
      hr = d3d11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                             
                             
                             
                             D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                             featureLevels.Elements(), featureLevels.Length(),
                             D3D11_SDK_VERSION, byRef(mD3D11Device), nullptr, nullptr);

      if (FAILED(hr)) {
        
        gfxCriticalError() << "Failed to initialize WARP D3D11 device!" << hr;
        return;
      }

      mIsWARP = true;
      reporterWARP.SetSuccessful();
    } MOZ_SEH_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
      gfxCriticalError() << "Exception occurred initializing WARP D3D11 device!";
      return;
    }
  }

  mD3D11Device->SetExceptionMode(0);

  
  
  
  if (Factory::SupportsD2D1() && (!useWARP || gfxPrefs::LayersD3D11ForceWARP())) {
    MOZ_ASSERT((useWARP && !adapter) || !useWARP);

    hr = E_INVALIDARG;
    MOZ_SEH_TRY {
      hr = d3d11CreateDevice(adapter, useWARP ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                             D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                             featureLevels.Elements(), featureLevels.Length(),
                             D3D11_SDK_VERSION, byRef(mD3D11ContentDevice), nullptr, nullptr);
    } MOZ_SEH_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
      mD3D11ContentDevice = nullptr;
    }

    if (FAILED(hr)) {
      d3d11Module.disown();
      return;
    }

    mD3D11ContentDevice->SetExceptionMode(0);

    Factory::SetDirect3D11Device(mD3D11ContentDevice);
  }

  if (!useWARP) {
    mCanInitMediaDevice = true;
  }

  
  
  d3d11Module.disown();
}

TemporaryRef<ID3D11Device>
gfxWindowsPlatform::CreateD3D11DecoderDevice()
{
  nsModuleHandle d3d11Module(LoadLibrarySystem32(L"d3d11.dll"));
  decltype(D3D11CreateDevice)* d3d11CreateDevice = (decltype(D3D11CreateDevice)*)
    GetProcAddress(d3d11Module, "D3D11CreateDevice");

   if (!d3d11CreateDevice) {
    
    return nullptr;
  }

  nsTArray<D3D_FEATURE_LEVEL> featureLevels;
  if (IsWin8OrLater()) {
    featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_1);
  }
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_11_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_1);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_10_0);
  featureLevels.AppendElement(D3D_FEATURE_LEVEL_9_3);

  RefPtr<IDXGIAdapter1> adapter = GetDXGIAdapter();

  if (!adapter) {
    return nullptr;
  }

  HRESULT hr = E_INVALIDARG;

  RefPtr<ID3D11Device> device;

  MOZ_SEH_TRY{
    hr = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                           D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
                           featureLevels.Elements(), featureLevels.Length(),
                           D3D11_SDK_VERSION, byRef(device), nullptr, nullptr);
  } MOZ_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }

  if (FAILED(hr) || !DoesD3D11DeviceWork(device)) {
    return nullptr;
  }

  nsRefPtr<ID3D10Multithread> multi;
  device->QueryInterface(__uuidof(ID3D10Multithread), getter_AddRefs(multi));

  multi->SetMultithreadProtected(TRUE);

  return device;
}

static bool
DwmCompositionEnabled()
{
  MOZ_ASSERT(WinUtils::dwmIsCompositionEnabledPtr);
  BOOL dwmEnabled = false;
  WinUtils::dwmIsCompositionEnabledPtr(&dwmEnabled);
  return dwmEnabled;
}

class D3DVsyncSource final : public VsyncSource
{
public:

  class D3DVsyncDisplay final : public VsyncSource::Display
  {
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(D3DVsyncDisplay)
    public:
      D3DVsyncDisplay()
        : mVsyncEnabledLock("D3DVsyncEnabledLock")
        , mVsyncEnabled(false)
      {
        mVsyncThread = new base::Thread("WindowsVsyncThread");
        const double rate = 1000 / 60.0;
        mSoftwareVsyncRate = TimeDuration::FromMilliseconds(rate);
        MOZ_RELEASE_ASSERT(mVsyncThread->Start(), "Could not start Windows vsync thread");
      }

      virtual void EnableVsync() override
      {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mVsyncThread->IsRunning());
        { 
          MonitorAutoLock lock(mVsyncEnabledLock);
          if (mVsyncEnabled) {
            return;
          }
          mVsyncEnabled = true;
        }

        CancelableTask* vsyncStart = NewRunnableMethod(this,
            &D3DVsyncDisplay::VBlankLoop);
        mVsyncThread->message_loop()->PostTask(FROM_HERE, vsyncStart);
      }

      virtual void DisableVsync() override
      {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mVsyncThread->IsRunning());
        MonitorAutoLock lock(mVsyncEnabledLock);
        if (!mVsyncEnabled) {
          return;
        }
        mVsyncEnabled = false;
      }

      virtual bool IsVsyncEnabled() override
      {
        MOZ_ASSERT(NS_IsMainThread());
        MonitorAutoLock lock(mVsyncEnabledLock);
        return mVsyncEnabled;
      }

      void ScheduleSoftwareVsync(TimeStamp aVsyncTimestamp)
      {
        MOZ_ASSERT(IsInVsyncThread());
        NS_WARNING("DwmComposition dynamically disabled, falling back to software timers\n");

        TimeStamp nextVsync = aVsyncTimestamp + mSoftwareVsyncRate;
        TimeDuration delay = nextVsync - TimeStamp::Now();
        if (delay.ToMilliseconds() < 0) {
          delay = mozilla::TimeDuration::FromMilliseconds(0);
        }

        mVsyncThread->message_loop()->PostDelayedTask(FROM_HERE,
            NewRunnableMethod(this, &D3DVsyncDisplay::VBlankLoop),
            delay.ToMilliseconds());
      }

      void VBlankLoop()
      {
        MOZ_ASSERT(IsInVsyncThread());
        MOZ_ASSERT(sizeof(int64_t) == sizeof(QPC_TIME));

        DWM_TIMING_INFO vblankTime;
        
        vblankTime.cbSize = sizeof(DWM_TIMING_INFO);

        LARGE_INTEGER qpcNow;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        TimeStamp vsync = TimeStamp::Now();
        TimeStamp previousVsync = vsync;
        const int microseconds = 1000000;

        for (;;) {
          { 
            MonitorAutoLock lock(mVsyncEnabledLock);
            if (!mVsyncEnabled) return;
          }

          if (previousVsync > vsync) {
            vsync = TimeStamp::Now();
            NS_WARNING("Previous vsync timestamp is ahead of the calculated vsync timestamp.");
          }

          previousVsync = vsync;
          Display::NotifyVsync(vsync);

          
          
          
          
          if (!DwmCompositionEnabled()) {
            ScheduleSoftwareVsync(vsync);
            return;
          }

          
          
          WinUtils::dwmFlushProcPtr();
          HRESULT hr = WinUtils::dwmGetCompositionTimingInfoPtr(0, &vblankTime);
          vsync = TimeStamp::Now();
          if (SUCCEEDED(hr)) {
            QueryPerformanceCounter(&qpcNow);
            
            
            
            int64_t adjust = qpcNow.QuadPart - vblankTime.qpcVBlank;
            int64_t usAdjust = (adjust * microseconds) / frequency.QuadPart;
            vsync -= TimeDuration::FromMicroseconds((double) usAdjust);
          }
        } 
      }

    private:
      virtual ~D3DVsyncDisplay()
      {
        MOZ_ASSERT(NS_IsMainThread());
        DisableVsync();
        mVsyncThread->Stop();
        delete mVsyncThread;
      }

      bool IsInVsyncThread()
      {
        return mVsyncThread->thread_id() == PlatformThread::CurrentId();
      }

      TimeDuration mSoftwareVsyncRate;
      Monitor mVsyncEnabledLock;
      base::Thread* mVsyncThread;
      bool mVsyncEnabled;
  }; 

  D3DVsyncSource()
  {
    mPrimaryDisplay = new D3DVsyncDisplay();
  }

  virtual Display& GetGlobalDisplay() override
  {
    return *mPrimaryDisplay;
  }

private:
  virtual ~D3DVsyncSource()
  {
  }
  nsRefPtr<D3DVsyncDisplay> mPrimaryDisplay;
}; 

already_AddRefed<mozilla::gfx::VsyncSource>
gfxWindowsPlatform::CreateHardwareVsyncSource()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  if (!WinUtils::dwmIsCompositionEnabledPtr) {
    NS_WARNING("Dwm composition not available, falling back to software vsync\n");
    return gfxPlatform::CreateHardwareVsyncSource();
  }

  BOOL dwmEnabled = false;
  WinUtils::dwmIsCompositionEnabledPtr(&dwmEnabled);
  if (!dwmEnabled) {
    NS_WARNING("DWM not enabled, falling back to software vsync\n");
    return gfxPlatform::CreateHardwareVsyncSource();
  }

  nsRefPtr<VsyncSource> d3dVsyncSource = new D3DVsyncSource();
  return d3dVsyncSource.forget();
}

bool
gfxWindowsPlatform::SupportsApzTouchInput()
{
  int value = Preferences::GetInt("dom.w3c_touch_events.enabled", 0);
  return value == 1 || value == 2;
}
