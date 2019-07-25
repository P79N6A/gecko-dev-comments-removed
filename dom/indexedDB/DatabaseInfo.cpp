






































#include "DatabaseInfo.h"

#include "nsDataHashtable.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE

namespace {

typedef nsDataHashtable<nsISupportsHashKey, DatabaseInfo*>
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

DatabaseInfo::~DatabaseInfo()
{
  DatabaseInfo::Remove(id);
}

#ifdef NS_BUILD_REFCNT_LOGGING

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

  if (gDatabaseHash &&
      gDatabaseHash->Get(aId, aInfo)) {
    return true;
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

  if (!gDatabaseHash->Put(aInfo->id, aInfo)) {
    NS_ERROR("Put failed!");
    return false;
  }

  return true;
}


void
DatabaseInfo::Remove(nsIAtom* aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  DatabaseInfo* info = nsnull;

  DebugOnly<bool> got = Get(aId, &info);
  NS_ASSERTION(got && info, "Don't know anything about this one!");

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

  DatabaseInfo* info;
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

  DatabaseInfo* dbInfo;

  return gDatabaseHash &&
         gDatabaseHash->Get(id, &dbInfo) &&
         dbInfo->objectStoreHash &&
         dbInfo->objectStoreHash->Get(aName, nsnull);
}


bool
ObjectStoreInfo::Get(nsIAtom* aDatabaseId,
                     const nsAString& aName,
                     ObjectStoreInfo** aInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (gDatabaseHash) {
    DatabaseInfo* info;
    if (gDatabaseHash->Get(aDatabaseId, &info) &&
        info->objectStoreHash) {
      return !!info->objectStoreHash->Get(aName, aInfo);
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

  DatabaseInfo* info;
  if (!gDatabaseHash->Get(aInfo->databaseId, &info)) {
    NS_ERROR("Don't know about this database!");
    return false;
  }

  if (!info->objectStoreHash) {
    nsAutoPtr<ObjectStoreInfoHash> objectStoreHash(new ObjectStoreInfoHash());
    if (!objectStoreHash->Init()) {
      NS_ERROR("Failed to initialize hashtable!");
      return false;
    }
    info->objectStoreHash = objectStoreHash.forget();
  }

  if (info->objectStoreHash->Get(aInfo->name, nsnull)) {
    NS_ERROR("Already have an entry for this objectstore!");
    return false;
  }

  return !!info->objectStoreHash->Put(aInfo->name, aInfo);
}


void
ObjectStoreInfo::Remove(nsIAtom* aDatabaseId,
                        const nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(Get(aDatabaseId, aName, nsnull), "Don't know about this one!");

  if (gDatabaseHash) {
    DatabaseInfo* info;
    if (gDatabaseHash->Get(aDatabaseId, &info) && info->objectStoreHash) {
      info->objectStoreHash->Remove(aName);
    }
  }
}
