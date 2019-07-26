



#include "CacheLog.h"
#include "CacheStorageService.h"
#include "CacheFileIOManager.h"
#include "CacheObserver.h"
#include "CacheIndex.h"

#include "nsICacheStorageVisitor.h"
#include "nsIObserverService.h"
#include "CacheStorage.h"
#include "AppCacheStorage.h"
#include "CacheEntry.h"
#include "CacheFileUtils.h"

#include "OldWrappers.h"
#include "nsCacheService.h"
#include "nsDeleteDir.h"

#include "nsIFile.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/VisualEventTracer.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace net {

namespace {

void AppendMemoryStorageID(nsAutoCString &key)
{
  key.Append('/');
  key.Append('M');
}

}




typedef nsClassHashtable<nsCStringHashKey, CacheEntryTable>
        GlobalEntryTables;








static GlobalEntryTables* sGlobalEntryTables;

CacheMemoryConsumer::CacheMemoryConsumer(uint32_t aFlags)
: mReportedMemoryConsumption(0)
, mFlags(aFlags)
{
}

void
CacheMemoryConsumer::DoMemoryReport(uint32_t aCurrentSize)
{
  if (!(mFlags & DONT_REPORT) && CacheStorageService::Self()) {
    CacheStorageService::Self()->OnMemoryConsumptionChange(this, aCurrentSize);
  }
}

CacheStorageService::MemoryPool::MemoryPool(EType aType)
: mType(aType)
, mMemorySize(0)
{
}

CacheStorageService::MemoryPool::~MemoryPool()
{
  if (mMemorySize != 0) {
    NS_ERROR("Network cache reported memory consumption is not at 0, probably leaking?");
  }
}

uint32_t const
CacheStorageService::MemoryPool::Limit() const
{
  switch (mType) {
  case DISK:
    return CacheObserver::MetadataMemoryLimit();
  case MEMORY:
    return CacheObserver::MemoryCacheCapacity();
  }

  MOZ_CRASH("Bad pool type");
  return 0;
}

NS_IMPL_ISUPPORTS(CacheStorageService,
                  nsICacheStorageService,
                  nsIMemoryReporter,
                  nsITimerCallback)

CacheStorageService* CacheStorageService::sSelf = nullptr;

CacheStorageService::CacheStorageService()
: mLock("CacheStorageService")
, mShutdown(false)
, mDiskPool(MemoryPool::DISK)
, mMemoryPool(MemoryPool::MEMORY)
{
  CacheFileIOManager::Init();

  MOZ_ASSERT(!sSelf);

  sSelf = this;
  sGlobalEntryTables = new GlobalEntryTables();

  RegisterStrongMemoryReporter(this);
}

CacheStorageService::~CacheStorageService()
{
  LOG(("CacheStorageService::~CacheStorageService"));
  sSelf = nullptr;
}

void CacheStorageService::Shutdown()
{
  if (mShutdown)
    return;

  LOG(("CacheStorageService::Shutdown - start"));

  mShutdown = true;

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &CacheStorageService::ShutdownBackground);
  Dispatch(event);

  mozilla::MutexAutoLock lock(mLock);
  sGlobalEntryTables->Clear();
  delete sGlobalEntryTables;
  sGlobalEntryTables = nullptr;

  LOG(("CacheStorageService::Shutdown - done"));
}

void CacheStorageService::ShutdownBackground()
{
  MOZ_ASSERT(IsOnManagementThread());

  Pool(false).mFrecencyArray.Clear();
  Pool(false).mExpirationArray.Clear();
  Pool(true).mFrecencyArray.Clear();
  Pool(true).mExpirationArray.Clear();
}



namespace { 




class WalkRunnable : public nsRunnable
{
public:
  WalkRunnable(nsCSubstring const & aContextKey, bool aVisitEntries,
               bool aUsingDisk,
               nsICacheStorageVisitor* aVisitor)
    : mContextKey(aContextKey)
    , mCallback(aVisitor)
    , mSize(0)
    , mNotifyStorage(true)
    , mVisitEntries(aVisitEntries)
    , mUsingDisk(aUsingDisk)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

private:
  NS_IMETHODIMP Run()
  {
    if (CacheStorageService::IsOnManagementThread()) {
      LOG(("WalkRunnable::Run - collecting [this=%p, disk=%d]", this, (bool)mUsingDisk));
      
      
      
      
      

      mozilla::MutexAutoLock lock(CacheStorageService::Self()->Lock());

      if (!CacheStorageService::IsRunning())
        return NS_ERROR_NOT_INITIALIZED;

      CacheEntryTable* entries;
      if (sGlobalEntryTables->Get(mContextKey, &entries))
        entries->EnumerateRead(&WalkRunnable::WalkStorage, this);

      
    }
    else if (NS_IsMainThread()) {
      LOG(("WalkRunnable::Run - notifying [this=%p, disk=%d]", this, (bool)mUsingDisk));
      if (mNotifyStorage) {
        LOG(("  storage"));
        
        mCallback->OnCacheStorageInfo(mEntryArray.Length(), mSize);
        if (!mVisitEntries)
          return NS_OK; 

        mNotifyStorage = false;
      }
      else {
        LOG(("  entry [left=%d]", mEntryArray.Length()));
        
        if (!mEntryArray.Length()) {
          mCallback->OnCacheEntryVisitCompleted();
          return NS_OK; 
        }

        mCallback->OnCacheEntryInfo(mEntryArray[0]);
        mEntryArray.RemoveElementAt(0);

        
      }
    }
    else {
      MOZ_ASSERT(false);
      return NS_ERROR_FAILURE;
    }

    NS_DispatchToMainThread(this);
    return NS_OK;
  }

