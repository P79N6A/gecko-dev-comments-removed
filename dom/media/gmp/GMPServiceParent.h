




#ifndef GMPServiceParent_h_
#define GMPServiceParent_h_

#include "GMPService.h"
#include "mozilla/gmp/PGMPServiceParent.h"
#include "mozIGeckoMediaPluginChromeService.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "mozilla/Atomics.h"
#include "nsThreadUtils.h"

template <class> struct already_AddRefed;

namespace mozilla {
namespace gmp {

class GMPParent;

#define GMP_DEFAULT_ASYNC_SHUTDONW_TIMEOUT 3000

class GeckoMediaPluginServiceParent final : public GeckoMediaPluginService
                                          , public mozIGeckoMediaPluginChromeService
{
public:
  static already_AddRefed<GeckoMediaPluginServiceParent> GetSingleton();

  GeckoMediaPluginServiceParent();
  virtual nsresult Init() override;

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetPluginVersionForAPI(const nsACString& aAPI,
                                    nsTArray<nsCString>* aTags,
                                    bool* aHasPlugin,
                                    nsACString& aOutVersion) override;
  NS_IMETHOD GetNodeId(const nsAString& aOrigin,
                       const nsAString& aTopLevelOrigin,
                       bool aInPrivateBrowsingMode,
                       UniquePtr<GetNodeIdCallback>&& aCallback) override;

  NS_DECL_MOZIGECKOMEDIAPLUGINCHROMESERVICE
  NS_DECL_NSIOBSERVER

  void AsyncShutdownNeeded(GMPParent* aParent);
  void AsyncShutdownComplete(GMPParent* aParent);
  void AbortAsyncShutdown();

  int32_t AsyncShutdownTimeoutMs();

private:
  friend class GMPServiceParent;

  virtual ~GeckoMediaPluginServiceParent();

  void ClearStorage();

  GMPParent* SelectPluginForAPI(const nsACString& aNodeId,
                                const nsCString& aAPI,
                                const nsTArray<nsCString>& aTags);
  GMPParent* FindPluginForAPIFrom(size_t aSearchStartIndex,
                                  const nsCString& aAPI,
                                  const nsTArray<nsCString>& aTags,
                                  size_t* aOutPluginIndex);

  nsresult GetNodeId(const nsAString& aOrigin, const nsAString& aTopLevelOrigin,
                     bool aInPrivateBrowsing, nsACString& aOutId);

  void UnloadPlugins();
  void CrashPlugins();
  void SetAsyncShutdownComplete();

  void LoadFromEnvironment();
  void ProcessPossiblePlugin(nsIFile* aDir);

  void AddOnGMPThread(const nsAString& aDirectory);
  void RemoveOnGMPThread(const nsAString& aDirectory,
                         const bool aDeleteFromDisk);

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
  virtual void InitializePlugins() override;
  virtual bool GetContentParentFrom(const nsACString& aNodeId,
                                    const nsCString& aAPI,
                                    const nsTArray<nsCString>& aTags,
                                    UniquePtr<GetGMPContentParentCallback>&& aCallback)
    override;
private:
  GMPParent* ClonePlugin(const GMPParent* aOriginal);
  nsresult EnsurePluginsOnDiskScanned();
  nsresult InitStorage();

  class PathRunnable : public nsRunnable
  {
  public:
    enum EOperation {
      ADD,
      REMOVE,
      REMOVE_AND_DELETE_FROM_DISK,
    };

    PathRunnable(GeckoMediaPluginServiceParent* aService, const nsAString& aPath,
                 EOperation aOperation)
      : mService(aService)
      , mPath(aPath)
      , mOperation(aOperation)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsRefPtr<GeckoMediaPluginServiceParent> mService;
    nsString mPath;
    EOperation mOperation;
  };

  
  nsTArray<nsRefPtr<GMPParent>> mPlugins;
  bool mShuttingDown;

  
  
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

  nsCOMPtr<nsIFile> mStorageBaseDir;

  
  
  nsClassHashtable<nsUint32HashKey, nsCString> mTempNodeIds;

  
  
  nsDataHashtable<nsCStringHashKey, bool> mPersistentStorageAllowed;
};

nsresult ReadSalt(nsIFile* aPath, nsACString& aOutData);
bool MatchOrigin(nsIFile* aPath, const nsACString& aSite);

class GMPServiceParent final : public PGMPServiceParent
{
public:
  explicit GMPServiceParent(GeckoMediaPluginServiceParent* aService)
    : mService(aService)
  {
  }
  virtual ~GMPServiceParent();

  virtual bool RecvLoadGMP(const nsCString& aNodeId,
                           const nsCString& aApi,
                           nsTArray<nsCString>&& aTags,
                           nsTArray<ProcessId>&& aAlreadyBridgedTo,
                           base::ProcessId* aID,
                           nsCString* aDisplayName,
                           nsCString* aPluginId) override;
  virtual bool RecvGetGMPNodeId(const nsString& aOrigin,
                                const nsString& aTopLevelOrigin,
                                const bool& aInPrivateBrowsing,
                                nsCString* aID) override;
  static bool RecvGetGMPPluginVersionForAPI(const nsCString& aAPI,
                                            nsTArray<nsCString>&& aTags,
                                            bool* aHasPlugin,
                                            nsCString* aVersion);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  static PGMPServiceParent* Create(Transport* aTransport, ProcessId aOtherPid);

private:
  nsRefPtr<GeckoMediaPluginServiceParent> mService;
};

} 
} 

#endif 
