




#include "DOMStorageCache.h"

#include "DOMStorage.h"
#include "DOMStorageDBThread.h"
#include "DOMStorageIPC.h"
#include "DOMStorageManager.h"

#include "nsDOMString.h"
#include "nsXULAppAPI.h"
#include "mozilla/unused.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {

#define DOM_STORAGE_CACHE_KEEP_ALIVE_TIME_MS 20000


DOMStorageDBBridge* DOMStorageCache::sDatabase = nullptr;
bool DOMStorageCache::sDatabaseDown = false;

namespace { 

const uint32_t kDefaultSet = 0;
const uint32_t kPrivateSet = 1;
const uint32_t kSessionSet = 2;

inline uint32_t
GetDataSetIndex(bool aPrivate, bool aSessionOnly)
{
  if (aPrivate) {
    return kPrivateSet;
  }

  if (aSessionOnly) {
    return kSessionSet;
  }

  return kDefaultSet;
}

inline uint32_t
GetDataSetIndex(const DOMStorage* aStorage)
{
  return GetDataSetIndex(aStorage->IsPrivate(), aStorage->IsSessionOnly());
}

} 



NS_IMPL_ADDREF(DOMStorageCacheBridge)




NS_IMETHODIMP_(void) DOMStorageCacheBridge::Release(void)
{
  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");
  nsrefcnt count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "DOMStorageCacheBridge");
  if (0 == count) {
    mRefCnt = 1; 
    
    
    delete (this);
  }
}



DOMStorageCache::DOMStorageCache(const nsACString* aScope)
: mScope(*aScope)
, mMonitor("DOMStorageCache")
, mLoaded(false)
, mLoadResult(NS_OK)
, mInitialized(false)
, mPersistent(false)
, mSessionOnlyDataSetActive(false)
, mPreloadTelemetryRecorded(false)
{
  MOZ_COUNT_CTOR(DOMStorageCache);
}

DOMStorageCache::~DOMStorageCache()
{
  if (mManager) {
    mManager->DropCache(this);
  }

  MOZ_COUNT_DTOR(DOMStorageCache);
}

NS_IMETHODIMP_(void)
DOMStorageCache::Release(void)
{
  
  
  
  if (NS_IsMainThread()) {
    DOMStorageCacheBridge::Release();
    return;
  }

  nsRefPtr<nsRunnableMethod<DOMStorageCacheBridge, void, false> > event =
    NS_NewNonOwningRunnableMethod(static_cast<DOMStorageCacheBridge*>(this),
                                  &DOMStorageCacheBridge::Release);

  nsresult rv = NS_DispatchToMainThread(event);
  if (NS_FAILED(rv)) {
    NS_WARNING("DOMStorageCache::Release() on a non-main thread");
    DOMStorageCacheBridge::Release();
  }
}

void
DOMStorageCache::Init(DOMStorageManager* aManager,
                      bool aPersistent,
                      nsIPrincipal* aPrincipal,
                      const nsACString& aQuotaScope)
{
  if (mInitialized) {
    return;
  }

  mInitialized = true;
  mPrincipal = aPrincipal;
  mPersistent = aPersistent;
  mQuotaScope = aQuotaScope.IsEmpty() ? mScope : aQuotaScope;

  if (mPersistent) {
    mManager = aManager;
    Preload();
  }

  mUsage = aManager->GetScopeUsage(mQuotaScope);
}

inline bool
DOMStorageCache::Persist(const DOMStorage* aStorage) const
{
  return mPersistent &&
         !aStorage->IsSessionOnly() &&
         !aStorage->IsPrivate();
}

namespace { 

PLDHashOperator
CloneSetData(const nsAString& aKey, const nsString aValue, void* aArg)
{
  DOMStorageCache::Data* target = static_cast<DOMStorageCache::Data*>(aArg);
  target->mKeys.Put(aKey, aValue);

  return PL_DHASH_NEXT;
}

} 

