




#include "GfxInfo.h"
#include "nsUnicharUtils.h"
#include "mozilla/FunctionTimer.h"
#include "prenv.h"
#include "prprf.h"
#include "nsHashKeys.h"

#include "AndroidBridge.h"

#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#endif

using namespace mozilla::widget;

#ifdef DEBUG
NS_IMPL_ISUPPORTS_INHERITED1(GfxInfo, GfxInfoBase, nsIGfxInfoDebug)
#endif

GfxInfo::GfxInfo()
  : mInitializedFromJavaData(false)
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

nsresult
GfxInfo::GetAzureEnabled(bool *aEnabled)
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
GfxInfo::EnsureInitializedFromGfxInfoData()
{
  if (mInitializedFromJavaData)
    return;
  mInitializedFromJavaData = true;

  {
    nsCString gfxInfoData;
    mozilla::AndroidBridge::Bridge()->GetGfxInfoData(gfxInfoData);

    
    
    
    
    
    
    nsCString *stringToFill = nsnull;
    char *bufptr = gfxInfoData.BeginWriting();

    while(true) {
      char *line = NS_strtok("\n", &bufptr);
      if (!line)
        break;
      if (stringToFill) {
        stringToFill->Assign(line);
        stringToFill = nsnull;
      } else if(!strcmp(line, "VENDOR")) {
        stringToFill = &mVendor;
      } else if(!strcmp(line, "RENDERER")) {
        stringToFill = &mRenderer;
      } else if(!strcmp(line, "VERSION")) {
        stringToFill = &mVersion;
      } else if(!strcmp(line, "ERROR")) {
        stringToFill = &mError;
      }
    }
  }


  if (!mError.IsEmpty()) {
    mAdapterDescription.AppendPrintf("An error occurred earlier while querying gfx info: %s. ",
                                     mError.get());
    printf_stderr("%s\n", mAdapterDescription.get());
  }

  const char *spoofedVendor = PR_GetEnv("MOZ_GFX_SPOOF_GL_VENDOR");
  if (spoofedVendor)
      mVendor.Assign(spoofedVendor);
  const char *spoofedRenderer = PR_GetEnv("MOZ_GFX_SPOOF_GL_RENDERER");
  if (spoofedRenderer)
      mRenderer.Assign(spoofedRenderer);
  const char *spoofedVersion = PR_GetEnv("MOZ_GFX_SPOOF_GL_VERSION");
  if (spoofedVersion)
      mVersion.Assign(spoofedVersion);

  mAdapterDescription.AppendPrintf("%s -- %s -- %s",
                                   mVendor.get(),
                                   mRenderer.get(),
                                   mVersion.get());

  
  
  
  
  if (mozilla::AndroidBridge::Bridge()) {
    nsAutoString str;
    if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MODEL", str)) {
      mAdapterDescription.AppendPrintf(" -- Model: %s",  NS_LossyConvertUTF16toASCII(str).get());
    }

    if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "PRODUCT", str)) {
      mAdapterDescription.AppendPrintf(", Product: %s", NS_LossyConvertUTF16toASCII(str).get());
    }

    if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MANUFACTURER", str)) {
      mAdapterDescription.AppendPrintf(", Manufacturer: %s", NS_LossyConvertUTF16toASCII(str).get());
    }

    int32_t version; 
    if (!mozilla::AndroidBridge::Bridge()->GetStaticIntField("android/os/Build$VERSION", "SDK_INT", &version))
      version = 0;

    if (version >= 8 && mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", str)) {
      if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", str)) {
        mAdapterDescription.AppendPrintf(", Hardware: %s", NS_LossyConvertUTF16toASCII(str).get());
      }
    }
  }

  AddCrashReportAnnotations();
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription = NS_ConvertASCIItoUTF16(mAdapterDescription);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription2(nsAString & aAdapterDescription)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  aAdapterRAM.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM2(nsAString & aAdapterRAM)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  aAdapterDriver.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver2(nsAString & aAdapterDriver)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  aAdapterDriverVersion = NS_ConvertASCIItoUTF16(mVersion);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate2(nsAString & aAdapterDriverDate)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(nsAString & aAdapterVendorID)
{
  aAdapterVendorID = NS_ConvertASCIItoUTF16(mVendor);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID2(nsAString & aAdapterVendorID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(nsAString & aAdapterDeviceID)
{
  aAdapterDeviceID = NS_ConvertASCIItoUTF16(mRenderer);
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID2(nsAString & aAdapterDeviceID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetIsGPU2Active(bool* aIsGPU2Active)
{
  return NS_ERROR_FAILURE;
}

void
GfxInfo::AddCrashReportAnnotations()
{
#if defined(MOZ_CRASHREPORTER)
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterVendorID"),
                                     mVendor);
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDeviceID"),
                                     mRenderer);

  

  nsCAutoString note;
  note.AppendPrintf("AdapterDescription: '%s'\n", mAdapterDescription.get());

  CrashReporter::AppendAppNotesToCrashReport(note);
#endif
}

const nsTArray<GfxDriverInfo>&
GfxInfo::GetGfxDriverInfo()
{
  if (mDriverInfo->IsEmpty()) {
#ifdef MOZ_JAVA_COMPOSITOR
    APPEND_TO_DRIVER_BLOCKLIST2( DRIVER_OS_ALL,
      (nsAString&) GfxDriverInfo::GetDeviceVendor(VendorAll), GfxDriverInfo::allDevices,
      nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_NO_INFO,
      DRIVER_COMPARISON_IGNORED, GfxDriverInfo::allDriverVersions );
#else
    APPEND_TO_DRIVER_BLOCKLIST2( DRIVER_OS_ALL,
      (nsAString&) GfxDriverInfo::GetDeviceVendor(VendorAll), GfxDriverInfo::allDevices,
      nsIGfxInfo::FEATURE_OPENGL_LAYERS, nsIGfxInfo::FEATURE_BLOCKED_DEVICE,
      DRIVER_COMPARISON_IGNORED, GfxDriverInfo::allDriverVersions );
#endif
  }

  return *mDriverInfo;
}

nsresult
GfxInfo::GetFeatureStatusImpl(PRInt32 aFeature, 
                              PRInt32 *aStatus, 
                              nsAString & aSuggestedDriverVersion,
                              const nsTArray<GfxDriverInfo>& aDriverInfo, 
                              OperatingSystem* aOS )
{
  NS_ENSURE_ARG_POINTER(aStatus);
  aSuggestedDriverVersion.SetIsVoid(true);
  *aStatus = nsIGfxInfo::FEATURE_STATUS_UNKNOWN;
  OperatingSystem os = DRIVER_OS_ANDROID;
  if (aOS)
    *aOS = os;

  EnsureInitializedFromGfxInfoData();

  if (!mError.IsEmpty()) {
    *aStatus = nsIGfxInfo::FEATURE_BLOCKED_DEVICE;
    return NS_OK;
  }

  
  if (aDriverInfo.IsEmpty()) {
    if (aFeature == FEATURE_WEBGL_OPENGL) {
      if (mRenderer.Find("Adreno 200") != -1 ||
          mRenderer.Find("Adreno 205") != -1)
      {
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
  EnsureInitializedFromGfxInfoData(); 
  mVendor = NS_LossyConvertUTF16toASCII(aVendorID);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDeviceID(const nsAString & aDeviceID)
{
  EnsureInitializedFromGfxInfoData(); 
  mRenderer = NS_LossyConvertUTF16toASCII(aDeviceID);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofDriverVersion(const nsAString & aDriverVersion)
{
  EnsureInitializedFromGfxInfoData(); 
  mVersion = NS_LossyConvertUTF16toASCII(aDriverVersion);
  return NS_OK;
}


NS_IMETHODIMP GfxInfo::SpoofOSVersion(PRUint32 aVersion)
{
  return NS_OK;
}

#endif
