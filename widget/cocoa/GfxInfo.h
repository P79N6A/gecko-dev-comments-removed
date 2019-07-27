






#ifndef __mozilla_widget_GfxInfo_h__
#define __mozilla_widget_GfxInfo_h__

#include "GfxInfoBase.h"
#include "nsIGfxInfo2.h"

#include "nsString.h"

namespace mozilla {
namespace widget {

class GfxInfo : public GfxInfoBase
{
public:

  GfxInfo();
  
  
  NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled) MOZ_OVERRIDE;
  NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled) MOZ_OVERRIDE;
  NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion) MOZ_OVERRIDE;
  NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterVendorID(nsAString & aAdapterVendorID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDeviceID(nsAString & aAdapterDeviceID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterSubsysID(nsAString & aAdapterSubsysID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterVendorID2(nsAString & aAdapterVendorID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDeviceID2(nsAString & aAdapterDeviceID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterSubsysID2(nsAString & aAdapterSubsysID) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion) MOZ_OVERRIDE;
  NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate) MOZ_OVERRIDE;
  NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active) MOZ_OVERRIDE;

  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init() MOZ_OVERRIDE;

  NS_DECL_ISUPPORTS_INHERITED
#ifdef DEBUG
  NS_DECL_NSIGFXINFODEBUG
#endif
  NS_DECL_NSIGFXINFO2

  virtual uint32_t OperatingSystemVersion() MOZ_OVERRIDE { return mOSXVersion; }

protected:

  virtual ~GfxInfo() {}

  virtual nsresult GetFeatureStatusImpl(int32_t aFeature, 
                                        int32_t *aStatus, 
                                        nsAString & aSuggestedDriverVersion, 
                                        const nsTArray<GfxDriverInfo>& aDriverInfo,
                                        OperatingSystem* aOS = nullptr) MOZ_OVERRIDE;
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo() MOZ_OVERRIDE;

private:

  void GetDeviceInfo();
  void GetSelectedCityInfo();
  void AddCrashReportAnnotations();

  nsString mAdapterRAMString;
  nsString mDeviceID;
  nsString mDriverVersion;
  nsString mDriverDate;
  nsString mDeviceKey;

  nsString mAdapterVendorID;
  nsString mAdapterDeviceID;

  nsString mCountryCode;

  uint32_t mOSXVersion;
};

} 
} 

#endif 