DOMStorageCache::Data&
DOMStorageCache::DataSet(const DOMStorage* aStorage)
{
  uint32_t index = GetDataSetIndex(aStorage);

  if (index == kSessionSet && !mSessionOnlyDataSetActive) {
    
    

    WaitForPreload(Telemetry::LOCALDOMSTORAGE_SESSIONONLY_PRELOAD_BLOCKING_MS);

    Data& defaultSet = mData[kDefaultSet];
    Data& sessionSet = mData[kSessionSet];

    defaultSet.mKeys.EnumerateRead(CloneSetData, &sessionSet);

    mSessionOnlyDataSetActive = true;

    
    
    ProcessUsageDelta(kSessionSet, defaultSet.mOriginQuotaUsage);
  }

  return mData[index];
}

bool
DOMStorageCache::ProcessUsageDelta(const DOMStorage* aStorage, int64_t aDelta)
{
  return ProcessUsageDelta(GetDataSetIndex(aStorage), aDelta);
}

bool
DOMStorageCache::ProcessUsageDelta(uint32_t aGetDataSetIndex, const int64_t aDelta)
{
  
  if (aDelta > 0 && mManager && mManager->IsLowDiskSpace()) {
    return false;
  }

  
  Data& data = mData[aGetDataSetIndex];
  uint64_t newOriginUsage = data.mOriginQuotaUsage + aDelta;
  if (aDelta > 0 && newOriginUsage > DOMStorageManager::GetQuota()) {
    return false;
  }

  
  if (mUsage && !mUsage->CheckAndSetETLD1UsageDelta(aGetDataSetIndex, aDelta)) {
    return false;
  }

  
  data.mOriginQuotaUsage = newOriginUsage;
  return true;
}

void
DOMStorageCache::Preload()
{
  if (mLoaded || !mPersistent) {
    return;
  }

  if (!StartDatabase()) {
    mLoaded = true;
    mLoadResult = NS_ERROR_FAILURE;
    return;
  }

  sDatabase->AsyncPreload(this);
}

namespace { 



class DOMStorageCacheHolder : public nsITimerCallback
{
  virtual ~DOMStorageCacheHolder() {}

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP
  Notify(nsITimer* aTimer)
  {
    mCache = nullptr;
    return NS_OK;
  }

  nsRefPtr<DOMStorageCache> mCache;

public:
  explicit DOMStorageCacheHolder(DOMStorageCache* aCache) : mCache(aCache) {}
};

NS_IMPL_ISUPPORTS(DOMStorageCacheHolder, nsITimerCallback)

} 

void
DOMStorageCache::KeepAlive()
{
  
  
  if (!mManager) {
    return;
  }

  if (!NS_IsMainThread()) {
    
    nsRefPtr<nsRunnableMethod<DOMStorageCache> > event =
      NS_NewRunnableMethod(this, &DOMStorageCache::KeepAlive);

    NS_DispatchToMainThread(event);
    return;
  }

  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
  if (!timer) {
    return;
  }

  nsRefPtr<DOMStorageCacheHolder> holder = new DOMStorageCacheHolder(this);
  timer->InitWithCallback(holder, DOM_STORAGE_CACHE_KEEP_ALIVE_TIME_MS,
                          nsITimer::TYPE_ONE_SHOT);

  mKeepAliveTimer.swap(timer);
}

namespace { 




class TelemetryAutoTimer
{
public:
  explicit TelemetryAutoTimer(Telemetry::ID aId)
    : id(aId), start(TimeStamp::Now()) {}
  ~TelemetryAutoTimer()
    { Telemetry::AccumulateDelta_impl<Telemetry::Millisecond>::compute(id, start); }
private:
  Telemetry::ID id;
  const TimeStamp start;
};

} 

void
DOMStorageCache::WaitForPreload(Telemetry::ID aTelemetryID)
{
  if (!mPersistent) {
    return;
  }

  bool loaded = mLoaded;

  
  if (!mPreloadTelemetryRecorded) {
    mPreloadTelemetryRecorded = true;
    Telemetry::Accumulate(
      Telemetry::LOCALDOMSTORAGE_PRELOAD_PENDING_ON_FIRST_ACCESS,
      !loaded);
  }

  if (loaded) {
    return;
  }

  
  TelemetryAutoTimer timer(aTelemetryID);

  
  
  

  

  
  
  
  sDatabase->SyncPreload(this);
}

