




#include "GfxInfo.h"
#include "GLContext.h"
#include "GLContextProvider.h"
#include "nsUnicharUtils.h"
#include "prenv.h"
#include "prprf.h"
#include "nsHashKeys.h"
#include "nsVersionComparator.h"
#include "AndroidBridge.h"
#include "nsIWindowWatcher.h"
#include "nsServiceManagerUtils.h"

#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#endif

namespace mozilla {
namespace widget {

class GfxInfo::GLStrings
{
  nsCString mVendor;
  nsCString mRenderer;
  nsCString mVersion;
  bool mReady;

public:
  GLStrings()
    : mReady(false)
  {}

  const nsCString& Vendor() {
    EnsureInitialized();
    return mVendor;
  }

  void SpoofVendor(const nsCString& s) {
    EnsureInitialized();
    mVendor = s;
  }

  const nsCString& Renderer() {
    EnsureInitialized();
    return mRenderer;
  }

  void SpoofRenderer(const nsCString& s) {
    EnsureInitialized();
    mRenderer = s;
  }

  const nsCString& Version() {
    EnsureInitialized();
    return mVersion;
  }

  void SpoofVersion(const nsCString& s) {
    EnsureInitialized();
    mVersion = s;
  }

  void EnsureInitialized() {
    if (mReady) {
      return;
    }

    nsRefPtr<gl::GLContext> gl;
    gl = gl::GLContextProvider::CreateOffscreen(gfxIntSize(16, 16),
                                                gl::SurfaceCaps::ForRGB());

    if (!gl) {
      
      
      
      mReady = true;
      return;
    }

    gl->MakeCurrent();

    const char *spoofedVendor = PR_GetEnv("MOZ_GFX_SPOOF_GL_VENDOR");
    if (spoofedVendor)
        mVendor.Assign(spoofedVendor);
    else
        mVendor.Assign((const char*)gl->fGetString(LOCAL_GL_VENDOR));
    const char *spoofedRenderer = PR_GetEnv("MOZ_GFX_SPOOF_GL_RENDERER");
    if (spoofedRenderer)
        mRenderer.Assign(spoofedRenderer);
    else
        mRenderer.Assign((const char*)gl->fGetString(LOCAL_GL_RENDERER));
    const char *spoofedVersion = PR_GetEnv("MOZ_GFX_SPOOF_GL_VERSION");
    if (spoofedVersion)
        mVersion.Assign(spoofedVersion);
    else
        mVersion.Assign((const char*)gl->fGetString(LOCAL_GL_VERSION));

    mReady = true;
  }
};

#ifdef DEBUG
NS_IMPL_ISUPPORTS_INHERITED(GfxInfo, GfxInfoBase, nsIGfxInfoDebug)
#endif

GfxInfo::GfxInfo()
  : mInitialized(false)
  , mGLStrings(new GLStrings)
{
}

GfxInfo::~GfxInfo()
{
}



nsresult
GfxInfo::GetD2DEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

nsresult
GfxInfo::GetDWriteEnabled(bool *aEnabled)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetDWriteVersion(nsAString & aDwriteVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetCleartypeParameters(nsAString & aCleartypeParams)
{
  return NS_ERROR_FAILURE;
}

void
GfxInfo::EnsureInitialized()
{
  if (mInitialized)
    return;

  mGLStrings->EnsureInitialized();

  MOZ_ASSERT(mozilla::AndroidBridge::Bridge());

  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MODEL", mModel)) {
    mAdapterDescription.AppendPrintf("Model: %s",  NS_LossyConvertUTF16toASCII(mModel).get());
  }

  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "PRODUCT", mProduct)) {
    mAdapterDescription.AppendPrintf(", Product: %s", NS_LossyConvertUTF16toASCII(mProduct).get());
  }

