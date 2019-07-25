






































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

typedef nsClassHashtable<nsISupportsHashKey, DatabaseInfoHash>
        DatabaseHash;

DatabaseHash* gDatabaseHash = nsnull;

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

#ifdef NS_BUILD_REFCNT_LOGGING
DatabaseInfo::DatabaseInfo()
: nextObjectStoreId(1),
  nextIndexId(1),
  runningVersionChange(false)
{
  MOZ_COUNT_CTOR(DatabaseInfo);
}

DatabaseInfo::~DatabaseInfo()
{
  MOZ_COUNT_DTOR(DatabaseInfo);
}

IndexInfo::IndexInfo()
: id(LL_MININT),
  unique(false),
  autoIncrement(false)
{
  MOZ_COUNT_CTOR(IndexInfo);
}

IndexInfo::~IndexInfo()
{
  MOZ_COUNT_DTOR(IndexInfo);
}

ObjectStoreInfo::ObjectStoreInfo()
: id(0),
  autoIncrement(false),
  databaseId(0)
{
  MOZ_COUNT_CTOR(ObjectStoreInfo);
}

ObjectStoreInfo::~ObjectStoreInfo()
{
  MOZ_COUNT_DTOR(ObjectStoreInfo);
}

IndexUpdateInfo::IndexUpdateInfo()
{
  MOZ_COUNT_CTOR(IndexUpdateInfo);
}

IndexUpdateInfo::~IndexUpdateInfo()
{
  MOZ_COUNT_DTOR(IndexUpdateInfo);
}
#endif 


bool
DatabaseInfo::Get(nsIAtom* aId,
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

  if (!gDatabaseHash) {
    nsAutoPtr<DatabaseHash> databaseHash(new DatabaseHash());
    if (!databaseHash->Init()) {
      NS_ERROR("Failed to initialize hashtable!");
      return false;
    }

    gDatabaseHash = databaseHash.forget();
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
DatabaseInfo::Remove(nsIAtom* aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(aId, nsnull), "Don't know anything about this one!");

  if (gDatabaseHash) {
    gDatabaseHash->Remove(aId);

    if (!gDatabaseHash->Count()) {
      delete gDatabaseHash;
      gDatabaseHash = nsnull;
    }
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

  DatabaseInfoHash* hash;
  ObjectStoreInfo* info;

  return gDatabaseHash &&
         gDatabaseHash->Get(id, &hash) &&
         hash->objectStoreHash &&
         hash->objectStoreHash->Get(aName, &info);
}


bool
ObjectStoreInfo::Get(nsIAtom* aDatabaseId,
                     const nsAString& aName,
                     ObjectStoreInfo** aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

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
    nsAutoPtr<ObjectStoreInfoHash> objectStoreHash(new ObjectStoreInfoHash());
    if (!objectStoreHash->Init()) {
      NS_ERROR("Failed to initialize hashtable!");
      return false;
    }
    hash->objectStoreHash = objectStoreHash.forget();
  }

  if (hash->objectStoreHash->Get(aInfo->name, nsnull)) {
    NS_ERROR("Already have an entry for this objectstore!");
    return false;
  }

  return !!hash->objectStoreHash->Put(aInfo->name, aInfo);
}


void
ObjectStoreInfo::Remove(nsIAtom* aDatabaseId,
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