nsresult
DOMStorageCache::GetLength(const DOMStorage* aStorage, uint32_t* aRetval)
{
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_GETLENGTH_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      return mLoadResult;
    }
  }

  *aRetval = DataSet(aStorage).mKeys.Count();
  return NS_OK;
}

namespace { 

class IndexFinderData
{
public:
  IndexFinderData(uint32_t aIndex, nsAString& aRetval)
    : mIndex(aIndex), mKey(aRetval)
  {
    mKey.SetIsVoid(true);
  }

  uint32_t mIndex;
  nsAString& mKey;
};

PLDHashOperator
FindKeyOrder(const nsAString& aKey, const nsString aValue, void* aArg)
{
  IndexFinderData* data = static_cast<IndexFinderData*>(aArg);

  if (data->mIndex--) {
    return PL_DHASH_NEXT;
  }

  data->mKey = aKey;
  return PL_DHASH_STOP;
}

} 

nsresult
DOMStorageCache::GetKey(const DOMStorage* aStorage, uint32_t aIndex, nsAString& aRetval)
{
  
  
  
  
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_GETKEY_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      return mLoadResult;
    }
  }

  IndexFinderData data(aIndex, aRetval);
  DataSet(aStorage).mKeys.EnumerateRead(FindKeyOrder, &data);
  return NS_OK;
}

namespace { 

static PLDHashOperator
KeysArrayBuilder(const nsAString& aKey, const nsString aValue, void* aArg)
{
  nsTArray<nsString>* keys = static_cast<nsTArray<nsString>* >(aArg);

  keys->AppendElement(aKey);
  return PL_DHASH_NEXT;
}

} 

void
DOMStorageCache::GetKeys(const DOMStorage* aStorage, nsTArray<nsString>& aKeys)
{
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_GETALLKEYS_BLOCKING_MS);
  }

  if (NS_FAILED(mLoadResult)) {
    return;
  }

  DataSet(aStorage).mKeys.EnumerateRead(KeysArrayBuilder, &aKeys);
}

nsresult
DOMStorageCache::GetItem(const DOMStorage* aStorage, const nsAString& aKey,
                         nsAString& aRetval)
{
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_GETVALUE_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      return mLoadResult;
    }
  }

  
  nsString value;
  if (!DataSet(aStorage).mKeys.Get(aKey, &value)) {
    SetDOMStringToNull(value);
  }

  aRetval = value;

  return NS_OK;
}

nsresult
DOMStorageCache::SetItem(const DOMStorage* aStorage, const nsAString& aKey,
                         const nsString& aValue, nsString& aOld)
{
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_SETVALUE_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      return mLoadResult;
    }
  }

  Data& data = DataSet(aStorage);
  if (!data.mKeys.Get(aKey, &aOld)) {
    SetDOMStringToNull(aOld);
  }

  
  const int64_t delta = static_cast<int64_t>(aValue.Length()) -
                        static_cast<int64_t>(aOld.Length());
  if (!ProcessUsageDelta(aStorage, delta)) {
    return NS_ERROR_DOM_QUOTA_REACHED;
  }

  if (aValue == aOld && DOMStringIsNull(aValue) == DOMStringIsNull(aOld)) {
    return NS_SUCCESS_DOM_NO_OPERATION;
  }

  data.mKeys.Put(aKey, aValue);

  if (Persist(aStorage)) {
    if (!sDatabase) {
      NS_ERROR("Writing to localStorage after the database has been shut down"
               ", data lose!");
      return NS_ERROR_NOT_INITIALIZED;
    }

    if (DOMStringIsNull(aOld)) {
      return sDatabase->AsyncAddItem(this, aKey, aValue);
    }

    return sDatabase->AsyncUpdateItem(this, aKey, aValue);
  }

  return NS_OK;
}

