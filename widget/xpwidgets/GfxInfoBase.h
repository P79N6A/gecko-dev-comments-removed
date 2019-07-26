






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
#include "nsIGfxInfoDebug.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace widget {  

class GfxInfoBase : public nsIGfxInfo,
                    public nsIObserver,
                    public nsSupportsWeakReference
#ifdef DEBUG
                  , public nsIGfxInfoDebug
#endif
{
public:
  GfxInfoBase();
  virtual ~GfxInfoBase();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  
  
  
  
  
  NS_IMETHOD GetFeatureStatus(int32_t aFeature, int32_t *_retval);
  NS_IMETHOD GetFeatureSuggestedDriverVersion(int32_t aFeature, nsAString & _retval);
  NS_IMETHOD GetWebGLParameter(const nsAString & aParam, nsAString & _retval);

  NS_IMETHOD GetFailures(uint32_t *failureCount, char ***failures);
  NS_IMETHOD_(void) LogFailure(const nsACString &failure);
  NS_IMETHOD GetInfo(JSContext*, jsval*);

  
  
  
  
  
  
  
  virtual nsresult Init();
  
  
  NS_IMETHOD_(void) GetData() { }

  static void AddCollector(GfxInfoCollectorBase* collector);
  static void RemoveCollector(GfxInfoCollectorBase* collector);

  static nsTArray<GfxDriverInfo>* mDriverInfo;
  static bool mDriverInfoObserverInitialized;

  virtual nsString Model() const { return EmptyString(); }
  virtual nsString Hardware() const { return EmptyString(); }
  virtual nsString Product() const { return EmptyString(); }
  virtual nsString Manufacturer() const { return EmptyString(); }
  virtual uint32_t OperatingSystemVersion() const { return 0; }

protected:

  virtual nsresult GetFeatureStatusImpl(int32_t aFeature, int32_t* aStatus,
                                        nsAString& aSuggestedDriverVersion,
                                        const nsTArray<GfxDriverInfo>& aDriverInfo,
                                        OperatingSystem* aOS = nullptr);

  
  
  virtual const nsTArray<GfxDriverInfo>& GetGfxDriverInfo() = 0;

private:
  virtual int32_t FindBlocklistedDeviceInList(const nsTArray<GfxDriverInfo>& aDriverInfo,
                                              nsAString& aSuggestedVersion,
                                              int32_t aFeature,
                                              OperatingSystem os);

  void EvaluateDownloadedBlacklist(nsTArray<GfxDriverInfo>& aDriverInfo);

  nsCString mFailures[9]; 
  uint32_t mFailureCount;
  Mutex mMutex;

};

}
}

#endif 
