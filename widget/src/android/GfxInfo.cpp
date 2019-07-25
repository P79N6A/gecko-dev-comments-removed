


































#include "GfxInfo.h"
#include "GfxInfoWebGL.h"
#include "nsUnicharUtils.h"
#include "nsPrintfCString.h"
#include "mozilla/FunctionTimer.h"
#include "prenv.h"
#include "prprf.h"
#include "EGLUtils.h"

#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
#include "nsExceptionHandler.h"
#include "nsICrashReporter.h"
#define NS_CRASHREPORTER_CONTRACTID "@mozilla.org/toolkit/crash-reporter;1"
#include "nsIPrefService.h"
#endif


using namespace mozilla::widget;


NS_IMPL_ISUPPORTS1(GfxInfo, nsIGfxInfo)



nsresult
GfxInfo::GetD2DEnabled(PRBool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

nsresult
GfxInfo::GetDWriteEnabled(PRBool *aEnabled)
{
  return NS_ERROR_FAILURE;
}

void
GfxInfo::Init()
{
}



NS_IMETHODIMP
GfxInfo::GetAdapterDescription(nsAString & aAdapterDescription)
{
  aAdapterDescription.AssignASCII(mozilla::gl::GetVendor());
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterRAM(nsAString & aAdapterRAM)
{
  aAdapterRAM.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriver(nsAString & aAdapterDriver)
{
  aAdapterDriver.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverVersion(nsAString & aAdapterDriverVersion)
{
  aAdapterDriverVersion.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDriverDate(nsAString & aAdapterDriverDate)
{
  aAdapterDriverDate.AssignLiteral("");
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterVendorID(PRUint32 *aAdapterVendorID)
{
  *aAdapterVendorID = 0;
  return NS_OK;
}


NS_IMETHODIMP
GfxInfo::GetAdapterDeviceID(PRUint32 *aAdapterDeviceID)
{
  *aAdapterDeviceID = 0;
  return NS_OK;
}

void
GfxInfo::AddCrashReportAnnotations()
{
#if 0
#if defined(MOZ_CRASHREPORTER) && defined(MOZ_ENABLE_LIBXUL)
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

NS_IMETHODIMP
GfxInfo::GetFeatureStatus(PRInt32 aFeature, PRInt32 *aStatus)
{
  PRInt32 status = nsIGfxInfo::FEATURE_NO_INFO;
  *aStatus = status;
  return NS_OK;
}

NS_IMETHODIMP
GfxInfo::GetFeatureSuggestedDriverVersion(PRInt32 aFeature, nsAString& aSuggestedDriverVersion)
{
  return NS_OK;
}

NS_IMETHODIMP
GfxInfo::GetWebGLParameter(const nsAString& aParam, nsAString& aResult)
{
  return GfxInfoWebGL::GetWebGLParameter(aParam, aResult);
}
