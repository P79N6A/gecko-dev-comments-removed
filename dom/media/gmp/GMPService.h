




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
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "mozilla/Atomics.h"

template <class> struct already_AddRefed;

namespace mozilla {
namespace gmp {

class GMPParent;

#define GMP_DEFAULT_ASYNC_SHUTDONW_TIMEOUT 3000

class GeckoMediaPluginService MOZ_FINAL : public mozIGeckoMediaPluginService
                                        , public nsIObserver
{
public:
  static already_AddRefed<GeckoMediaPluginService> GetGeckoMediaPluginService();

  GeckoMediaPluginService();
  nsresult Init();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZIGECKOMEDIAPLUGINSERVICE
  NS_DECL_NSIOBSERVER

  void AsyncShutdownNeeded(GMPParent* aParent);
  void AsyncShutdownComplete(GMPParent* aParent);
  void AbortAsyncShutdown();

  int32_t AsyncShutdownTimeoutMs();

private:
  ~GeckoMediaPluginService();

  nsresult GMPDispatch(nsIRunnable* event, uint32_t flags = NS_DISPATCH_NORMAL);

  void ClearStorage();

  GMPParent* SelectPluginForAPI(const nsACString& aNodeId,
                                const nsCString& aAPI,
                                const nsTArray<nsCString>& aTags);
  GMPParent* FindPluginForAPIFrom(size_t aSearchStartIndex,
                                  const nsCString& aAPI,
                                  const nsTArray<nsCString>& aTags,
                                  size_t* aOutPluginIndex);

  void UnloadPlugins();
  void CrashPlugins();
  void SetAsyncShutdownComplete();

  void LoadFromEnvironment();
  void ProcessPossiblePlugin(nsIFile* aDir);

  void AddOnGMPThread(const nsAString& aSearchDir);
  void RemoveOnGMPThread(const nsAString& aSearchDir);

  nsresult SetAsyncShutdownTimeout();

  struct DirectoryFilter {
    virtual bool operator()(nsIFile* aPath) = 0;
    ~DirectoryFilter() {}
  };
  void ClearNodeIdAndPlugin(DirectoryFilter& aFilter);

  void ForgetThisSiteOnGMPThread(const nsACString& aOrigin);
  void ClearRecentHistoryOnGMPThread(PRTime aSince);

protected:
  friend class GMPParent;
  void ReAddOnGMPThread(nsRefPtr<GMPParent>& aOld);
private:
  GMPParent* ClonePlugin(const GMPParent* aOriginal);
  nsresult EnsurePluginsOnDiskScanned();

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

  
  
  Atomic<bool> mScannedPluginOnDisk;

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

#ifndef MOZ_WIDGET_GONK
  nsCOMPtr<nsIFile> mStorageBaseDir;
#endif

  
  
  nsClassHashtable<nsUint32HashKey, nsCString> mTempNodeIds;

  
  
  nsDataHashtable<nsCStringHashKey, bool> mPersistentStorageAllowed;
};

nsresult ReadSalt(nsIFile* aPath, nsACString& aOutData);
bool MatchOrigin(nsIFile* aPath, const nsACString& aOrigin);

} 
} 

#endif 
