






#ifndef __GfxInfoX11_h__
#define __GfxInfoX11_h__

#include "GfxInfoBase.h"

namespace mozilla {
namespace widget {

class GfxInfo final : public GfxInfoBase
{
public:

  
  
  NS_IMETHOD GetD2DEnabled(bool *aD2DEnabled) override;
  NS_IMETHOD GetDWriteEnabled(bool *aDWriteEnabled) override;
  NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion) override;
  NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams) override;
  NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription) override;
  NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver) override;
  NS_IMETHOD GetAdapterVendorID(nsAString & aAdapterVendorID) override;
  NS_IMETHOD GetAdapterDeviceID(nsAString & aAdapterDeviceID) override;
  NS_IMETHOD GetAdapterSubsysID(nsAString & aAdapterSubsysID) override;
  NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM) override;
  NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion) override;
  NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate) override;
  NS_IMETHOD GetAdapterDescription2(nsAString & aAdapterDescription) override;
  NS_IMETHOD GetAdapterDriver2(nsAString & aAdapterDriver) override;
  NS_IMETHOD GetAdapterVendorID2(nsAString & aAdapterVendorID) override;
  NS_IMETHOD GetAdapterDeviceID2(nsAString & aAdapterDeviceID) override;
  NS_IMETHOD GetAdapterSubsysID2(nsAString & aAdapterSubsysID) override;
  NS_IMETHOD GetAdapterRAM2(nsAString & aAdapterRAM) override;
  NS_IMETHOD GetAdapterDriverVersion2(nsAString & aAdapterDriverVersion) override;
  NS_IMETHOD GetAdapterDriverDate2(nsAString & aAdapterDriverDate) override;
  NS_IMETHOD GetIsGPU2Active(bool *aIsGPU2Active) override;
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init() override;
  
  NS_IMETHOD_(void) GetData() override;

  nsresult FindMonitors(JSContext* cx, JS::HandleObject array) override;

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
                                        OperatingSystem* aOS = nullptr) override;
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo() override;

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
