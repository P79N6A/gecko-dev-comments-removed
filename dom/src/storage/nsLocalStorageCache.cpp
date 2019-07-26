




#include "nsDOMStorage.h"

#include "nsLocalStorageCache.h"

using namespace mozilla;



#define MAX_IDLE_TIME (30) // seconds




namespace {

struct GetUsageEnumData
{
  const nsACString& mQuotaKey;
  int32_t mUsage;
};

PLDHashOperator
GetUsageEnum(const nsACString& aScopeName,
             nsScopeCache* aScopeCache,
             void* aParams)
{
  GetUsageEnumData* data = static_cast<GetUsageEnumData*>(aParams);
  if (StringBeginsWith(aScopeName, data->mQuotaKey)) {
    data->mUsage += aScopeCache->GetQuotaUsage();
  }
  return PL_DHASH_NEXT;
}

struct EvictEnumData
{
  nsTArray<nsCString>& mEvicted;
  nsTArray<int32_t>& mEvictedSize;
};

struct GetAllKeysEnumData
{
  nsTHashtable<nsSessionStorageEntry>& mKeys;
  DOMStorageImpl* mStorage;
};

PLDHashOperator
GetAllKeysEnum(const nsAString& aKey,
               nsScopeCache::KeyEntry* aEntry,
               void* aParams)
{
  GetAllKeysEnumData* data = static_cast<GetAllKeysEnumData*>(aParams);
  nsSessionStorageEntry* keyEntry = data->mKeys.PutEntry(aKey);
  keyEntry->mItem = new nsDOMStorageItem(data->mStorage,
                                         aKey,
                                         aEntry->mValue,
                                         aEntry->mIsSecure);
  return PL_DHASH_NEXT;
}

PLDHashOperator
MarkMatchingDeletedEnum(const nsACString& aScopeName,
                        nsAutoPtr<nsScopeCache>& aScopeCache,
                        void* aPattern)
{
  const nsACString* pattern = static_cast<const nsACString*>(aPattern);
  if (!StringBeginsWith(aScopeName, *pattern)) {
    return PL_DHASH_NEXT;
  }

  aScopeCache->DeleteScope();

  return PL_DHASH_NEXT;
}

PLDHashOperator
MarkKeysEnum(const nsAString& aKey,
             nsAutoPtr<nsScopeCache::KeyEntry>& aEntry,
             void* aDirtyState)
{
  aEntry->mIsDirty = *(static_cast<bool*>(aDirtyState));
  return PL_DHASH_NEXT;
}

PLDHashOperator
GetChangedKeysEnum(const nsAString& aKey,
                   nsAutoPtr<nsScopeCache::KeyEntry>& aEntry,
                   void* aParams)
{
  if (!aEntry->mIsDirty) {
    return PL_DHASH_NEXT;
  }

  nsLocalStorageCache::FlushData::ChangeSet* changeSet =
    static_cast<nsLocalStorageCache::FlushData::ChangeSet*>(aParams);

  changeSet->mDirtyKeys.AppendElement(aKey);
  changeSet->mDirtyValues.AppendElement(aEntry.get());

  return PL_DHASH_NEXT;
}

PLDHashOperator
GetEntrySize(const nsAString& aKey,
             nsScopeCache::KeyEntry* aEntry,
             void* aParam)
{
  int32_t* usage = static_cast<int32_t*>(aParam);
  *usage += aKey.Length() + aEntry->mValue.Length();
  return PL_DHASH_NEXT;
}

} 


nsLocalStorageCache::nsLocalStorageCache()
{
  mScopeCaches.Init(16);
}

nsScopeCache*
nsLocalStorageCache::GetScope(const nsACString& aScopeName)
{
  nsScopeCache* scopeCache = nullptr;
  if (mScopeCaches.Get(aScopeName, &scopeCache)) {
    scopeCache->mAccessTime = PR_IntervalNow();
  }
  return scopeCache;
}

