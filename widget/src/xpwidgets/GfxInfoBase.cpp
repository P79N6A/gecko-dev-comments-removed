






































#include "GfxInfoBase.h"
#include "GfxInfoWebGL.h"

using namespace mozilla::widget;

NS_IMPL_ISUPPORTS1(GfxInfoBase, nsIGfxInfo)

NS_IMETHODIMP
GfxInfoBase::GetFeatureStatus(PRInt32 aFeature, PRInt32* aStatus NS_OUTPARAM)
{
  nsString version;
  return GetFeatureStatusImpl(aFeature, aStatus, version);
}

NS_IMETHODIMP
GfxInfoBase::GetFeatureSuggestedDriverVersion(PRInt32 aFeature,
                                              nsAString& aVersion NS_OUTPARAM)
{
  PRInt32 status;
  return GetFeatureStatusImpl(aFeature, &status, aVersion);
}

NS_IMETHODIMP
GfxInfoBase::GetWebGLParameter(const nsAString& aParam,
                               nsAString& aResult NS_OUTPARAM)
{
  return GfxInfoWebGL::GetWebGLParameter(aParam, aResult);
}
