


































#include "GfxInfo.h"
#include "nsUnicharUtils.h"
#include "mozilla/FunctionTimer.h"
#include "prenv.h"
#include "prprf.h"
#include "EGLUtils.h"

#include "AndroidBridge.h"

#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#endif

using namespace mozilla::widget;



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

nsresult
GfxInfo::Init()
{
  return GfxInfoBase::Init();
}


NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription.AssignASCII(mozilla::gl::GetVendor());
  if (mozilla::AndroidBridge::Bridge()) {
      nsAutoString str;
      aAdapterDescription.Append(NS_LITERAL_STRING(" "));
      if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MODEL", str))
        aAdapterDescription.Append(str);
      aAdapterDescription.Append(NS_LITERAL_STRING(" "));
      if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "MANUFACTURER", str))
        aAdapterDescription.Append(str);
      aAdapterDescription.Append(NS_LITERAL_STRING(" "));
      if (mozilla::AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "HARDWARE", str))
        aAdapterDescription.Append(str);
  }

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
  aAdapterDriverVersion.AssignLiteral("");
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
GfxInfo::GetAdapterVendorID(PRUint32 *aAdapterVendorID)
{
  *aAdapterVendorID = 0;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID2(PRUint32 *aAdapterVendorID)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
{
  *aAdapterDeviceID = 0;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID2(PRUint32 *aAdapterDeviceID)
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
#if 0
#if defined(MOZ_CRASHREPORTER)
  nsCAutoString deviceIDString, vendorIDString;
  PRUint32 deviceID, vendorID;

  GetAdapterDeviceID(&deviceID);
  GetAdapterVendorID(&vendorID);

  deviceIDString.AppendPrintf("%04x", deviceID);
  vendorIDString.AppendPrintf("%04x", vendorID);

  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterVendorID"),
      vendorIDString);
  CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("AdapterDeviceID"),
      deviceIDString);

  

  nsCAutoString note;
  
  note.AppendPrintf("AdapterVendorID: %04x, ", vendorID);
  note.AppendPrintf("AdapterDeviceID: %04x", deviceID);

  if (vendorID == 0) {
      
      note.Append(", ");
      note.AppendWithConversion(mDeviceID);
      note.Append(", ");
      note.AppendWithConversion(mDeviceKeyDebug);
  }
  note.Append("\n");

  CrashReporter::AppendAppNotesToCrashReport(note);

#endif
#endif
}

nsresult
GfxInfo::GetFeatureStatusImpl(PRInt32 aFeature, PRInt32 *aStatus, nsAString & aSuggestedDriverVersion,
                              GfxDriverInfo* aDriverInfo )
{
  PRInt32 status = nsIGfxInfo::FEATURE_NO_INFO;

  aSuggestedDriverVersion.SetIsVoid(true);

  
  if (aDriverInfo) {
    *aStatus = status;
    return NS_OK;
  }

  if (aFeature == FEATURE_OPENGL_LAYERS) {
    
    
    
    
    
    
    
    

    status = FEATURE_BLOCKED_DEVICE;
  }

  *aStatus = status;
  return NS_OK;
}