void
nsLocalStorageCache::AddScope(const nsACString& aScopeName,
                              nsScopeCache* aScopeCache)
{
  aScopeCache->mAccessTime = PR_IntervalNow();
  mScopeCaches.Put(aScopeName, aScopeCache);
}

bool
nsLocalStorageCache::IsScopeCached(const nsACString& aScopeName) const
{
  return mScopeCaches.Get(aScopeName, nullptr);
}

uint32_t
nsLocalStorageCache::Count() const
{
  return mScopeCaches.Count();
}

int32_t
nsLocalStorageCache::GetQuotaUsage(const nsACString& aQuotaKey) const
{
  GetUsageEnumData data = { aQuotaKey , 0 };
  mScopeCaches.EnumerateRead(GetUsageEnum, &data);
  return data.mUsage;
}

void
nsLocalStorageCache::MarkMatchingScopesDeleted(const nsACString& aPattern)
{
  mScopeCaches.Enumerate(MarkMatchingDeletedEnum, (void*)&aPattern);
}

void
nsLocalStorageCache::ForgetAllScopes()
{
  mScopeCaches.Clear();
}


PLDHashOperator
nsLocalStorageCache::GetDirtyDataEnum(const nsACString& aScopeName,
                                      nsScopeCache* aScopeCache,
                                      void* aParams)
{
  if (!aScopeCache->mIsDirty) {
    return PL_DHASH_NEXT;
  }

  FlushData* flushData = static_cast<FlushData*>(aParams);
  FlushData::ChangeSet changeSet;

  changeSet.mWasDeleted = aScopeCache->mWasScopeDeleted;
  changeSet.mDeletedKeys = &aScopeCache->mDeletedKeys;

  aScopeCache->mTable.Enumerate(GetChangedKeysEnum, &changeSet);

  flushData->mScopeNames.AppendElement(aScopeName);
  flushData->mChanged.AppendElement(changeSet);

  return PL_DHASH_NEXT;
}

void
nsLocalStorageCache::GetFlushData(FlushData* aData) const
{
  mScopeCaches.EnumerateRead(GetDirtyDataEnum, aData);
}


PLDHashOperator
nsLocalStorageCache::SetFlushStateEnum(const nsACString& aScopeName,
                                       nsAutoPtr<nsScopeCache>& aScopeCache,
                                       void* aParams)
{
  FlushState* newState = static_cast<FlushState*>(aParams);
  if (*newState == FLUSH_PENDING && aScopeCache->mIsDirty) {
    
    bool isDirty = false;
    aScopeCache->mTable.Enumerate(MarkKeysEnum, &isDirty);
    aScopeCache->mDeletedKeys.Clear();
    aScopeCache->mWasScopeDeleted = false;
    aScopeCache->mIsDirty = false;
    aScopeCache->mIsFlushPending = true;
  } else if (*newState == FLUSHED && aScopeCache->mIsFlushPending) {
    aScopeCache->mIsFlushPending = false;
  } else if (*newState == FLUSH_FAILED && aScopeCache->mIsFlushPending) {
    
    
    
    
    
    
    
    bool isDirty = true;
    aScopeCache->mWasScopeDeleted = true;
    aScopeCache->mTable.Enumerate(MarkKeysEnum, &isDirty);
    aScopeCache->mIsFlushPending = false;
    aScopeCache->mIsDirty = true;
  }
  return PL_DHASH_NEXT;
}

void
nsLocalStorageCache::MarkScopesPending()
{
  FlushState newState = FLUSH_PENDING;
  mScopeCaches.Enumerate(SetFlushStateEnum, &newState);
}

void
nsLocalStorageCache::MarkScopesFlushed()
{
  FlushState newState = FLUSHED;
  mScopeCaches.Enumerate(SetFlushStateEnum, &newState);
}

void
nsLocalStorageCache::MarkFlushFailed()
{
  FlushState newState = FLUSH_FAILED;
  mScopeCaches.Enumerate(SetFlushStateEnum, &newState);
}


