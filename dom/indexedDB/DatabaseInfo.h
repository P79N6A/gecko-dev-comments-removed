





#ifndef mozilla_dom_indexeddb_databaseinfo_h__
#define mozilla_dom_indexeddb_databaseinfo_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozilla/dom/quota/PersistenceType.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "mozilla/dom/indexedDB/Key.h"
#include "mozilla/dom/indexedDB/KeyPath.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDBDatabaseChild;
struct ObjectStoreInfo;

typedef nsRefPtrHashtable<nsStringHashKey, ObjectStoreInfo>
        ObjectStoreInfoHash;

struct DatabaseInfoGuts
{
  typedef mozilla::dom::quota::PersistenceType PersistenceType;

  DatabaseInfoGuts()
  : nextObjectStoreId(1), nextIndexId(1)
  { }

  bool operator==(const DatabaseInfoGuts& aOther) const
  {
    return this->name == aOther.name &&
           this->group == aOther.group &&
           this->origin == aOther.origin &&
           this->version == aOther.version &&
           this->persistenceType == aOther.persistenceType &&
           this->nextObjectStoreId == aOther.nextObjectStoreId &&
           this->nextIndexId == aOther.nextIndexId;
  };

  
  nsString name;
  nsCString group;
  nsCString origin;
  uint64_t version;
  PersistenceType persistenceType;
  int64_t nextObjectStoreId;
  int64_t nextIndexId;
};

struct DatabaseInfo MOZ_FINAL : public DatabaseInfoGuts
{
  DatabaseInfo()
  : cloned(false)
  { }

private:
  
  ~DatabaseInfo();

public:
  static bool Get(const nsACString& aId,
                  DatabaseInfo** aInfo);

  static bool Put(DatabaseInfo* aInfo);

  static void Remove(const nsACString& aId);

  bool GetObjectStoreNames(nsTArray<nsString>& aNames);
  bool ContainsStoreName(const nsAString& aName);

  ObjectStoreInfo* GetObjectStore(const nsAString& aName);

  bool PutObjectStore(ObjectStoreInfo* aInfo);

  void RemoveObjectStore(const nsAString& aName);

  already_AddRefed<DatabaseInfo> Clone();

  nsCString id;
  nsString filePath;
  bool cloned;

  nsAutoPtr<ObjectStoreInfoHash> objectStoreHash;

  NS_INLINE_DECL_REFCOUNTING(DatabaseInfo)
};

struct IndexInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  IndexInfo();
  IndexInfo(const IndexInfo& aOther);
  ~IndexInfo();
#else
  IndexInfo()
  : id(INT64_MIN), keyPath(0), unique(false), multiEntry(false) { }
#endif

  bool operator==(const IndexInfo& aOther) const
  {
    return this->name == aOther.name &&
           this->id == aOther.id &&
           this->keyPath == aOther.keyPath &&
           this->unique == aOther.unique &&
           this->multiEntry == aOther.multiEntry;
  };

  
  nsString name;
  int64_t id;
  KeyPath keyPath;
  bool unique;
  bool multiEntry;
};

struct ObjectStoreInfoGuts
{
  ObjectStoreInfoGuts()
  : id(0), keyPath(0), autoIncrement(false)
  { }

  bool operator==(const ObjectStoreInfoGuts& aOther) const
  {
    return this->name == aOther.name &&
           this->id == aOther.id;
  };

  

  
  nsString name;
  int64_t id;
  KeyPath keyPath;
  bool autoIncrement;

  
  
  nsTArray<IndexInfo> indexes;
};

struct ObjectStoreInfo MOZ_FINAL : public ObjectStoreInfoGuts
{
#ifdef NS_BUILD_REFCNT_LOGGING
  ObjectStoreInfo();
#else
  ObjectStoreInfo()
  : nextAutoIncrementId(0), comittedAutoIncrementId(0) { }
#endif

  ObjectStoreInfo(ObjectStoreInfo& aOther);

private:
  
#ifdef NS_BUILD_REFCNT_LOGGING
  ~ObjectStoreInfo();
#else
  ~ObjectStoreInfo() {}
#endif
public:

  
  
  int64_t nextAutoIncrementId;
  int64_t comittedAutoIncrementId;

  
  
  
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ObjectStoreInfo)
};

struct IndexUpdateInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  IndexUpdateInfo();
  IndexUpdateInfo(const IndexUpdateInfo& aOther);
  ~IndexUpdateInfo();
#endif

  bool operator==(const IndexUpdateInfo& aOther) const
  {
    return this->indexId == aOther.indexId &&
           this->indexUnique == aOther.indexUnique &&
           this->value == aOther.value;
  };

  
  int64_t indexId;
  bool indexUnique;
  Key value;
};

END_INDEXEDDB_NAMESPACE

#endif 