  virtual ~WalkRunnable()
  {
    if (mCallback)
      ProxyReleaseMainThread(mCallback);
  }

  static PLDHashOperator
  WalkStorage(const nsACString& aKey,
              CacheEntry* aEntry,
              void* aClosure)
  {
    WalkRunnable* walker = static_cast<WalkRunnable*>(aClosure);

    if (!walker->mUsingDisk && aEntry->IsUsingDiskLocked())
      return PL_DHASH_NEXT;

    walker->mSize += aEntry->GetMetadataMemoryConsumption();

    int64_t size;
    if (NS_SUCCEEDED(aEntry->GetDataSize(&size)))
      walker->mSize += size;

    walker->mEntryArray.AppendElement(aEntry);
    return PL_DHASH_NEXT;
  }

  nsCString mContextKey;
  nsCOMPtr<nsICacheStorageVisitor> mCallback;
  nsTArray<nsRefPtr<CacheEntry> > mEntryArray;

  uint64_t mSize;

  bool mNotifyStorage : 1;
  bool mVisitEntries : 1;
  bool mUsingDisk : 1;
};

PLDHashOperator CollectPrivateContexts(const nsACString& aKey,
                                       CacheEntryTable* aTable,
                                       void* aClosure)
{
  nsCOMPtr<nsILoadContextInfo> info = CacheFileUtils::ParseKey(aKey);
  if (info && info->IsPrivate()) {
    nsTArray<nsCString>* keys = static_cast<nsTArray<nsCString>*>(aClosure);
    keys->AppendElement(aKey);
  }

  return PL_DHASH_NEXT;
}

PLDHashOperator CollectContexts(const nsACString& aKey,
                                       CacheEntryTable* aTable,
                                       void* aClosure)
{
  nsTArray<nsCString>* keys = static_cast<nsTArray<nsCString>*>(aClosure);
  keys->AppendElement(aKey);

  return PL_DHASH_NEXT;
}

} 

void CacheStorageService::DropPrivateBrowsingEntries()
{
  mozilla::MutexAutoLock lock(mLock);

  if (mShutdown)
    return;

  nsTArray<nsCString> keys;
  sGlobalEntryTables->EnumerateRead(&CollectPrivateContexts, &keys);

  for (uint32_t i = 0; i < keys.Length(); ++i)
    DoomStorageEntries(keys[i], nullptr, true, nullptr);
}


void CacheStorageService::WipeCacheDirectory(uint32_t aVersion)
{
  nsCOMPtr<nsIFile> cacheDir;
  switch (aVersion) {
  case 0:
    nsCacheService::GetDiskCacheDirectory(getter_AddRefs(cacheDir));
    break;
  case 1:
    CacheFileIOManager::GetCacheDirectory(getter_AddRefs(cacheDir));
    break;
  }

  if (!cacheDir)
    return;

  nsDeleteDir::DeleteDir(cacheDir, true, 30000);
}




bool CacheStorageService::IsOnManagementThread()
{
  nsRefPtr<CacheStorageService> service = Self();
  if (!service)
    return false;

  nsCOMPtr<nsIEventTarget> target = service->Thread();
  if (!target)
    return false;

  bool currentThread;
  nsresult rv = target->IsOnCurrentThread(&currentThread);
  return NS_SUCCEEDED(rv) && currentThread;
}

already_AddRefed<nsIEventTarget> CacheStorageService::Thread() const
{
  return CacheFileIOManager::IOTarget();
}

nsresult CacheStorageService::Dispatch(nsIRunnable* aEvent)
{
  nsRefPtr<CacheIOThread> cacheIOThread = CacheFileIOManager::IOThread();
  if (!cacheIOThread)
    return NS_ERROR_NOT_AVAILABLE;

  return cacheIOThread->Dispatch(aEvent, CacheIOThread::MANAGEMENT);
}



