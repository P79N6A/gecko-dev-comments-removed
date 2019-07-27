




#ifndef nsDOMStorageCache_h___
#define nsDOMStorageCache_h___

#include "nsIPrincipal.h"
#include "nsITimer.h"

#include "nsString.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/Monitor.h"
#include "mozilla/Telemetry.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class DOMStorage;
class DOMStorageUsage;
class DOMStorageManager;
class DOMStorageDBBridge;



class DOMStorageCacheBridge
{
public:
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(void) Release(void);

  
  virtual const nsCString& Scope() const = 0;

  
  virtual bool Loaded() = 0;

  
  
  virtual uint32_t LoadedCount() = 0;

  
  virtual bool LoadItem(const nsAString& aKey, const nsString& aValue) = 0;

  
  
  virtual void LoadDone(nsresult aRv) = 0;

  
  
  virtual void LoadWait() = 0;

protected:
  virtual ~DOMStorageCacheBridge() {}

  ThreadSafeAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};





class DOMStorageCache : public DOMStorageCacheBridge
{
public:
  NS_IMETHOD_(void) Release(void);

  explicit DOMStorageCache(const nsACString* aScope);

protected:
  virtual ~DOMStorageCache();

public:
  void Init(DOMStorageManager* aManager, bool aPersistent, nsIPrincipal* aPrincipal,
            const nsACString& aQuotaScope);

  
  void CloneFrom(const DOMStorageCache* aThat);

  
  void Preload();

  
  void KeepAlive();

  
  
  
  
  nsresult GetLength(const DOMStorage* aStorage, uint32_t* aRetval);
  nsresult GetKey(const DOMStorage* aStorage, uint32_t index, nsAString& aRetval);
  nsresult GetItem(const DOMStorage* aStorage, const nsAString& aKey, nsAString& aRetval);
  nsresult SetItem(const DOMStorage* aStorage, const nsAString& aKey, const nsString& aValue, nsString& aOld);
  nsresult RemoveItem(const DOMStorage* aStorage, const nsAString& aKey, nsString& aOld);
  nsresult Clear(const DOMStorage* aStorage);

  void GetKeys(const DOMStorage* aStorage, nsTArray<nsString>& aKeys);

  
  bool CheckPrincipal(nsIPrincipal* aPrincipal) const;
  nsIPrincipal* Principal() const { return mPrincipal; }

  
  static DOMStorageDBBridge* StartDatabase();
  static DOMStorageDBBridge* GetDatabase();

  
  static nsresult StopDatabase();

  

  virtual const nsCString& Scope() const { return mScope; }
  virtual bool Loaded() { return mLoaded; }
  virtual uint32_t LoadedCount();
  virtual bool LoadItem(const nsAString& aKey, const nsString& aValue);
  virtual void LoadDone(nsresult aRv);
  virtual void LoadWait();

  
  
  
  class Data
  {
  public:
    Data() : mOriginQuotaUsage(0) {}
    int64_t mOriginQuotaUsage;
    nsDataHashtable<nsStringHashKey, nsString> mKeys;
  };

public:
  
  static const uint32_t kDataSetCount = 3;

private:
  
  
  friend class DOMStorageManager;

  static const uint32_t kUnloadDefault = 1 << 0;
  static const uint32_t kUnloadPrivate = 1 << 1;
  static const uint32_t kUnloadSession = 1 << 2;
  static const uint32_t kUnloadComplete =
    kUnloadDefault | kUnloadPrivate | kUnloadSession;

#ifdef DOM_STORAGE_TESTS
  static const uint32_t kTestReload    = 1 << 15;
#endif

  void UnloadItems(uint32_t aUnloadFlags);

private:
  
  void WaitForPreload(mozilla::Telemetry::ID aTelemetryID);

  
  Data& DataSet(const DOMStorage* aStorage);

  
  bool Persist(const DOMStorage* aStorage) const;

  
  
  bool ProcessUsageDelta(uint32_t aGetDataSetIndex, const int64_t aDelta);
  bool ProcessUsageDelta(const DOMStorage* aStorage, const int64_t aDelta);

private:
  
  
  
  
  nsRefPtr<DOMStorageManager> mManager;

  
  
  nsRefPtr<DOMStorageUsage> mUsage;

  
  nsCOMPtr<nsITimer> mKeepAliveTimer;

  
  
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  nsCString mScope;

  
  nsCString mQuotaScope;

  
  Data mData[kDataSetCount];

  
  mozilla::Monitor mMonitor;

  
  
  
  
  bool mLoaded;

  
  nsresult mLoadResult;

  
  bool mInitialized : 1;

  
  
  
  bool mPersistent : 1;

  
  
  
  
  bool mSessionOnlyDataSetActive : 1;

  
  bool mPreloadTelemetryRecorded : 1;

  
  
  static DOMStorageDBBridge* sDatabase;

  
  static bool sDatabaseDown;
};



class DOMStorageUsageBridge
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DOMStorageUsageBridge)

  virtual const nsCString& Scope() = 0;
  virtual void LoadUsage(const int64_t aUsage) = 0;

protected:
  
  virtual ~DOMStorageUsageBridge() {}
};

class DOMStorageUsage : public DOMStorageUsageBridge
{
public:
  explicit DOMStorageUsage(const nsACString& aScope);

  bool CheckAndSetETLD1UsageDelta(uint32_t aDataSetIndex, int64_t aUsageDelta);

private:
  virtual const nsCString& Scope() { return mScope; }
  virtual void LoadUsage(const int64_t aUsage);

  nsCString mScope;
  int64_t mUsage[DOMStorageCache::kDataSetCount];
};

} 
} 

#endif
