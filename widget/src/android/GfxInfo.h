






































#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include "GfxInfoBase.h"
#include "GfxDriverInfo.h"

#include "nsString.h"

namespace mozilla {
namespace widget {

class GfxInfo : public GfxInfoBase
{
public:
  
  
  NS_SCRIPTABLE NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetAzureEnabled(bool *aAzureEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion);
  NS_SCRIPTABLE NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterVendorID(nsAString & aAdapterVendorID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDeviceID(nsAString & aAdapterDeviceID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterVendorID2(nsAString & aAdapterVendorID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDeviceID2(nsAString & aAdapterDeviceID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate);
  NS_SCRIPTABLE NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active);
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init();

protected:

  virtual nsresult GetFeatureStatusImpl(PRInt32 aFeature, 
                                        PRInt32 *aStatus, 
                                        nsAString & aSuggestedDriverVersion, 
                                        const nsTArray<GfxDriverInfo>& aDriverInfo,
                                        OperatingSystem* aOS = nsnull);
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo();

private:

  void     AddOpenGLCrashReportAnnotations();
  nsString mRendererIDsString;
  nsString mAdapterRAMString;

  nsString mDeviceID;
  nsString mDriverVersion;
  nsString mDriverDate;
  nsString mDeviceKey;

  nsString mAdapterDeviceID;
  nsString mAdapterVendorID;
  nsString mAdapterDescription;
  PRInt32 mAndroidSDKVersion;

  PRUint32 mRendererIDs[16];
};

} 
} 

#endif 
