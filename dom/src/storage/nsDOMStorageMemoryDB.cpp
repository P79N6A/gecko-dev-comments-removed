






































#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsDOMStorage.h"
#include "nsDOMStorageMemoryDB.h"
#include "nsNetUtil.h"

nsresult
nsDOMStorageMemoryDB::Init(nsDOMStoragePersistentDB* aPreloadDB)
{
  if (!mData.Init(20))
    return NS_ERROR_OUT_OF_MEMORY;

  mPreloadDB = aPreloadDB;
  return NS_OK;
}

static PLDHashOperator
AllKeyEnum(nsSessionStorageEntry* aEntry, void* userArg)
{
  nsDOMStorageMemoryDB::nsStorageItemsTable *target =
      (nsDOMStorageMemoryDB::nsStorageItemsTable *)userArg;

  nsDOMStorageMemoryDB::nsInMemoryItem* item =
      new nsDOMStorageMemoryDB::nsInMemoryItem();
  if (!item)
    return PL_DHASH_STOP;

  aEntry->mItem->GetValue(item->mValue);
  nsresult rv = aEntry->mItem->GetSecure(&item->mSecure);
  if (NS_FAILED(rv))
    item->mSecure = PR_FALSE;

  target->Put(aEntry->GetKey(), item);
  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorageMemoryDB::GetItemsTable(nsDOMStorage* aStorage,
                                    nsInMemoryStorage** aMemoryStorage)
{
  if (mData.Get(aStorage->GetScopeDBKey(), aMemoryStorage))
    return NS_OK;

  *aMemoryStorage = nsnull;

  nsInMemoryStorage* storageData = new nsInMemoryStorage();
  if (!storageData)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!storageData->mTable.Init()) {
    delete storageData;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (mPreloadDB) {
    nsresult rv;

    nsTHashtable<nsSessionStorageEntry> keys;
    keys.Init();

    rv = mPreloadDB->GetAllKeys(aStorage, &keys);
    NS_ENSURE_SUCCESS(rv, rv);

    mPreloading = PR_TRUE;
    keys.EnumerateEntries(AllKeyEnum, &storageData->mTable);
    mPreloading = PR_FALSE;
  }

  mData.Put(aStorage->GetScopeDBKey(), storageData);
  *aMemoryStorage = storageData;

  return NS_OK;
}

struct GetAllKeysEnumStruc
{
  nsTHashtable<nsSessionStorageEntry>* mTarget;
  nsDOMStorage* mStorage;
};

static PLDHashOperator
GetAllKeysEnum(const nsAString& keyname,
               nsDOMStorageMemoryDB::nsInMemoryItem* item,
               void *closure)
{
  GetAllKeysEnumStruc* struc = (GetAllKeysEnumStruc*)closure;

  nsSessionStorageEntry* entry = struc->mTarget->PutEntry(keyname);
  if (!entry)
    return PL_DHASH_STOP;

  entry->mItem = new nsDOMStorageItem(struc->mStorage,
                                      keyname,
                                      EmptyString(),
                                      item->mSecure);
  if (!entry->mItem)
    return PL_DHASH_STOP;

  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorageMemoryDB::GetAllKeys(nsDOMStorage* aStorage,
                                 nsTHashtable<nsSessionStorageEntry>* aKeys)
{
  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  GetAllKeysEnumStruc struc;
  struc.mTarget = aKeys;
  struc.mStorage = aStorage;
  storage->mTable.EnumerateRead(GetAllKeysEnum, &struc);

  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::GetKeyValue(nsDOMStorage* aStorage,
                                  const nsAString& aKey,
                                  nsAString& aValue,
                                  PRBool* aSecure)
{
  if (mPreloading) {
    NS_PRECONDITION(mPreloadDB, "Must have a preload DB set when preloading");
    return mPreloadDB->GetKeyValue(aStorage, aKey, aValue, aSecure);
  }

  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsInMemoryItem* item;
  if (!storage->mTable.Get(aKey, &item))
    return NS_ERROR_DOM_NOT_FOUND_ERR;

  aValue = item->mValue;
  *aSecure = item->mSecure;
  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::SetKey(nsDOMStorage* aStorage,
                             const nsAString& aKey,
                             const nsAString& aValue,
                             PRBool aSecure,
                             PRInt32 aQuota,
                             PRBool aExcludeOfflineFromUsage,
                             PRInt32 *aNewUsage)
{
  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 usage = 0;
  if (!aStorage->GetQuotaDomainDBKey(!aExcludeOfflineFromUsage).IsEmpty()) {
    rv = GetUsage(aStorage, aExcludeOfflineFromUsage, &usage);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  usage += aKey.Length() + aValue.Length();

  nsInMemoryItem* item;
  if (!storage->mTable.Get(aKey, &item)) {
    if (usage > aQuota) {
      return NS_ERROR_DOM_QUOTA_REACHED;
    }

    item = new nsInMemoryItem();
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;

    storage->mTable.Put(aKey, item);
    storage->mUsageDelta += aKey.Length();
  }
  else
  {
    usage -= aKey.Length() + item->mValue.Length();
    if (usage > aQuota) {
      return NS_ERROR_DOM_QUOTA_REACHED;
    }
  }

  storage->mUsageDelta += aValue.Length() - item->mValue.Length();

  item->mValue = aValue;
  item->mSecure = aSecure;

  *aNewUsage = usage;

  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::SetSecure(nsDOMStorage* aStorage,
                                const nsAString& aKey,
                                const PRBool aSecure)
{
  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsInMemoryItem* item;
  if (!storage->mTable.Get(aKey, &item))
    return NS_ERROR_DOM_NOT_FOUND_ERR;

  item->mSecure = aSecure;

  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::RemoveKey(nsDOMStorage* aStorage,
                                const nsAString& aKey,
                                PRBool aExcludeOfflineFromUsage,
                                PRInt32 aKeyUsage)
{
  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsInMemoryItem* item;
  if (!storage->mTable.Get(aKey, &item))
    return NS_ERROR_DOM_NOT_FOUND_ERR;

  storage->mUsageDelta -= aKey.Length() + item->mValue.Length();
  storage->mTable.Remove(aKey);

  return NS_OK;
}

static PLDHashOperator
RemoveAllKeysEnum(const nsAString& keyname,
                  nsAutoPtr<nsDOMStorageMemoryDB::nsInMemoryItem>& item,
                  void *closure)
{
  nsDOMStorageMemoryDB::nsInMemoryStorage* storage =
      (nsDOMStorageMemoryDB::nsInMemoryStorage*)closure;

  storage->mUsageDelta -= keyname.Length() + item->mValue.Length();
  return PL_DHASH_REMOVE;
}

nsresult
nsDOMStorageMemoryDB::ClearStorage(nsDOMStorage* aStorage)
{
  nsresult rv;

  nsInMemoryStorage* storage;
  rv = GetItemsTable(aStorage, &storage);
  NS_ENSURE_SUCCESS(rv, rv);

  storage->mTable.Enumerate(RemoveAllKeysEnum, storage);
  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::DropStorage(nsDOMStorage* aStorage)
{
  mData.Remove(aStorage->GetScopeDBKey());
  return NS_OK;
}

struct RemoveOwnersStruc
{
  nsCString* mSubDomain;
  PRBool mMatch;
};

static PLDHashOperator
RemoveOwnersEnum(const nsACString& key,
                 nsAutoPtr<nsDOMStorageMemoryDB::nsInMemoryStorage>& storage,
                 void *closure)
{
  RemoveOwnersStruc* struc = (RemoveOwnersStruc*)closure;

  if (StringBeginsWith(key, *(struc->mSubDomain)) == struc->mMatch)
    return PL_DHASH_REMOVE;

  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorageMemoryDB::RemoveOwner(const nsACString& aOwner,
                                  PRBool aIncludeSubDomains)
{
  nsCAutoString subdomainsDBKey;
  nsDOMStorageDBWrapper::CreateDomainScopeDBKey(aOwner, subdomainsDBKey);

  if (!aIncludeSubDomains)
    subdomainsDBKey.AppendLiteral(":");

  RemoveOwnersStruc struc;
  struc.mSubDomain = &subdomainsDBKey;
  struc.mMatch = PR_TRUE;
  mData.Enumerate(RemoveOwnersEnum, &struc);

  return NS_OK;
}


nsresult
nsDOMStorageMemoryDB::RemoveOwners(const nsTArray<nsString> &aOwners,
                                   PRBool aIncludeSubDomains,
                                   PRBool aMatch)
{
  if (aOwners.Length() == 0) {
    if (aMatch) {
      return NS_OK;
    }

    return RemoveAll();
  }

  for (PRUint32 i = 0; i < aOwners.Length(); i++) {
    nsCAutoString quotaKey;
    nsresult rv;
    rv = nsDOMStorageDBWrapper::CreateDomainScopeDBKey(
      NS_ConvertUTF16toUTF8(aOwners[i]), quotaKey);

    if (!aIncludeSubDomains)
      quotaKey.AppendLiteral(":");

    RemoveOwnersStruc struc;
    struc.mSubDomain = &quotaKey;
    struc.mMatch = aMatch;
    mData.Enumerate(RemoveOwnersEnum, &struc);
  }

  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::RemoveAll()
{
  mData.Clear(); 
  return NS_OK;
}

nsresult
nsDOMStorageMemoryDB::GetUsage(nsDOMStorage* aStorage,
                               PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage)
{
  return GetUsageInternal(aStorage->GetQuotaDomainDBKey(!aExcludeOfflineFromUsage),
                          aExcludeOfflineFromUsage, aUsage);
}

nsresult
nsDOMStorageMemoryDB::GetUsage(const nsACString& aDomain,
                               PRBool aIncludeSubDomains,
                               PRInt32 *aUsage)
{
  nsresult rv;

  nsCAutoString quotadomainDBKey;
  rv = nsDOMStorageDBWrapper::CreateQuotaDomainDBKey(aDomain,
                                                     aIncludeSubDomains,
                                                     PR_FALSE,
                                                     quotadomainDBKey);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetUsageInternal(quotadomainDBKey, PR_FALSE, aUsage);
}

struct GetUsageEnumStruc
{
  PRInt32 mUsage;
  PRInt32 mExcludeOfflineFromUsage;
  nsCString mSubdomain;
};

static PLDHashOperator
GetUsageEnum(const nsACString& key,
             nsDOMStorageMemoryDB::nsInMemoryStorage* storageData,
             void *closure)
{
  GetUsageEnumStruc* struc = (GetUsageEnumStruc*)closure;

  if (StringBeginsWith(key, struc->mSubdomain)) {
    if (struc->mExcludeOfflineFromUsage) {
      nsCAutoString domain;
      nsresult rv = nsDOMStorageDBWrapper::GetDomainFromScopeKey(key, domain);
      if (NS_SUCCEEDED(rv) && IsOfflineAllowed(domain))
        return PL_DHASH_NEXT;
    }

    struc->mUsage += storageData->mUsageDelta;
  }

  return PL_DHASH_NEXT;
}

nsresult
nsDOMStorageMemoryDB::GetUsageInternal(const nsACString& aQuotaDomainDBKey,
                                       PRBool aExcludeOfflineFromUsage,
                                       PRInt32 *aUsage)
{
  GetUsageEnumStruc struc;
  struc.mUsage = 0;
  struc.mExcludeOfflineFromUsage = aExcludeOfflineFromUsage;
  struc.mSubdomain = aQuotaDomainDBKey;

  if (mPreloadDB) {
    nsresult rv;

    rv = mPreloadDB->GetUsageInternal(aQuotaDomainDBKey,
                                      aExcludeOfflineFromUsage, &struc.mUsage);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mData.EnumerateRead(GetUsageEnum, &struc);

  *aUsage = struc.mUsage;
  return NS_OK;
}
