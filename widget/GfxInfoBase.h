






#ifndef __mozilla_widget_GfxInfoBase_h__
#define __mozilla_widget_GfxInfoBase_h__

#include "nsIGfxInfo.h"
#if defined(XP_MACOSX) || defined(XP_WIN)
#include "nsIGfxInfo2.h"
#endif
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "GfxDriverInfo.h"
#include "nsTArray.h"
#include "nsString.h"
#include "GfxInfoCollector.h"
#include "nsIGfxInfoDebug.h"
#include "mozilla/Mutex.h"
#include "js/Value.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace widget {  

class GfxInfoBase : public nsIGfxInfo,
#if defined(XP_MACOSX) || defined(XP_WIN)
                    public nsIGfxInfo2,
#endif
                    public nsIObserver,
                    public nsSupportsWeakReference
#ifdef DEBUG
                  , public nsIGfxInfoDebug
#endif
{
public:
  GfxInfoBase();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  
  
  
  
  
  NS_IMETHOD GetFeatureStatus(int32_t aFeature, int32_t *_retval) override;
  NS_IMETHOD GetFeatureSuggestedDriverVersion(int32_t aFeature, nsAString & _retval) override;
  NS_IMETHOD GetWebGLParameter(const nsAString & aParam, nsAString & _retval) override;

  NS_IMETHOD GetMonitors(JSContext* cx, JS::MutableHandleValue _retval) override;
  NS_IMETHOD GetFailures(uint32_t *failureCount, int32_t** indices, char ***failures) override;
  NS_IMETHOD_(void) LogFailure(const nsACString &failure) override;
  NS_IMETHOD GetInfo(JSContext*, JS::MutableHandle<JS::Value>) override;

  
  
  
  
  
  
  
  virtual nsresult Init();
  
  
  NS_IMETHOD_(void) GetData() override { }

  static void AddCollector(GfxInfoCollectorBase* collector);
  static void RemoveCollector(GfxInfoCollectorBase* collector);

  static nsTArray<GfxDriverInfo>* mDriverInfo;
  static bool mDriverInfoObserverInitialized;

  virtual nsString Model() { return EmptyString(); }
  virtual nsString Hardware() { return EmptyString(); }
  virtual nsString Product() { return EmptyString(); }
  virtual nsString Manufacturer() { return EmptyString(); }
  virtual uint32_t OperatingSystemVersion() { return 0; }

  
  static const nsCString& GetApplicationVersion();

  virtual nsresult FindMonitors(JSContext* cx, JS::HandleObject array) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

protected:

  virtual ~GfxInfoBase();

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

  Mutex mMutex;

};

}
}

#endif 