PLDHashOperator
nsLocalStorageCache::EvictEnum(const nsACString& aScopeName,
                               nsAutoPtr<nsScopeCache>& aScopeCache,
                               void* aParams)
{
  EvictEnumData* data = static_cast<EvictEnumData*>(aParams);

  
  if (aScopeCache->mIsDirty || aScopeCache->mIsFlushPending) {
    return PL_DHASH_NEXT;
  }

  static const PRIntervalTime kMaxIdleTime = PR_SecondsToInterval(MAX_IDLE_TIME);
  bool evict = (PR_IntervalNow() - aScopeCache->mAccessTime) > kMaxIdleTime;

  if (evict) {
    data->mEvicted.AppendElement(aScopeName);
    data->mEvictedSize.AppendElement(aScopeCache->GetQuotaUsage());
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

void
nsLocalStorageCache::EvictScopes(nsTArray<nsCString>& aEvicted,
                                 nsTArray<int32_t>& aEvictedSize)
{
  EvictEnumData data = { aEvicted, aEvictedSize };
  mScopeCaches.Enumerate(EvictEnum, &data);
}



nsScopeCache::nsScopeCache()
  : mWasScopeDeleted(false), mIsDirty(false), mIsFlushPending(false)
{
  mTable.Init();
}

nsresult
nsScopeCache::AddEntry(const nsAString& aKey,
                       const nsAString& aValue,
                       bool aSecure)
{
  KeyEntry* entry = new KeyEntry();
  entry->mValue = aValue;
  entry->mIsSecure = aSecure;
  entry->mIsDirty = false;

  mTable.Put(aKey, entry);

  return NS_OK;
}

nsresult
nsScopeCache::GetAllKeys(DOMStorageImpl* aStorage,
                         nsTHashtable<nsSessionStorageEntry>* aKeys) const
{
  GetAllKeysEnumData data = { *aKeys, aStorage };
  mTable.EnumerateRead(GetAllKeysEnum, &data);
  return NS_OK;
}

bool
nsScopeCache::GetKey(const nsAString& aKey,
                     nsAString& aValue,
                     bool* aSecure) const
{
  KeyEntry* entry = nullptr;
  if (mTable.Get(aKey, &entry)) {
    aValue = entry->mValue;
    *aSecure = entry->mIsSecure;
    return true;
  }

  return false;
}

nsresult
nsScopeCache::SetKey(const nsAString& aKey,
                     const nsAString& aValue,
                     bool aSecure)
{
  KeyEntry* entry = nullptr;
  if (!mTable.Get(aKey, &entry)) {
    entry = new KeyEntry();
    mTable.Put(aKey, entry);
  } else if (entry->mValue == aValue) {
    
    return NS_OK;
  }

  entry->mValue = aValue;
  entry->mIsSecure = aSecure;
  entry->mIsDirty = true;

  mDeletedKeys.RemoveElement(aKey);

  mIsDirty = true;

  return NS_OK;
}

void
nsScopeCache::SetSecure(const nsAString& aKey,
                        bool aSecure)
{
  KeyEntry* entry = nullptr;
  if (!mTable.Get(aKey, &entry)) {
    return;
  }

  entry->mIsSecure = aSecure;
  entry->mIsDirty = true;

  mIsDirty = true;
}

void
nsScopeCache::RemoveKey(const nsAString& aKey)
{
  KeyEntry* entry = nullptr;
  if (!mTable.Get(aKey, &entry)) {
    return;
  }

  nsAutoString key(aKey);
  if (!mWasScopeDeleted) {
    if (!mDeletedKeys.Contains(key)) {
      mDeletedKeys.AppendElement(key);
    }
  }
  mTable.Remove(key);

  mIsDirty = true;
}

void
nsScopeCache::DeleteScope()
{
  mTable.Clear();
  mDeletedKeys.Clear();
  mWasScopeDeleted = true;

  mIsDirty = true;
}

int32_t
nsScopeCache::GetQuotaUsage() const
{
  int32_t usage = 0;
  mTable.EnumerateRead(GetEntrySize, &usage);
  return usage;
}

