




#ifndef nsDOMStorageIPC_h___
#define nsDOMStorageIPC_h___

#include "mozilla/dom/PStorageChild.h"
#include "mozilla/dom/PStorageParent.h"
#include "DOMStorageDBThread.h"
#include "DOMStorageCache.h"
#include "DOMStorageObserver.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace dom {

class DOMLocalStorageManager;





class DOMStorageDBChild MOZ_FINAL : public DOMStorageDBBridge
                                  , public PStorageChild
{
  virtual ~DOMStorageDBChild();

public:
  DOMStorageDBChild(DOMLocalStorageManager* aManager);

  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(MozExternalRefCountType) Release(void);

  void AddIPDLReference();
  void ReleaseIPDLReference();

  virtual nsresult Init();
  virtual nsresult Shutdown();

  virtual void AsyncPreload(DOMStorageCacheBridge* aCache, bool aPriority = false);
  virtual void AsyncGetUsage(DOMStorageUsageBridge* aUsage);

  virtual void SyncPreload(DOMStorageCacheBridge* aCache, bool aForceSync = false);

  virtual nsresult AsyncAddItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue);
  virtual nsresult AsyncUpdateItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue);
  virtual nsresult AsyncRemoveItem(DOMStorageCacheBridge* aCache, const nsAString& aKey);
  virtual nsresult AsyncClear(DOMStorageCacheBridge* aCache);

  virtual void AsyncClearAll()
  {
    if (mScopesHavingData) {
      mScopesHavingData->Clear(); 
    }
  }

  virtual void AsyncClearMatchingScope(const nsACString& aScope)
    {  }

  virtual void AsyncFlush()
    { SendAsyncFlush(); }

  virtual bool ShouldPreloadScope(const nsACString& aScope);
  virtual void GetScopesHavingData(InfallibleTArray<nsCString>* aScopes)
    { NS_NOTREACHED("Not implemented for child process"); }

private:
  bool RecvObserve(const nsCString& aTopic,
                   const nsCString& aScopePrefix);
  bool RecvLoadItem(const nsCString& aScope,
                    const nsString& aKey,
                    const nsString& aValue);
  bool RecvLoadDone(const nsCString& aScope,
                    const nsresult& aRv);
  bool RecvScopesHavingData(const InfallibleTArray<nsCString>& aScopes);
  bool RecvLoadUsage(const nsCString& aScope,
                     const int64_t& aUsage);
  bool RecvError(const nsresult& aRv);

  nsTHashtable<nsCStringHashKey>& ScopesHavingData();

  ThreadSafeAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  
  nsRefPtr<DOMLocalStorageManager> mManager;

  
  nsAutoPtr<nsTHashtable<nsCStringHashKey> > mScopesHavingData;

  
  
  nsTHashtable<nsRefPtrHashKey<DOMStorageCacheBridge> > mLoadingCaches;

  
  nsresult mStatus;

  bool mIPCOpen;
};







class DOMStorageDBParent MOZ_FINAL : public PStorageParent
                                   , public DOMStorageObserverSink
{
  virtual ~DOMStorageDBParent();

public:
  DOMStorageDBParent();

  virtual mozilla::ipc::IProtocol*
  CloneProtocol(Channel* aChannel,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(MozExternalRefCountType) Release(void);

  void AddIPDLReference();
  void ReleaseIPDLReference();

  bool IPCOpen() { return mIPCOpen; }

public:
  
  
  class CacheParentBridge : public DOMStorageCacheBridge {
  public:
    CacheParentBridge(DOMStorageDBParent* aParentDB, const nsACString& aScope)
      : mParent(aParentDB), mScope(aScope), mLoaded(false), mLoadedCount(0) {}
    virtual ~CacheParentBridge() {}

    
    virtual const nsCString& Scope() const
      { return mScope; }
    virtual bool Loaded()
      { return mLoaded; }
    virtual uint32_t LoadedCount()
      { return mLoadedCount; }

    virtual bool LoadItem(const nsAString& aKey, const nsString& aValue);
    virtual void LoadDone(nsresult aRv);
    virtual void LoadWait();

  private:
    nsRefPtr<DOMStorageDBParent> mParent;
    nsCString mScope;
    bool mLoaded;
    uint32_t mLoadedCount;
  };

  
  class UsageParentBridge : public DOMStorageUsageBridge
  {
  public:
    UsageParentBridge(DOMStorageDBParent* aParentDB, const nsACString& aScope)
      : mParent(aParentDB), mScope(aScope) {}
    virtual ~UsageParentBridge() {}

    
    virtual const nsCString& Scope() { return mScope; }
    virtual void LoadUsage(const int64_t usage);

  private:
    nsRefPtr<DOMStorageDBParent> mParent;
    nsCString mScope;
  };

private:
  
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  bool RecvAsyncPreload(const nsCString& aScope, const bool& aPriority);
  bool RecvPreload(const nsCString& aScope, const uint32_t& aAlreadyLoadedCount,
                   InfallibleTArray<nsString>* aKeys, InfallibleTArray<nsString>* aValues,
                   nsresult* aRv);
  bool RecvAsyncGetUsage(const nsCString& aScope);
  bool RecvAsyncAddItem(const nsCString& aScope, const nsString& aKey, const nsString& aValue);
  bool RecvAsyncUpdateItem(const nsCString& aScope, const nsString& aKey, const nsString& aValue);
  bool RecvAsyncRemoveItem(const nsCString& aScope, const nsString& aKey);
  bool RecvAsyncClear(const nsCString& aScope);
  bool RecvAsyncFlush();

  
  virtual nsresult Observe(const char* aTopic, const nsACString& aScopePrefix);

private:
  CacheParentBridge* NewCache(const nsACString& aScope);

  ThreadSafeAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
	
	
  bool mIPCOpen;
};

} 
} 

#endif