nsresult
DOMStorageCache::RemoveItem(const DOMStorage* aStorage, const nsAString& aKey,
                            nsString& aOld)
{
  if (Persist(aStorage)) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_REMOVEKEY_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      return mLoadResult;
    }
  }

  Data& data = DataSet(aStorage);
  if (!data.mKeys.Get(aKey, &aOld)) {
    SetDOMStringToNull(aOld);
    return NS_SUCCESS_DOM_NO_OPERATION;
  }

  
  const int64_t delta = -(static_cast<int64_t>(aOld.Length()));
  unused << ProcessUsageDelta(aStorage, delta);
  data.mKeys.Remove(aKey);

  if (Persist(aStorage)) {
    if (!sDatabase) {
      NS_ERROR("Writing to localStorage after the database has been shut down"
               ", data lose!");
      return NS_ERROR_NOT_INITIALIZED;
    }

    return sDatabase->AsyncRemoveItem(this, aKey);
  }

  return NS_OK;
}

nsresult
DOMStorageCache::Clear(const DOMStorage* aStorage)
{
  bool refresh = false;
  if (Persist(aStorage)) {
    
    
    
    
    
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_CLEAR_BLOCKING_MS);
    if (NS_FAILED(mLoadResult)) {
      
      
      refresh = true;
      mLoadResult = NS_OK;
    }
  }

  Data& data = DataSet(aStorage);
  bool hadData = !!data.mKeys.Count();

  if (hadData) {
    unused << ProcessUsageDelta(aStorage, -data.mOriginQuotaUsage);
    data.mKeys.Clear();
  }

  if (Persist(aStorage) && (refresh || hadData)) {
    if (!sDatabase) {
      NS_ERROR("Writing to localStorage after the database has been shut down"
               ", data lose!");
      return NS_ERROR_NOT_INITIALIZED;
    }

    return sDatabase->AsyncClear(this);
  }

  return hadData ? NS_OK : NS_SUCCESS_DOM_NO_OPERATION;
}

void
DOMStorageCache::CloneFrom(const DOMStorageCache* aThat)
{
  mLoaded = aThat->mLoaded;
  mInitialized = aThat->mInitialized;
  mPersistent = aThat->mPersistent;
  mSessionOnlyDataSetActive = aThat->mSessionOnlyDataSetActive;

  for (uint32_t i = 0; i < kDataSetCount; ++i) {
    aThat->mData[i].mKeys.EnumerateRead(CloneSetData, &mData[i]);
    ProcessUsageDelta(i, aThat->mData[i].mOriginQuotaUsage);
  }
}


extern bool
PrincipalsEqual(nsIPrincipal* aObjectPrincipal, nsIPrincipal* aSubjectPrincipal);

bool
DOMStorageCache::CheckPrincipal(nsIPrincipal* aPrincipal) const
{
  return PrincipalsEqual(mPrincipal, aPrincipal);
}

void
DOMStorageCache::UnloadItems(uint32_t aUnloadFlags)
{
  if (aUnloadFlags & kUnloadDefault) {
    
    
    
    
    
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_UNLOAD_BLOCKING_MS);

    mData[kDefaultSet].mKeys.Clear();
    ProcessUsageDelta(kDefaultSet, -mData[kDefaultSet].mOriginQuotaUsage);
  }

  if (aUnloadFlags & kUnloadPrivate) {
    mData[kPrivateSet].mKeys.Clear();
    ProcessUsageDelta(kPrivateSet, -mData[kPrivateSet].mOriginQuotaUsage);
  }

  if (aUnloadFlags & kUnloadSession) {
    mData[kSessionSet].mKeys.Clear();
    ProcessUsageDelta(kSessionSet, -mData[kSessionSet].mOriginQuotaUsage);
    mSessionOnlyDataSetActive = false;
  }