NS_IMETHODIMP CacheStorageService::MemoryCacheStorage(nsILoadContextInfo *aLoadContextInfo,
                                                      nsICacheStorage * *_retval)
{
  NS_ENSURE_ARG(aLoadContextInfo);
  NS_ENSURE_ARG(_retval);

  nsCOMPtr<nsICacheStorage> storage;
  if (CacheObserver::UseNewCache()) {
    storage = new CacheStorage(aLoadContextInfo, false, false);
  }
  else {
    storage = new _OldStorage(aLoadContextInfo, false, false, false, nullptr);
  }

  storage.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP CacheStorageService::DiskCacheStorage(nsILoadContextInfo *aLoadContextInfo,
                                                    bool aLookupAppCache,
                                                    nsICacheStorage * *_retval)
{
  NS_ENSURE_ARG(aLoadContextInfo);
  NS_ENSURE_ARG(_retval);

  

  
  
  bool useDisk = CacheObserver::UseDiskCache();

  nsCOMPtr<nsICacheStorage> storage;
  if (CacheObserver::UseNewCache()) {
    storage = new CacheStorage(aLoadContextInfo, useDisk, aLookupAppCache);
  }
  else {
    storage = new _OldStorage(aLoadContextInfo, useDisk, aLookupAppCache, false, nullptr);
  }

  storage.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP CacheStorageService::AppCacheStorage(nsILoadContextInfo *aLoadContextInfo,
                                                   nsIApplicationCache *aApplicationCache,
                                                   nsICacheStorage * *_retval)
{
  NS_ENSURE_ARG(aLoadContextInfo);
  NS_ENSURE_ARG(_retval);

  nsCOMPtr<nsICacheStorage> storage;
  if (CacheObserver::UseNewCache()) {
    
    
    storage = new mozilla::net::AppCacheStorage(aLoadContextInfo, aApplicationCache);
  }
  else {
    storage = new _OldStorage(aLoadContextInfo, true, false, true, aApplicationCache);
  }

  storage.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP CacheStorageService::Clear()
{
  nsresult rv;

  if (CacheObserver::UseNewCache()) {
    {
      mozilla::MutexAutoLock lock(mLock);

      NS_ENSURE_TRUE(!mShutdown, NS_ERROR_NOT_INITIALIZED);

      nsTArray<nsCString> keys;
      sGlobalEntryTables->EnumerateRead(&CollectContexts, &keys);

      for (uint32_t i = 0; i < keys.Length(); ++i)
        DoomStorageEntries(keys[i], nullptr, true, nullptr);
    }

    rv = CacheFileIOManager::EvictAll();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = serv->EvictEntries(nsICache::STORE_ANYWHERE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP CacheStorageService::PurgeFromMemory(uint32_t aWhat)
{
  uint32_t what;

  switch (aWhat) {
  case PURGE_DISK_DATA_ONLY:
    what = CacheEntry::PURGE_DATA_ONLY_DISK_BACKED;
    break;

  case PURGE_DISK_ALL:
    what = CacheEntry::PURGE_WHOLE_ONLY_DISK_BACKED;
    break;

  case PURGE_EVERYTHING:
    what = CacheEntry::PURGE_WHOLE;
    break;

  default:
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIRunnable> event =
    new PurgeFromMemoryRunnable(this, what);

  return Dispatch(event);
}

NS_IMETHODIMP CacheStorageService::AsyncGetDiskConsumption(
  nsICacheStorageConsumptionObserver* aObserver)
{
  NS_ENSURE_ARG(aObserver);

  nsresult rv;

  if (CacheObserver::UseNewCache()) {
    rv = CacheIndex::AsyncGetDiskConsumption(aObserver);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = _OldGetDiskConsumption::Get(aObserver);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP CacheStorageService::GetIoTarget(nsIEventTarget** aEventTarget)
{
  NS_ENSURE_ARG(aEventTarget);

  if (CacheObserver::UseNewCache()) {
    nsCOMPtr<nsIEventTarget> ioTarget = CacheFileIOManager::IOTarget();
    ioTarget.forget(aEventTarget);
  }
  else {
    nsresult rv;

    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = serv->GetCacheIOTarget(aEventTarget);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



namespace { 

class FrecencyComparator
{
public:
  bool Equals(CacheEntry* a, CacheEntry* b) const {
    return a->GetFrecency() == b->GetFrecency();
  }
  bool LessThan(CacheEntry* a, CacheEntry* b) const {
    return a->GetFrecency() < b->GetFrecency();
  }
};

class ExpirationComparator
{
public:
  bool Equals(CacheEntry* a, CacheEntry* b) const {
    return a->GetExpirationTime() == b->GetExpirationTime();
  }
  bool LessThan(CacheEntry* a, CacheEntry* b) const {
    return a->GetExpirationTime() < b->GetExpirationTime();
  }
};

} 

void
CacheStorageService::RegisterEntry(CacheEntry* aEntry)
{
  MOZ_ASSERT(IsOnManagementThread());

  if (mShutdown || !aEntry->CanRegister())
    return;

  LOG(("CacheStorageService::RegisterEntry [entry=%p]", aEntry));

  MemoryPool& pool = Pool(aEntry->IsUsingDisk());
  pool.mFrecencyArray.InsertElementSorted(aEntry, FrecencyComparator());
  pool.mExpirationArray.InsertElementSorted(aEntry, ExpirationComparator());

  aEntry->SetRegistered(true);
}

void
CacheStorageService::UnregisterEntry(CacheEntry* aEntry)
{
  MOZ_ASSERT(IsOnManagementThread());

  if (!aEntry->IsRegistered())
    return;

  LOG(("CacheStorageService::UnregisterEntry [entry=%p]", aEntry));

  MemoryPool& pool = Pool(aEntry->IsUsingDisk());
  mozilla::DebugOnly<bool> removedFrecency = pool.mFrecencyArray.RemoveElement(aEntry);
  mozilla::DebugOnly<bool> removedExpiration = pool.mExpirationArray.RemoveElement(aEntry);

  MOZ_ASSERT(mShutdown || (removedFrecency && removedExpiration));

  
  aEntry->SetRegistered(false);
}

static bool
AddExactEntry(CacheEntryTable* aEntries,
              nsCString const& aKey,
              CacheEntry* aEntry,
              bool aOverwrite)
{
  nsRefPtr<CacheEntry> existingEntry;
  if (!aOverwrite && aEntries->Get(aKey, getter_AddRefs(existingEntry))) {
    bool equals = existingEntry == aEntry;
    LOG(("AddExactEntry [entry=%p equals=%d]", aEntry, equals));
    return equals; 
  }

  LOG(("AddExactEntry [entry=%p put]", aEntry));
  aEntries->Put(aKey, aEntry);
  return true;
}

static bool
RemoveExactEntry(CacheEntryTable* aEntries,
                 nsCString const& aKey,
                 CacheEntry* aEntry,
                 bool aOverwrite)
{
  nsRefPtr<CacheEntry> existingEntry;
  if (!aEntries->Get(aKey, getter_AddRefs(existingEntry))) {
    LOG(("RemoveExactEntry [entry=%p already gone]", aEntry));
    return false; 
  }

  if (!aOverwrite && existingEntry != aEntry) {
    LOG(("RemoveExactEntry [entry=%p already replaced]", aEntry));
    return false; 
  }

  LOG(("RemoveExactEntry [entry=%p removed]", aEntry));
  aEntries->Remove(aKey);
  return true;
}

bool
CacheStorageService::RemoveEntry(CacheEntry* aEntry, bool aOnlyUnreferenced)
{
  LOG(("CacheStorageService::RemoveEntry [entry=%p]", aEntry));

  nsAutoCString entryKey;
  nsresult rv = aEntry->HashingKey(entryKey);
  if (NS_FAILED(rv)) {
    NS_ERROR("aEntry->HashingKey() failed?");
    return false;
  }

  mozilla::MutexAutoLock lock(mLock);

  if (mShutdown) {
    LOG(("  after shutdown"));
    return false;
  }

  if (aOnlyUnreferenced && aEntry->IsReferenced()) {
    LOG(("  still referenced, not removing"));
    return false;
  }

  CacheEntryTable* entries;
  if (sGlobalEntryTables->Get(aEntry->GetStorageID(), &entries))
    RemoveExactEntry(entries, entryKey, aEntry, false );

  nsAutoCString memoryStorageID(aEntry->GetStorageID());
  AppendMemoryStorageID(memoryStorageID);

  if (sGlobalEntryTables->Get(memoryStorageID, &entries))
    RemoveExactEntry(entries, entryKey, aEntry, false );

  return true;
}

void
CacheStorageService::RecordMemoryOnlyEntry(CacheEntry* aEntry,
                                           bool aOnlyInMemory,
                                           bool aOverwrite)
{
  LOG(("CacheStorageService::RecordMemoryOnlyEntry [entry=%p, memory=%d, overwrite=%d]",
    aEntry, aOnlyInMemory, aOverwrite));
  
  
  
  
  

  mLock.AssertCurrentThreadOwns();

  if (mShutdown) {
    LOG(("  after shutdown"));
    return;
  }

  nsresult rv;

  nsAutoCString entryKey;
  rv = aEntry->HashingKey(entryKey);
  if (NS_FAILED(rv)) {
    NS_ERROR("aEntry->HashingKey() failed?");
    return;
  }

  CacheEntryTable* entries = nullptr;
  nsAutoCString memoryStorageID(aEntry->GetStorageID());
  AppendMemoryStorageID(memoryStorageID);

  if (!sGlobalEntryTables->Get(memoryStorageID, &entries)) {
    if (!aOnlyInMemory) {
      LOG(("  not recorded as memory only"));
      return;
    }

    entries = new CacheEntryTable(CacheEntryTable::MEMORY_ONLY);
    sGlobalEntryTables->Put(memoryStorageID, entries);
    LOG(("  new memory-only storage table for %s", memoryStorageID.get()));
  }

  if (aOnlyInMemory) {
    AddExactEntry(entries, entryKey, aEntry, aOverwrite);
  }
  else {
    RemoveExactEntry(entries, entryKey, aEntry, aOverwrite);
  }
}

void
CacheStorageService::OnMemoryConsumptionChange(CacheMemoryConsumer* aConsumer,
                                               uint32_t aCurrentMemoryConsumption)
{
  LOG(("CacheStorageService::OnMemoryConsumptionChange [consumer=%p, size=%u]",
    aConsumer, aCurrentMemoryConsumption));

  uint32_t savedMemorySize = aConsumer->mReportedMemoryConsumption;
  if (savedMemorySize == aCurrentMemoryConsumption)
    return;

  
  aConsumer->mReportedMemoryConsumption = aCurrentMemoryConsumption;

  bool usingDisk = !(aConsumer->mFlags & CacheMemoryConsumer::MEMORY_ONLY);
  bool overLimit = Pool(usingDisk).OnMemoryConsumptionChange(
    savedMemorySize, aCurrentMemoryConsumption);

  if (!overLimit)
    return;

  
  
  if (mPurgeTimer)
    return;

  
  
  nsRefPtr<nsIEventTarget> cacheIOTarget = Thread();
  if (!cacheIOTarget)
    return;

  
  
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &CacheStorageService::SchedulePurgeOverMemoryLimit);
  cacheIOTarget->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
}

bool
CacheStorageService::MemoryPool::OnMemoryConsumptionChange(uint32_t aSavedMemorySize,
                                                           uint32_t aCurrentMemoryConsumption)
{
  mMemorySize -= aSavedMemorySize;
  mMemorySize += aCurrentMemoryConsumption;

  LOG(("  mMemorySize=%u (+%u,-%u)", uint32_t(mMemorySize), aCurrentMemoryConsumption, aSavedMemorySize));

  
  if (aCurrentMemoryConsumption <= aSavedMemorySize)
    return false;

  return mMemorySize > Limit();
}

void
CacheStorageService::SchedulePurgeOverMemoryLimit()
{
  mozilla::MutexAutoLock lock(mLock);

  if (mPurgeTimer)
    return;

  mPurgeTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (mPurgeTimer)
    mPurgeTimer->InitWithCallback(this, 1000, nsITimer::TYPE_ONE_SHOT);
}

NS_IMETHODIMP
CacheStorageService::Notify(nsITimer* aTimer)
{
  if (aTimer == mPurgeTimer) {
    mPurgeTimer = nullptr;

    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &CacheStorageService::PurgeOverMemoryLimit);
    Dispatch(event);
  }

  return NS_OK;
}

void
CacheStorageService::PurgeOverMemoryLimit()
{
  MOZ_ASSERT(IsOnManagementThread());

  LOG(("CacheStorageService::PurgeOverMemoryLimit"));

  Pool(true).PurgeOverMemoryLimit();
  Pool(false).PurgeOverMemoryLimit();
}

void
CacheStorageService::MemoryPool::PurgeOverMemoryLimit()
{
#ifdef PR_LOGGING
  TimeStamp start(TimeStamp::Now());
#endif

  uint32_t const memoryLimit = Limit();
  if (mMemorySize > memoryLimit) {
    LOG(("  memory data consumption over the limit, abandon expired entries"));
    PurgeExpired();
  }

  bool frecencyNeedsSort = true;

  
  
  
  
  
  
#if 0
  if (mMemorySize > memoryLimit) {
    LOG(("  memory data consumption over the limit, abandon disk backed data"));
    PurgeByFrecency(frecencyNeedsSort, CacheEntry::PURGE_DATA_ONLY_DISK_BACKED);
  }

  if (mMemorySize > memoryLimit) {
    LOG(("  metadata consumtion over the limit, abandon disk backed entries"));
    PurgeByFrecency(frecencyNeedsSort, CacheEntry::PURGE_WHOLE_ONLY_DISK_BACKED);
  }
#endif

  if (mMemorySize > memoryLimit) {
    LOG(("  memory data consumption over the limit, abandon any entry"));
    PurgeByFrecency(frecencyNeedsSort, CacheEntry::PURGE_WHOLE);
  }

  LOG(("  purging took %1.2fms", (TimeStamp::Now() - start).ToMilliseconds()));
}

void
CacheStorageService::MemoryPool::PurgeExpired()
{
  MOZ_ASSERT(IsOnManagementThread());

  mExpirationArray.Sort(ExpirationComparator());
  uint32_t now = NowInSeconds();

  uint32_t const memoryLimit = Limit();

  for (uint32_t i = 0; mMemorySize > memoryLimit && i < mExpirationArray.Length();) {
    if (CacheIOThread::YieldAndRerun())
      return;

    nsRefPtr<CacheEntry> entry = mExpirationArray[i];

    uint32_t expirationTime = entry->GetExpirationTime();
    if (expirationTime > 0 && expirationTime <= now) {
      LOG(("  dooming expired entry=%p, exptime=%u (now=%u)",
        entry.get(), entry->GetExpirationTime(), now));

      entry->PurgeAndDoom();
      continue;
    }

    
    ++i;
  }
}

void
CacheStorageService::MemoryPool::PurgeByFrecency(bool &aFrecencyNeedsSort, uint32_t aWhat)
{
  MOZ_ASSERT(IsOnManagementThread());

  if (aFrecencyNeedsSort) {
    mFrecencyArray.Sort(FrecencyComparator());
    aFrecencyNeedsSort = false;
  }

  uint32_t const memoryLimit = Limit();

  for (uint32_t i = 0; mMemorySize > memoryLimit && i < mFrecencyArray.Length();) {
    if (CacheIOThread::YieldAndRerun())
      return;

    nsRefPtr<CacheEntry> entry = mFrecencyArray[i];

    if (entry->Purge(aWhat)) {
      LOG(("  abandoned (%d), entry=%p, frecency=%1.10f",
        aWhat, entry.get(), entry->GetFrecency()));
      continue;
    }

    
    ++i;
  }
}

void
CacheStorageService::MemoryPool::PurgeAll(uint32_t aWhat)
{
  LOG(("CacheStorageService::MemoryPool::PurgeAll aWhat=%d", aWhat));
  MOZ_ASSERT(IsOnManagementThread());

  for (uint32_t i = 0; i < mFrecencyArray.Length();) {
    if (CacheIOThread::YieldAndRerun())
      return;

    nsRefPtr<CacheEntry> entry = mFrecencyArray[i];

    if (entry->Purge(aWhat)) {
      LOG(("  abandoned entry=%p", entry.get()));
      continue;
    }

    
    ++i;
  }
}



nsresult
CacheStorageService::AddStorageEntry(CacheStorage const* aStorage,
                                     nsIURI* aURI,
                                     const nsACString & aIdExtension,
                                     bool aCreateIfNotExist,
                                     bool aReplace,
                                     CacheEntryHandle** aResult)
{
  NS_ENSURE_FALSE(mShutdown, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG(aStorage);

  nsAutoCString contextKey;
  CacheFileUtils::AppendKeyPrefix(aStorage->LoadInfo(), contextKey);

  return AddStorageEntry(contextKey, aURI, aIdExtension,
                         aStorage->WriteToDisk(), aCreateIfNotExist, aReplace,
                         aResult);
}

nsresult
CacheStorageService::AddStorageEntry(nsCSubstring const& aContextKey,
                                     nsIURI* aURI,
                                     const nsACString & aIdExtension,
                                     bool aWriteToDisk,
                                     bool aCreateIfNotExist,
                                     bool aReplace,
                                     CacheEntryHandle** aResult)
{
  NS_ENSURE_ARG(aURI);

  nsresult rv;

  nsAutoCString entryKey;
  rv = CacheEntry::HashingKey(EmptyCString(), aIdExtension, aURI, entryKey);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("CacheStorageService::AddStorageEntry [entryKey=%s, contextKey=%s]",
    entryKey.get(), aContextKey.BeginReading()));

  nsRefPtr<CacheEntry> entry;
  nsRefPtr<CacheEntryHandle> handle;

  {
    mozilla::MutexAutoLock lock(mLock);

    NS_ENSURE_FALSE(mShutdown, NS_ERROR_NOT_INITIALIZED);

    
    CacheEntryTable* entries;
    if (!sGlobalEntryTables->Get(aContextKey, &entries)) {
      entries = new CacheEntryTable(CacheEntryTable::ALL_ENTRIES);
      sGlobalEntryTables->Put(aContextKey, entries);
      LOG(("  new storage entries table for context %s", aContextKey.BeginReading()));
    }

    bool entryExists = entries->Get(entryKey, getter_AddRefs(entry));

    
    if (entryExists && entry->IsFileDoomed() && !aReplace) {
      LOG(("  file already doomed, replacing the entry"));
      aReplace = true;
    }

    
    if (entryExists && aReplace) {
      entries->Remove(entryKey);

      LOG(("  dooming entry %p for %s because of OPEN_TRUNCATE", entry.get(), entryKey.get()));
      
      
      
      entry->DoomAlreadyRemoved();

      entry = nullptr;
      entryExists = false;
    }

    if (entryExists && entry->SetUsingDisk(aWriteToDisk)) {
      RecordMemoryOnlyEntry(entry, !aWriteToDisk, true );
    }

    
    if (!entryExists && (aCreateIfNotExist || aReplace)) {
      
      entry = new CacheEntry(aContextKey, aURI, aIdExtension, aWriteToDisk);
      entries->Put(entryKey, entry);
      LOG(("  new entry %p for %s", entry.get(), entryKey.get()));
    }

    if (entry) {
      
      
      handle = entry->NewHandle();
    }
  }

  handle.forget(aResult);
  return NS_OK;
}

namespace { 

class CacheEntryDoomByKeyCallback : public CacheFileIOListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  CacheEntryDoomByKeyCallback(nsICacheEntryDoomCallback* aCallback)
    : mCallback(aCallback) { }
  virtual ~CacheEntryDoomByKeyCallback();

private:
  NS_IMETHOD OnFileOpened(CacheFileHandle *aHandle, nsresult aResult) { return NS_OK; }
  NS_IMETHOD OnDataWritten(CacheFileHandle *aHandle, const char *aBuf, nsresult aResult) { return NS_OK; }
  NS_IMETHOD OnDataRead(CacheFileHandle *aHandle, char *aBuf, nsresult aResult) { return NS_OK; }
  NS_IMETHOD OnFileDoomed(CacheFileHandle *aHandle, nsresult aResult);
  NS_IMETHOD OnEOFSet(CacheFileHandle *aHandle, nsresult aResult) { return NS_OK; }
  NS_IMETHOD OnFileRenamed(CacheFileHandle *aHandle, nsresult aResult) { return NS_OK; }

  nsCOMPtr<nsICacheEntryDoomCallback> mCallback;
};

CacheEntryDoomByKeyCallback::~CacheEntryDoomByKeyCallback()
{
  if (mCallback)
    ProxyReleaseMainThread(mCallback);
}

NS_IMETHODIMP CacheEntryDoomByKeyCallback::OnFileDoomed(CacheFileHandle *aHandle,
                                                        nsresult aResult)
{
  if (!mCallback)
    return NS_OK;

  mCallback->OnCacheEntryDoomed(aResult);
  return NS_OK;
}

NS_IMPL_ISUPPORTS(CacheEntryDoomByKeyCallback, CacheFileIOListener);

} 

nsresult
CacheStorageService::DoomStorageEntry(CacheStorage const* aStorage,
                                      nsIURI *aURI,
                                      const nsACString & aIdExtension,
                                      nsICacheEntryDoomCallback* aCallback)
{
  LOG(("CacheStorageService::DoomStorageEntry"));

  NS_ENSURE_ARG(aStorage);
  NS_ENSURE_ARG(aURI);

  nsAutoCString contextKey;
  CacheFileUtils::AppendKeyPrefix(aStorage->LoadInfo(), contextKey);

  nsAutoCString entryKey;
  nsresult rv = CacheEntry::HashingKey(EmptyCString(), aIdExtension, aURI, entryKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<CacheEntry> entry;
  {
    mozilla::MutexAutoLock lock(mLock);

    NS_ENSURE_FALSE(mShutdown, NS_ERROR_NOT_INITIALIZED);

    CacheEntryTable* entries;
    if (sGlobalEntryTables->Get(contextKey, &entries)) {
      if (entries->Get(entryKey, getter_AddRefs(entry))) {
        if (aStorage->WriteToDisk() || !entry->IsUsingDiskLocked()) {
          
          
          LOG(("  purging entry %p for %s [storage use disk=%d, entry use disk=%d]",
            entry.get(), entryKey.get(), aStorage->WriteToDisk(), entry->IsUsingDiskLocked()));
          entries->Remove(entryKey);
        }
        else {
          
          LOG(("  leaving entry %p for %s [storage use disk=%d, entry use disk=%d]",
            entry.get(), entryKey.get(), aStorage->WriteToDisk(), entry->IsUsingDiskLocked()));
          entry = nullptr;
        }
      }
    }
  }

  if (entry) {
    LOG(("  dooming entry %p for %s", entry.get(), entryKey.get()));
    return entry->AsyncDoom(aCallback);
  }

  LOG(("  no entry loaded for %s", entryKey.get()));

  if (aStorage->WriteToDisk()) {
    nsAutoCString contextKey;
    CacheFileUtils::AppendKeyPrefix(aStorage->LoadInfo(), contextKey);

    rv = CacheEntry::HashingKey(contextKey, aIdExtension, aURI, entryKey);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("  dooming file only for %s", entryKey.get()));

    nsRefPtr<CacheEntryDoomByKeyCallback> callback(
      new CacheEntryDoomByKeyCallback(aCallback));
    rv = CacheFileIOManager::DoomFileByKey(entryKey, callback);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  if (aCallback)
    aCallback->OnCacheEntryDoomed(NS_ERROR_NOT_AVAILABLE);

  return NS_OK;
}

nsresult
CacheStorageService::DoomStorageEntries(CacheStorage const* aStorage,
                                        nsICacheEntryDoomCallback* aCallback)
{
  LOG(("CacheStorageService::DoomStorageEntries"));

  NS_ENSURE_FALSE(mShutdown, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG(aStorage);

  nsAutoCString contextKey;
  CacheFileUtils::AppendKeyPrefix(aStorage->LoadInfo(), contextKey);

  mozilla::MutexAutoLock lock(mLock);

  return DoomStorageEntries(contextKey, aStorage->LoadInfo(),
                            aStorage->WriteToDisk(), aCallback);
}

nsresult
CacheStorageService::DoomStorageEntries(nsCSubstring const& aContextKey,
                                        nsILoadContextInfo* aContext,
                                        bool aDiskStorage,
                                        nsICacheEntryDoomCallback* aCallback)
{
  mLock.AssertCurrentThreadOwns();

  NS_ENSURE_TRUE(!mShutdown, NS_ERROR_NOT_INITIALIZED);

  nsAutoCString memoryStorageID(aContextKey);
  AppendMemoryStorageID(memoryStorageID);

  if (aDiskStorage) {
    LOG(("  dooming disk+memory storage of %s", aContextKey.BeginReading()));

    
    sGlobalEntryTables->Remove(aContextKey);
    sGlobalEntryTables->Remove(memoryStorageID);

    if (aContext && !aContext->IsPrivate()) {
      LOG(("  dooming disk entries"));
      CacheFileIOManager::EvictByContext(aContext);
    }
  } else {
    LOG(("  dooming memory-only storage of %s", aContextKey.BeginReading()));

    class MemoryEntriesRemoval {
    public:
      static PLDHashOperator EvictEntry(const nsACString& aKey,
                                        CacheEntry* aEntry,
                                        void* aClosure)
      {
        CacheEntryTable* entries = static_cast<CacheEntryTable*>(aClosure);
        nsCString key(aKey);
        RemoveExactEntry(entries, key, aEntry, false);
        return PL_DHASH_NEXT;
      }
    };

    
    
    
    
    nsAutoPtr<CacheEntryTable> memoryEntries;
    sGlobalEntryTables->RemoveAndForget(memoryStorageID, memoryEntries);

    CacheEntryTable* entries;
    sGlobalEntryTables->Get(aContextKey, &entries);
    if (memoryEntries && entries)
      memoryEntries->EnumerateRead(&MemoryEntriesRemoval::EvictEntry, entries);
  }

  
  
  
  
  
  
  class Callback : public nsRunnable
  {
  public:
    Callback(nsICacheEntryDoomCallback* aCallback) : mCallback(aCallback) { }
    NS_IMETHODIMP Run()
    {
      mCallback->OnCacheEntryDoomed(NS_OK);
      return NS_OK;
    }
    nsCOMPtr<nsICacheEntryDoomCallback> mCallback;
  };

  if (aCallback) {
    nsRefPtr<nsRunnable> callback = new Callback(aCallback);
    return NS_DispatchToCurrentThread(callback);
  }

  return NS_OK;
}

nsresult
CacheStorageService::WalkStorageEntries(CacheStorage const* aStorage,
                                        bool aVisitEntries,
                                        nsICacheStorageVisitor* aVisitor)
{
  LOG(("CacheStorageService::WalkStorageEntries [cb=%p, visitentries=%d]", aVisitor, aVisitEntries));
  NS_ENSURE_FALSE(mShutdown, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG(aStorage);

  nsAutoCString contextKey;
  CacheFileUtils::AppendKeyPrefix(aStorage->LoadInfo(), contextKey);

  nsRefPtr<WalkRunnable> event = new WalkRunnable(
    contextKey, aVisitEntries, aStorage->WriteToDisk(), aVisitor);
  return Dispatch(event);
}

void
CacheStorageService::CacheFileDoomed(nsILoadContextInfo* aLoadContextInfo,
                                     const nsACString & aIdExtension,
                                     const nsACString & aURISpec)
{
  nsAutoCString contextKey;
  CacheFileUtils::AppendKeyPrefix(aLoadContextInfo, contextKey);

  nsAutoCString entryKey;
  CacheEntry::HashingKey(EmptyCString(), aIdExtension, aURISpec, entryKey);

  mozilla::MutexAutoLock lock(mLock);

  if (mShutdown)
    return;

  CacheEntryTable* entries;
  if (!sGlobalEntryTables->Get(contextKey, &entries))
    return;

  nsRefPtr<CacheEntry> entry;
  if (!entries->Get(entryKey, getter_AddRefs(entry)))
    return;

  if (!entry->IsFileDoomed())
    return;

  if (entry->IsReferenced())
    return;

  
  
  RemoveExactEntry(entries, entryKey, entry, false);
  entry->DoomAlreadyRemoved();
}



size_t
CacheStorageService::SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  CacheStorageService::Self()->Lock().AssertCurrentThreadOwns();

  size_t n = 0;
  
  n += Pool(true).mFrecencyArray.SizeOfExcludingThis(mallocSizeOf);
  n += Pool(true).mExpirationArray.SizeOfExcludingThis(mallocSizeOf);
  n += Pool(false).mFrecencyArray.SizeOfExcludingThis(mallocSizeOf);
  n += Pool(false).mExpirationArray.SizeOfExcludingThis(mallocSizeOf);
  
  if (sGlobalEntryTables) {
    n += sGlobalEntryTables->SizeOfIncludingThis(nullptr, mallocSizeOf);
  }

  return n;
}

size_t
CacheStorageService::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
}

namespace { 

class ReportStorageMemoryData
{
public:
  nsIMemoryReporterCallback *mHandleReport;
  nsISupports *mData;
};

size_t CollectEntryMemory(nsACString const & aKey,
                          nsRefPtr<mozilla::net::CacheEntry> const & aEntry,
                          mozilla::MallocSizeOf mallocSizeOf,
                          void * aClosure)
{
  CacheStorageService::Self()->Lock().AssertCurrentThreadOwns();

  CacheEntryTable* aTable = static_cast<CacheEntryTable*>(aClosure);

  size_t n = 0;
  n += aKey.SizeOfExcludingThisIfUnshared(mallocSizeOf);

  
  
  
  if (aTable->Type() == CacheEntryTable::MEMORY_ONLY || aEntry->IsUsingDiskLocked())
    n += aEntry->SizeOfIncludingThis(mallocSizeOf);

  return n;
}

PLDHashOperator ReportStorageMemory(const nsACString& aKey,
                                    CacheEntryTable* aTable,
                                    void* aClosure)
{
  CacheStorageService::Self()->Lock().AssertCurrentThreadOwns();

  size_t size = aTable->SizeOfIncludingThis(&CollectEntryMemory,
                                            CacheStorageService::MallocSizeOf,
                                            aTable);

  ReportStorageMemoryData& data = *static_cast<ReportStorageMemoryData*>(aClosure);
  data.mHandleReport->Callback(
    EmptyCString(),
    nsPrintfCString("explicit/network/cache2/%s-storage(%s)",
      aTable->Type() == CacheEntryTable::MEMORY_ONLY ? "memory" : "disk",
      aKey.BeginReading()),
    nsIMemoryReporter::KIND_HEAP,
    nsIMemoryReporter::UNITS_BYTES,
    size,
    NS_LITERAL_CSTRING("Memory used by the cache storage."),
    data.mData);

  return PL_DHASH_NEXT;
}

} 

NS_IMETHODIMP
CacheStorageService::CollectReports(nsIMemoryReporterCallback* aHandleReport, nsISupports* aData)
{
  nsresult rv;

  rv = MOZ_COLLECT_REPORT(
    "explicit/network/cache2/io", KIND_HEAP, UNITS_BYTES,
    CacheFileIOManager::SizeOfIncludingThis(MallocSizeOf),
    "Memory used by the cache IO manager.");
  if (NS_WARN_IF(NS_FAILED(rv)))
    return rv;

  rv = MOZ_COLLECT_REPORT(
    "explicit/network/cache2/index", KIND_HEAP, UNITS_BYTES,
    CacheIndex::SizeOfIncludingThis(MallocSizeOf),
    "Memory used by the cache index.");
  if (NS_WARN_IF(NS_FAILED(rv)))
    return rv;

  MutexAutoLock lock(mLock);

  
  rv = MOZ_COLLECT_REPORT(
    "explicit/network/cache2/service", KIND_HEAP, UNITS_BYTES,
    SizeOfIncludingThis(MallocSizeOf),
    "Memory used by the cache storage service.");
  if (NS_WARN_IF(NS_FAILED(rv)))
    return rv;

  
  
  
  
  
  
  
  
  
  
  
  ReportStorageMemoryData data;
  data.mHandleReport = aHandleReport;
  data.mData = aData;
  sGlobalEntryTables->EnumerateRead(&ReportStorageMemory, &data);

  return NS_OK;
}

} 
} 
