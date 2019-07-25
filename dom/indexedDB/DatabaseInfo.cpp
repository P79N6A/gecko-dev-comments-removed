






































#include "DatabaseInfo.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE

namespace {

typedef nsClassHashtable<nsStringHashKey, ObjectStoreInfo>
        ObjectStoreInfoHash;

struct DatabaseInfoHash
{
  DatabaseInfoHash(DatabaseInfo* aInfo) {
    NS_ASSERTION(aInfo, "Null pointer!");
    info = aInfo;
  }

  nsAutoPtr<DatabaseInfo> info;
  nsAutoPtr<ObjectStoreInfoHash> objectStoreHash;
};

typedef nsClassHashtable<nsUint32HashKey, DatabaseInfoHash>
        DatabaseHash;

DatabaseHash* gDatabaseHash = nsnull;
bool gShutdown = false;

PLDHashOperator
EnumerateObjectStoreNames(const nsAString& aKey,
                          ObjectStoreInfo* aData,
                          void* aUserArg)
{
  nsTArray<nsString>* array = static_cast<nsTArray<nsString>*>(aUserArg);
  if (!array->AppendElement(aData->name)) {
    NS_ERROR("Out of memory?");
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

}


bool
DatabaseInfo::Get(PRUint32 aId,
                  DatabaseInfo** aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aId, "Bad id!");

  if (gDatabaseHash) {
    DatabaseInfoHash* hash;
    if (gDatabaseHash->Get(aId, &hash)) {
      if (aInfo) {
        *aInfo = hash->info;
      }
      return true;
    }
  }
  return false;
}


bool
DatabaseInfo::Put(DatabaseInfo* aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aInfo, "Null pointer!");

  NS_ENSURE_FALSE(gShutdown, false);

  if (!gDatabaseHash) {
    gDatabaseHash = new DatabaseHash();
    if (!gDatabaseHash->Init()) {
      NS_ERROR("Failed to initialize hashtable!");
      return false;
    }
  }

  if (gDatabaseHash->Get(aInfo->id, nsnull)) {
    NS_ERROR("Already know about this database!");
    return false;
  }

  nsAutoPtr<DatabaseInfoHash> hash(new DatabaseInfoHash(aInfo));
  if (!gDatabaseHash->Put(aInfo->id, hash)) {
    NS_ERROR("Put failed!");
    return false;
  }

  hash.forget();
  return true;
}


void
DatabaseInfo::Remove(PRUint32 aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(aId, nsnull), "Don't know anything about this one!");

  if (gDatabaseHash) {
    gDatabaseHash->Remove(aId);
  }
}

bool
DatabaseInfo::GetObjectStoreNames(nsTArray<nsString>& aNames)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(id, nsnull), "Don't know anything about this one!");

  if (!gDatabaseHash) {
    return false;
  }

  DatabaseInfoHash* info;
  if (!gDatabaseHash->Get(id, &info)) {
    return false;
  }

  aNames.Clear();
  if (info->objectStoreHash) {
    info->objectStoreHash->EnumerateRead(EnumerateObjectStoreNames, &aNames);
  }
  return true;
}


bool
DatabaseInfo::ContainsStoreName(const nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(id, nsnull), "Don't know anything about this one!");

  if (!gDatabaseHash) {
    return false;
  }

  DatabaseInfoHash* info;
  if (!gDatabaseHash->Get(id, &info)) {
    return false;
  }

  return info && info->objectStoreHash &&
         info->objectStoreHash->Get(aName, nsnull);
}


bool
ObjectStoreInfo::Get(PRUint32 aDatabaseId,
                     const nsAString& aName,
                     ObjectStoreInfo** aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aName.IsEmpty(), "Empty object store name!");

  if (gDatabaseHash) {
    DatabaseInfoHash* hash;
    if (gDatabaseHash->Get(aDatabaseId, &hash)) {
      if (hash->objectStoreHash) {
        return !!hash->objectStoreHash->Get(aName, aInfo);
      }
    }
  }

  return false;
}


bool
ObjectStoreInfo::Put(ObjectStoreInfo* aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aInfo, "Null pointer!");

  NS_ENSURE_FALSE(gShutdown, false);

  if (!gDatabaseHash) {
    NS_ERROR("No databases known!");
    return false;
  }

  DatabaseInfoHash* hash;
  if (!gDatabaseHash->Get(aInfo->databaseId, &hash)) {
    NS_ERROR("Don't know about this database!");
    return false;
  }

  if (!hash->objectStoreHash) {
    hash->objectStoreHash = new ObjectStoreInfoHash();
    if (!hash->objectStoreHash->Init()) {
      NS_ERROR("Failed to initialize hashtable!");
      return false;
    }
  }

  if (hash->objectStoreHash->Get(aInfo->name, nsnull)) {
    NS_ERROR("Already have an entry for this objectstore!");
    return false;
  }

  return !!hash->objectStoreHash->Put(aInfo->name, aInfo);
}


void
ObjectStoreInfo::Remove(PRUint32 aDatabaseId,
                        const nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(aDatabaseId, aName, nsnull), "Don't know about this one!");

  if (gDatabaseHash) {
    DatabaseInfoHash* hash;
    if (gDatabaseHash->Get(aDatabaseId, &hash) && hash->objectStoreHash) {
      hash->objectStoreHash->Remove(aName);
    }
  }
}

void
mozilla::dom::indexedDB::Shutdown()
{
  NS_ASSERTION(!gShutdown, "Shutdown called twice!");
  gShutdown = true;

  
  delete gDatabaseHash;
}
