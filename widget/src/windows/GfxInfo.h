






































#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include "GfxInfoBase.h"
#include "nsIGfxInfoDebug.h"

namespace mozilla {
namespace widget {

class GfxInfo : public GfxInfoBase
#ifdef DEBUG
              , public nsIGfxInfoDebug
#endif
{
public:
  GfxInfo();

  
  
  NS_SCRIPTABLE NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetAzureEnabled(bool *aAzureEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion);
  NS_SCRIPTABLE NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterVendorID(PRUint32 *aAdapterVendorID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDeviceID(PRUint32 *aAdapterDeviceID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterVendorID2(PRUint32 *aAdapterVendorID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDeviceID2(PRUint32 *aAdapterDeviceID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate);
  NS_SCRIPTABLE NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active);
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

#ifdef DEBUG
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIGFXINFODEBUG
#endif

  virtual nsresult Init();

protected:

  virtual nsresult GetFeatureStatusImpl(PRInt32 aFeature, 
                                        PRInt32 *aStatus, 
                                        nsAString & aSuggestedDriverVersion, 
                                        const nsTArray<GfxDriverInfo>& aDriverInfo, 
                                        OperatingSystem* aOS = nsnull);
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo();

private:

  void AddCrashReportAnnotations();
  nsString mDeviceString;
  nsString mDeviceID;
  nsString mDriverVersion;
  nsString mDriverDate;
  nsString mDeviceKey;
  nsString mDeviceKeyDebug;
  PRUint32 mAdapterVendorID;
  PRUint32 mAdapterDeviceID;
  PRUint32 mAdapterSubsysID;
  nsString mDeviceString2;
  nsString mDriverVersion2;
  nsString mDeviceID2;
  nsString mDriverDate2;
  nsString mDeviceKey2;
  PRUint32 mAdapterVendorID2;
  PRUint32 mAdapterDeviceID2;
  PRUint32 mAdapterSubsysID2;
  PRUint32 mWindowsVersion;
  bool mHasDualGPU;
  bool mIsGPU2Active;
  bool mHasDriverVersionMismatch;
};

} 
} 

#endif 
