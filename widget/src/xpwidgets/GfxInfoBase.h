






































#ifndef __mozilla_widget_GfxInfoBase_h__
#define __mozilla_widget_GfxInfoBase_h__

#include "nsIGfxInfo.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "GfxDriverInfo.h"
#include "nsTArray.h"

namespace mozilla {
namespace widget {  

class GfxInfoBase : public nsIGfxInfo,
                    public nsIObserver,
                    public nsSupportsWeakReference
{
public:
  GfxInfoBase();
  virtual ~GfxInfoBase();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  
  
  
  
  
  NS_SCRIPTABLE NS_IMETHOD GetFeatureStatus(PRInt32 aFeature, PRInt32 *_retval NS_OUTPARAM);
  NS_SCRIPTABLE NS_IMETHOD GetFeatureSuggestedDriverVersion(PRInt32 aFeature, nsAString & _retval NS_OUTPARAM);
  NS_SCRIPTABLE NS_IMETHOD GetWebGLParameter(const nsAString & aParam, nsAString & _retval NS_OUTPARAM);

  
  
  
  
  virtual nsresult Init();

protected:

  virtual nsresult GetFeatureStatusImpl(PRInt32 aFeature, PRInt32* aStatus,
                                        nsAString& aSuggestedDriverVersion,
                                        GfxDriverInfo* aDriverInfo = nsnull) = 0;

private:

  void EvaluateDownloadedBlacklist(nsTArray<GfxDriverInfo>& aDriverInfo);

};

};
};

#endif 
