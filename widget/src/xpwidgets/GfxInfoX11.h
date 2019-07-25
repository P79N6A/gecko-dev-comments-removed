






































#ifndef __GfxInfoX11_h__
#define __GfxInfoX11_h__

#include "GfxInfoBase.h"

namespace mozilla {
namespace widget {

class GfxInfo : public GfxInfoBase
{
public:

  
  
  NS_SCRIPTABLE NS_IMETHOD GetD2DEnabled(PRBool *aD2DEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteEnabled(PRBool *aDWriteEnabled);
  NS_SCRIPTABLE NS_IMETHOD GetDWriteVersion(nsAString & aDwriteVersion);
  NS_SCRIPTABLE NS_IMETHOD GetCleartypeParameters(nsAString & aCleartypeParams);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDescription(nsAString & aAdapterDescription);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriver(nsAString & aAdapterDriver);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterVendorID(PRUint32 *aAdapterVendorID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDeviceID(PRUint32 *aAdapterDeviceID);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterRAM(nsAString & aAdapterRAM);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverVersion(nsAString & aAdapterDriverVersion);
  NS_SCRIPTABLE NS_IMETHOD GetAdapterDriverDate(nsAString & aAdapterDriverDate);
  using GfxInfoBase::GetFeatureStatus;
  using GfxInfoBase::GetFeatureSuggestedDriverVersion;
  using GfxInfoBase::GetWebGLParameter;

  virtual nsresult Init();

protected:

  virtual nsresult GetFeatureStatusImpl(PRInt32 aFeature, PRInt32 *aStatus, nsAString & aSuggestedDriverVersion, GfxDriverInfo* aDriverInfo = nsnull);

private:
  nsCString mVendor;
  nsCString mRenderer;
  nsCString mVersion;
  nsCString mAdapterDescription;
  bool mIsMesa, mIsNVIDIA, mIsFGLRX;
  int mMajorVersion, mMinorVersion;

  void AddCrashReportAnnotations();
  void GetData();
};

} 
} 

#endif 
