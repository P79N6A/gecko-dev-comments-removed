






#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include "GfxInfoBase.h"

namespace mozilla {
namespace widget {

class GfxInfo : public GfxInfoBase
{
public:
  GfxInfo();

  
  
  NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled);
  NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled);
  NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion);
  NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams);
  NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription);
  NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver);
  NS_IMETHOD GetAdapterVendorID(nsAString & aAdapterVendorID);
  NS_IMETHOD GetAdapterDeviceID(nsAString & aAdapterDeviceID);
  NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM);
  NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion);
  NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate);
  NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription);
  NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver);
  NS_IMETHOD GetAdapterVendorID2(nsAString & aAdapterVendorID);
  NS_IMETHOD GetAdapterDeviceID2(nsAString & aAdapterDeviceID);
  NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM);
  NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion);
  NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate);
  NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active);
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init();

#ifdef DEBUG
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIGFXINFODEBUG
#endif

protected:

  virtual nsresult GetFeatureStatusImpl(int32_t aFeature, 
                                        int32_t *aStatus, 
                                        nsAString & aSuggestedDriverVersion, 
                                        const nsTArray<GfxDriverInfo>& aDriverInfo, 
                                        OperatingSystem* aOS = nullptr);
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo();

private:

  void AddCrashReportAnnotations();
  nsString mDeviceString;
  nsString mDeviceID;
  nsString mDriverVersion;
  nsString mDriverDate;
  nsString mDeviceKey;
  nsString mDeviceKeyDebug;
  nsString mAdapterVendorID;
  nsString mAdapterDeviceID;
  uint32_t mAdapterSubsysID;
  nsString mDeviceString2;
  nsString mDriverVersion2;
  nsString mDeviceID2;
  nsString mDriverDate2;
  nsString mDeviceKey2;
  nsString mAdapterVendorID2;
  nsString mAdapterDeviceID2;
  uint32_t mAdapterSubsysID2;
  uint32_t mWindowsVersion;
  bool mHasDualGPU;
  bool mIsGPU2Active;
  bool mHasDriverVersionMismatch;
};

} 
} 

#endif 
