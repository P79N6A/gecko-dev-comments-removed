




#ifndef GMPService_h_
#define GMPService_h_

#include "nsString.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "nsITimer.h"

template <class> struct already_AddRefed;

namespace mozilla {
namespace gmp {

class GMPParent;

class GeckoMediaPluginService MOZ_FINAL : public mozIGeckoMediaPluginService
                                        , public nsIObserver
{
public:
  static already_AddRefed<GeckoMediaPluginService> GetGeckoMediaPluginService();

  GeckoMediaPluginService();
  void Init();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZIGECKOMEDIAPLUGINSERVICE
  NS_DECL_NSIOBSERVER

  void AsyncShutdownNeeded(GMPParent* aParent);
  void AsyncShutdownComplete(GMPParent* aParent);
  void AbortAsyncShutdown();

private:
  ~GeckoMediaPluginService();

  GMPParent* SelectPluginForAPI(const nsAString& aOrigin,
                                const nsCString& aAPI,
                                const nsTArray<nsCString>& aTags,
                                bool aCloneCrossOrigin = true);

  void UnloadPlugins();
  void CrashPlugins();
  void SetAsyncShutdownComplete();

  void LoadFromEnvironment();
  void ProcessPossiblePlugin(nsIFile* aDir);

  void AddOnGMPThread(const nsAString& aSearchDir);
  void RemoveOnGMPThread(const nsAString& aSearchDir);

  nsresult SetAsyncShutdownTimeout();

protected:
  friend class GMPParent;
  void ReAddOnGMPThread(nsRefPtr<GMPParent>& aOld);
private:
  GMPParent* ClonePlugin(const GMPParent* aOriginal);

  class PathRunnable : public nsRunnable
  {
  public:
    PathRunnable(GeckoMediaPluginService* service, const nsAString& path,
                 bool add)
      : mService(service)
      , mPath(path)
      , mAdd(add)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsRefPtr<GeckoMediaPluginService> mService;
    nsString mPath;
    bool mAdd;
  };

  Mutex mMutex; 
  nsTArray<nsRefPtr<GMPParent>> mPlugins;
  nsCOMPtr<nsIThread> mGMPThread;
  bool mShuttingDown;
  bool mShuttingDownOnGMPThread;

  template<typename T>
  class MainThreadOnly {
  public:
    MOZ_IMPLICIT MainThreadOnly(T aValue)
      : mValue(aValue)
    {}
    operator T&() {
      MOZ_ASSERT(NS_IsMainThread());
      return mValue;
    }

  private:
    T mValue;
  };

  MainThreadOnly<bool> mWaitingForPluginsAsyncShutdown;

  nsTArray<nsRefPtr<GMPParent>> mAsyncShutdownPlugins; 
  nsCOMPtr<nsITimer> mAsyncShutdownTimeout; 
};

} 
} 

#endif 
