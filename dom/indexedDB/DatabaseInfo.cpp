






































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

  bool ok = !!hash->objectStoreHash->Put(aInfo->name, aInfo);
  if (ok && !hash->info->objectStoreNames.AppendElement(aInfo->name)) {
    NS_ERROR("Out of memory!");
  }
  return ok;
}


void
ObjectStoreInfo::Remove(PRUint32 aDatabaseId,
                        const nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(aDatabaseId, aName, nsnull), "Don't know about this one!");

  if (gDatabaseHash) {
    DatabaseInfoHash* hash;
    if (gDatabaseHash->Get(aDatabaseId, &hash)) {
      if (hash->objectStoreHash) {
        hash->objectStoreHash->Remove(aName);
      }
      hash->info->objectStoreNames.RemoveElement(aName);
    }
  }
}


void
ObjectStoreInfo::RemoveAllForDatabase(PRUint32 aDatabaseId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  DatabaseInfo::Remove(aDatabaseId);
}

void
mozilla::dom::indexedDB::Shutdown()
{
  NS_ASSERTION(!gShutdown, "Shutdown called twice!");
  gShutdown = true;

  
  delete gDatabaseHash;
}
