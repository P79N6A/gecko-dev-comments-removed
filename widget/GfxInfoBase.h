






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

  
  
  
  
  
  
  
  NS_IMETHOD GetFeatureStatus(int32_t aFeature, int32_t *_retval) MOZ_OVERRIDE;
  NS_IMETHOD GetFeatureSuggestedDriverVersion(int32_t aFeature, nsAString & _retval) MOZ_OVERRIDE;
  NS_IMETHOD GetWebGLParameter(const nsAString & aParam, nsAString & _retval) MOZ_OVERRIDE;

    NS_IMETHOD GetFailures(uint32_t *failureCount, int32_t** indices, char ***failures) MOZ_OVERRIDE;
  NS_IMETHOD_(void) LogFailure(const nsACString &failure) MOZ_OVERRIDE;
  NS_IMETHOD GetInfo(JSContext*, JS::MutableHandle<JS::Value>) MOZ_OVERRIDE;

  
  
  
  
  
  
  
  virtual nsresult Init();
  
  
  NS_IMETHOD_(void) GetData() MOZ_OVERRIDE { }

  static void AddCollector(GfxInfoCollectorBase* collector);
  static void RemoveCollector(GfxInfoCollectorBase* collector);

  static nsTArray<GfxDriverInfo>* mDriverInfo;
  static bool mDriverInfoObserverInitialized;

  virtual nsString Model() { return EmptyString(); }
  virtual nsString Hardware() { return EmptyString(); }
  virtual nsString Product() { return EmptyString(); }
  virtual nsString Manufacturer() { return EmptyString(); }
  virtual uint32_t OperatingSystemVersion() { return 0; }

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
