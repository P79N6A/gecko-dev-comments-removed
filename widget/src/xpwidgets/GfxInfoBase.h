






































#ifndef __mozilla_widget_GfxInfoBase_h__
#define __mozilla_widget_GfxInfoBase_h__

#include "nsIGfxInfo.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "GfxDriverInfo.h"
#include "nsTArray.h"
#include "nsString.h"
#include "GfxInfoCollector.h"

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

  NS_SCRIPTABLE NS_IMETHOD GetFailures(PRUint32 *failureCount NS_OUTPARAM, char ***failures NS_OUTPARAM);
  NS_IMETHOD_(void) LogFailure(const nsACString &failure);
  NS_SCRIPTABLE NS_IMETHOD GetInfo(JSContext*, jsval*);

  
  
  
  
  
  
  
  virtual nsresult Init();

  
  NS_IMETHOD_(void) GetData() { }

  static nsTArray<GfxDriverInfo>* mDriverInfo;
  static bool mDriverInfoObserverInitialized;

  static void AddCollector(GfxInfoCollectorBase* collector);
  static void RemoveCollector(GfxInfoCollectorBase* collector);


protected:

  virtual nsresult GetFeatureStatusImpl(PRInt32 aFeature, PRInt32* aStatus,
                                        nsAString& aSuggestedDriverVersion,
                                        const nsTArray<GfxDriverInfo>& aDriverInfo,
                                        OperatingSystem* aOS = nsnull);

  
  
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo() = 0;

private:

  virtual PRInt32 FindBlocklistedDeviceInList(const nsTArray<GfxDriverInfo>& aDriverInfo,
                                              nsAString& aSuggestedVersion,
                                              PRInt32 aFeature,
                                              OperatingSystem os);

  void EvaluateDownloadedBlacklist(nsTArray<GfxDriverInfo>& aDriverInfo);

  nsCString mFailures[9]; 
  PRUint32 mFailureCount;

};

}
}

#endif 