  if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MANUFACTURER", mManufacturer)) {
    mAdapterDescription.AppendPrintf(", Manufacturer: %s", NS_LossyConvertUTF16toASCII(mManufacturer).get());
  }

  int32_t sdkVersion;
  if (!mozilla::AndroidBridge::Bridge()->GetStaticIntField("android/os/Build$VERSION", "SDK_INT", &sdkVersion))
    sdkVersion = 0;

  
  if (sdkVersion >= 8 && mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", mHardware)) {
    mAdapterDescription.AppendPrintf(", Hardware: %s", NS_LossyConvertUTF16toASCII(mHardware).get());
  }

  nsString release;
  mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build$VERSION", "RELEASE", release);
  mOSVersion = NS_LossyConvertUTF16toASCII(release);

  mOSVersionInteger = 0;
  char a[5], b[5], c[5], d[5];
  SplitDriverVersion(mOSVersion.get(), a, b, c, d);
  uint8_t na = atoi(a);
  uint8_t nb = atoi(b);
  uint8_t nc = atoi(c);
  uint8_t nd = atoi(d);

  mOSVersionInteger = (uint32_t(na) << 24) |
                      (uint32_t(nb) << 16) |
                      (uint32_t(nc) << 8)  |
                      uint32_t(nd);

  mAdapterDescription.AppendPrintf(", OpenGL: %s -- %s -- %s",
                                   mGLStrings->Vendor().get(),
                                   mGLStrings->Renderer().get(),
                                   mGLStrings->Version().get());

  AddCrashReportAnnotations();

  mInitialized = true;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  EnsureInitialized();
  aAdapterDescription = NS_ConvertASCIItoUTF16(mAdapterDescription);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription2(nsAString & aAdapterDescription)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  EnsureInitialized();
  aAdapterRAM.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM2(nsAString & aAdapterRAM)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  EnsureInitialized();
  aAdapterDriver.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver2(nsAString & aAdapterDriver)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  EnsureInitialized();
  aAdapterDriverVersion = NS_ConvertASCIItoUTF16(mGLStrings->Version());
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  EnsureInitialized();
  aAdapterDriverDate.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate2(nsAString & aAdapterDriverDate)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(nsAString & aAdapterVendorID)
{
  EnsureInitialized();
  aAdapterVendorID = NS_ConvertASCIItoUTF16(mGLStrings->Vendor());
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID2(nsAString & aAdapterVendorID)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(nsAString & aAdapterDeviceID)
{
  EnsureInitialized();
  aAdapterDeviceID = NS_ConvertASCIItoUTF16(mGLStrings->Renderer());
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID2(nsAString & aAdapterDeviceID)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterSubsysID(nsAString & aAdapterSubsysID)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterSubsysID2(nsAString & aAdapterSubsysID)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetIsGPU2Active(bool* aIsGPU2Active)
{
  EnsureInitialized();
  return NS_ERROR_FAILURE;
}

void
GfxInfo::AddCrashReportAnnotations()
{
#if defined(MOZ_CRASHREPORTER)
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterVendorID"),
                                     mGLStrings->Vendor());
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDeviceID"),
                                     mGLStrings->Renderer());
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDriverVersion"),
                                     mGLStrings->Version());

  

  nsAutoCString note;
  note.AppendPrintf("AdapterDescription: '%s'\n", mAdapterDescription.get());

  CrashReporter::AppendAppNotesToCrashReport(note);
#endif
}

const nsTArray<GfxDriverInfo>&
GfxInfo::GetGfxDriverInfo()
{
  if (mDriverInfo->IsEmpty()) {
    APPEND_TO_DRIVER_BLOCKLIST2( DRIVER_OS_ALL,
      (nsAString&) GfxDriverInfo::GetDeviceVendor(VendorAll), GfxDriverInfo::allDevices,
      nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_STATUS_OK,
      DRIVER_COMPARISON_IGNORED, GfxDriverInfo::allDriverVersions );
  }

  return *mDriverInfo;
}