#ifdef DOM_STORAGE_TESTS
  if (aUnloadFlags & kTestReload) {
    WaitForPreload(Telemetry::LOCALDOMSTORAGE_UNLOAD_BLOCKING_MS);

    mData[kDefaultSet].mKeys.Clear();
    mLoaded = false; 
    Preload();
  }
#endif
}



uint32_t
DOMStorageCache::LoadedCount()
{
  MonitorAutoLock monitor(mMonitor);
  Data& data = mData[kDefaultSet];
  return data.mKeys.Count();
}

bool
DOMStorageCache::LoadItem(const nsAString& aKey, const nsString& aValue)
{
  MonitorAutoLock monitor(mMonitor);
  if (mLoaded) {
    return false;
  }

  Data& data = mData[kDefaultSet];
  if (data.mKeys.Get(aKey, nullptr)) {
    return true; 
  }

  data.mKeys.Put(aKey, aValue);
  data.mOriginQuotaUsage += aKey.Length() + aValue.Length();
  return true;
}

void
DOMStorageCache::LoadDone(nsresult aRv)
{
  
  KeepAlive();

  MonitorAutoLock monitor(mMonitor);
  mLoadResult = aRv;
  mLoaded = true;
  monitor.Notify();
}

void
DOMStorageCache::LoadWait()
{
  MonitorAutoLock monitor(mMonitor);
  while (!mLoaded) {
    monitor.Wait();
  }
}



DOMStorageUsage::DOMStorageUsage(const nsACString& aScope)
  : mScope(aScope)
{
  mUsage[kDefaultSet] = mUsage[kPrivateSet] = mUsage[kSessionSet] = 0LL;
}

namespace { 

class LoadUsageRunnable : public nsRunnable
{
public:
  LoadUsageRunnable(int64_t* aUsage, const int64_t aDelta)
    : mTarget(aUsage)
    , mDelta(aDelta)
  {}

private:
  int64_t* mTarget;
  int64_t mDelta;

  NS_IMETHOD Run() { *mTarget = mDelta; return NS_OK; }
};

} 

void
DOMStorageUsage::LoadUsage(const int64_t aUsage)
{
  
  
  if (!NS_IsMainThread()) {
    
    nsRefPtr<LoadUsageRunnable> r =
      new LoadUsageRunnable(mUsage + kDefaultSet, aUsage);
    NS_DispatchToMainThread(r);
  } else {
    
    mUsage[kDefaultSet] += aUsage;
  }
}

bool
DOMStorageUsage::CheckAndSetETLD1UsageDelta(uint32_t aDataSetIndex, const int64_t aDelta)
{
  MOZ_ASSERT(NS_IsMainThread());

  int64_t newUsage = mUsage[aDataSetIndex] + aDelta;
  if (aDelta > 0 && newUsage > DOMStorageManager::GetQuota()) {
    return false;
  }

  mUsage[aDataSetIndex] = newUsage;
  return true;
}



DOMStorageDBBridge*
DOMStorageCache::StartDatabase()
{
  if (sDatabase || sDatabaseDown) {
    
    
    
    return sDatabase;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsAutoPtr<DOMStorageDBThread> db(new DOMStorageDBThread());

    nsresult rv = db->Init();
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    sDatabase = db.forget();
  } else {
    nsRefPtr<DOMStorageDBChild> db = new DOMStorageDBChild(
        DOMLocalStorageManager::Self());

    nsresult rv = db->Init();
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    db.forget(&sDatabase);
  }

  return sDatabase;
}


DOMStorageDBBridge*
DOMStorageCache::GetDatabase()
{
  return sDatabase;
}


nsresult
DOMStorageCache::StopDatabase()
{
  if (!sDatabase) {
    return NS_OK;
  }

  sDatabaseDown = true;

  nsresult rv = sDatabase->Shutdown();
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    delete sDatabase;
  } else {
    DOMStorageDBChild* child = static_cast<DOMStorageDBChild*>(sDatabase);
    NS_RELEASE(child);
  }

  sDatabase = nullptr;
  return rv;
}

} 
} 
