






#ifndef __GfxInfoX11_h__
#define __GfxInfoX11_h__

#include "GfxInfoBase.h"

namespace mozilla {
namespace widget {

class GfxInfo MOZ_FINAL : public GfxInfoBase
{
public:

  
  
  NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled);
  NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled);
  NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion);
  NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams);
  NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription);
  NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver);
  NS_IMETHOD GetAdapterVendorID(nsAString & aAdapterVendorID);
  NS_IMETHOD GetAdapterDeviceID(nsAString & aAdapterDeviceID);
  NS_IMETHOD GetAdapterSubsysID(nsAString & aAdapterSubsysID);
  NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM);
  NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion);
  NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate);
  NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription);
  NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver);
  NS_IMETHOD GetAdapterVendorID2(nsAString & aAdapterVendorID);
  NS_IMETHOD GetAdapterDeviceID2(nsAString & aAdapterDeviceID);
  NS_IMETHOD GetAdapterSubsysID2(nsAString & aAdapterSubsysID);
  NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM);
  NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion);
  NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate);
  NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active);
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init();
  
  NS_IMETHOD_(void) GetData();

#ifdef DEBUG
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIGFXINFODEBUG
#endif

protected:
  ~GfxInfo() {}

  virtual nsresult GetFeatureStatusImpl(int32_t aFeature, 
                                        int32_t *aStatus, 
                                        nsAString & aSuggestedDriverVersion, 
                                        const nsTArray<GfxDriverInfo>& aDriverInfo, 
                                        OperatingSystem* aOS = nullptr);
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo();

private:
  nsCString mVendor;
  nsCString mRenderer;
  nsCString mVersion;
  nsCString mAdapterDescription;
  nsCString mOS;
  nsCString mOSRelease;
  bool mIsMesa, mIsNVIDIA, mIsFGLRX, mIsNouveau, mIsIntel, mIsOldSwrast, mIsLlvmpipe;
  bool mHasTextureFromPixmap;
  int mGLMajorVersion, mMajorVersion, mMinorVersion, mRevisionVersion;

  void AddCrashReportAnnotations();
};

} 
} 

#endif 