nsresult
GfxInfo::GetFeatureStatusImpl(int32_t aFeature,
                              int32_t *aStatus,
                              nsAString & aSuggestedDriverVersion,
                              const nsTArray<GfxDriverInfo>& aDriverInfo,
                              OperatingSystem* aOS )
{
  NS_ENSURE_ARG_POINTER(aStatus);
  aSuggestedDriverVersion.SetIsVoid(true);
  *aStatus = nsIGfxInfo::FEATURE_STATUS_UNKNOWN;
  OperatingSystem os = mOS;
  if (aOS)
    *aOS = os;

  
  
  
  if (aFeature == nsIGfxInfo::FEATURE_OPENGL_LAYERS) {
    *aStatus = nsIGfxInfo::FEATURE_STATUS_OK;
    return NS_OK;
  }

  EnsureInitialized();

  if (mGLStrings->Vendor().IsEmpty() || mGLStrings->Renderer().IsEmpty()) {
    *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
    return NS_OK;
  }

  
  if (aDriverInfo.IsEmpty()) {
    if (aFeature == FEATURE_WEBGL_OPENGL) {
      if (mGLStrings->Renderer().Find("Adreno 200") != -1 ||
          mGLStrings->Renderer().Find("Adreno 205") != -1)
      {
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
        return NS_OK;
      }

      if (mHardware.EqualsLiteral("ville")) {
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
        return NS_OK;
      }
    }

    if (aFeature == FEATURE_STAGEFRIGHT) {
      NS_LossyConvertUTF16toASCII cManufacturer(mManufacturer);
      NS_LossyConvertUTF16toASCII cModel(mModel);
      NS_LossyConvertUTF16toASCII cHardware(mHardware);

      if (cHardware.EqualsLiteral("antares") ||
          cHardware.EqualsLiteral("harmony") ||
          cHardware.EqualsLiteral("picasso") ||
          cHardware.EqualsLiteral("picasso_e") ||
          cHardware.EqualsLiteral("ventana") ||
          cHardware.EqualsLiteral("rk30board"))
      {
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
        return NS_OK;
      }

      if (CompareVersions(mOSVersion.get(), "2.2.0") >= 0 &&
          CompareVersions(mOSVersion.get(), "2.3.0") < 0)
      {
        
        
        bool isWhitelisted =
          cManufacturer.Equals("lge", nsCaseInsensitiveCStringComparator());

        if (!isWhitelisted) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
      else if (CompareVersions(mOSVersion.get(), "2.3.0") >= 0 &&
          CompareVersions(mOSVersion.get(), "2.4.0") < 0)
      {
        
        
        
        
        
        bool isWhitelisted =
          cManufacturer.Equals("htc", nsCaseInsensitiveCStringComparator()) ||
          (cManufacturer.Find("sony", true) != -1) ||
          cManufacturer.Equals("samsung", nsCaseInsensitiveCStringComparator());

        if (cModel.Equals("GT-I8160", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-I8160L", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-I8530", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-I9070", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-I9070P", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-I8160P", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-S7500", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-S7500T", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-S7500L", nsCaseInsensitiveCStringComparator()) ||
            cModel.Equals("GT-S6500T", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("smdkc110", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("smdkc210", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("herring", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("shw-m110s", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("shw-m180s", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("n1", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("latona", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("aalto", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("atlas", nsCaseInsensitiveCStringComparator()) ||
            cHardware.Equals("qcom", nsCaseInsensitiveCStringComparator()))
        {
          isWhitelisted = false;
        }

        if (!isWhitelisted) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
      else if (CompareVersions(mOSVersion.get(), "3.0.0") >= 0 &&
          CompareVersions(mOSVersion.get(), "4.0.0") < 0)
      {
        
        
        bool isWhitelisted =
          cManufacturer.Equals("samsung", nsCaseInsensitiveCStringComparator());

        if (!isWhitelisted) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
      else if (CompareVersions(mOSVersion.get(), "4.0.0") < 0)
      {
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_OS_VERSION;
        return NS_OK;
      }
      else if (CompareVersions(mOSVersion.get(), "4.1.0") < 0)
      {
        
        
        
        
        
        
        
        bool isWhitelisted =
          cModel.Equals("LT28h", nsCaseInsensitiveCStringComparator()) ||
          cManufacturer.Equals("samsung", nsCaseInsensitiveCStringComparator()) ||
          cModel.Equals("galaxy nexus", nsCaseInsensitiveCStringComparator()); 

        if (cModel.Find("SGH-I717", true) != -1 ||
            cModel.Find("SGH-I727", true) != -1 ||
            cModel.Find("SGH-I757", true) != -1)
        {
          isWhitelisted = false;
        }

        if (!isWhitelisted) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
      else if (CompareVersions(mOSVersion.get(), "4.2.0") < 0)
      {
        
        
        
        
        
        bool isBlocklisted =
          cModel.Find("GT-P3100", true) != -1 ||
          cModel.Find("GT-P3110", true) != -1 ||
          cModel.Find("GT-P3113", true) != -1 ||
          cModel.Find("GT-P5100", true) != -1 ||
          cModel.Find("GT-P5110", true) != -1 ||
          cModel.Find("GT-P5113", true) != -1 ||
          cModel.Find("XT890", true) != -1;

        if (isBlocklisted) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
      else if (CompareVersions(mOSVersion.get(), "4.3.0") < 0)
      {
        
        if (cManufacturer.Find("Sony", true) != -1) {
          *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
          return NS_OK;
        }
      }
    }

    if (aFeature == FEATURE_WEBRTC_HW_ACCELERATION) {
      NS_LossyConvertUTF16toASCII cManufacturer(mManufacturer);
      NS_LossyConvertUTF16toASCII cModel(mModel);
      NS_LossyConvertUTF16toASCII cHardware(mHardware);

      if (cHardware.EqualsLiteral("hammerhead") &&
          CompareVersions(mOSVersion.get(), "4.4.2") >= 0 &&
          cManufacturer.Equals("lge", nsCaseInsensitiveCStringComparator()) &&
          cModel.Equals("nexus 5", nsCaseInsensitiveCStringComparator())) {
        *aStatus = nsIGfxInfo::FEATURE_STATUS_OK;
        return NS_OK;
      } else {
        
        *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
        return NS_OK;
      }
    }
  }

  return GfxInfoBase::GetFeatureStatusImpl(aFeature, aStatus, aSuggestedDriverVersion, aDriverInfo, &os);
}

#ifdef DEBUG




NS_IMETHODIMP GfxInfo::SpoofVendorID(const nsAString & aVendorID)
{
  EnsureInitialized();
  mGLStrings->SpoofVendor(NS_LossyConvertUTF16toASCII(aVendorID));
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDeviceID(const nsAString & aDeviceID)
{
  EnsureInitialized();
  mGLStrings->SpoofRenderer(NS_LossyConvertUTF16toASCII(aDeviceID));
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDriverVersion(const nsAString & aDriverVersion)
{
  EnsureInitialized();
  mGLStrings->SpoofVersion(NS_LossyConvertUTF16toASCII(aDriverVersion));
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofOSVersion(uint32_t aVersion)
{
  EnsureInitialized();
  mOSVersion = aVersion;
  return NS_OK;
}

#endif

nsString GfxInfo::Model()
{
  EnsureInitialized();
  return mModel;
}

nsString GfxInfo::Hardware()
{
  EnsureInitialized();
  return mHardware;
}

nsString GfxInfo::Product()
{
  EnsureInitialized();
  return mProduct;
}

nsString GfxInfo::Manufacturer()
{
  EnsureInitialized();
  return mManufacturer;
}

uint32_t GfxInfo::OperatingSystemVersion()
{
  EnsureInitialized();
  return mOSVersionInteger;
}

}
}
