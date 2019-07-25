






































#ifndef mozilla_dom_indexeddb_databaseinfo_h__
#define mozilla_dom_indexeddb_databaseinfo_h__


#include "IndexedDatabase.h"

#include "Key.h"
#include "IDBObjectStore.h"

#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

BEGIN_INDEXEDDB_NAMESPACE

struct ObjectStoreInfo;

typedef nsRefPtrHashtable<nsStringHashKey, ObjectStoreInfo>
        ObjectStoreInfoHash;

class IDBDatabase;
class OpenDatabaseHelper;

struct DatabaseInfo
{
  friend class IDBDatabase;
  friend class OpenDatabaseHelper;

private:
  DatabaseInfo()
  : nextObjectStoreId(1),
    nextIndexId(1),
    cloned(false)
  { }
  ~DatabaseInfo();

  static bool Get(nsIAtom* aId,
                  DatabaseInfo** aInfo);

  static bool Put(DatabaseInfo* aInfo);

public:
  static void Remove(nsIAtom* aId);

  static void RemoveAllForOrigin(const nsACString& aOrigin);

  bool GetObjectStoreNames(nsTArray<nsString>& aNames);
  bool ContainsStoreName(const nsAString& aName);

  ObjectStoreInfo* GetObjectStore(const nsAString& aName);

  bool PutObjectStore(ObjectStoreInfo* aInfo);

  void RemoveObjectStore(const nsAString& aName);

  already_AddRefed<DatabaseInfo> Clone();

  nsString name;
  nsCString origin;
  PRUint64 version;
  nsCOMPtr<nsIAtom> id;
  nsString filePath;
  PRInt64 nextObjectStoreId;
  PRInt64 nextIndexId;
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
  : id(LL_MININT), unique(false), multiEntry(false) { }
#endif

  PRInt64 id;
  nsString name;
  nsString keyPath;
  nsTArray<nsString> keyPathArray;
  bool unique;
  bool multiEntry;
};

struct ObjectStoreInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  ObjectStoreInfo();
#else
  ObjectStoreInfo()
  : id(0), nextAutoIncrementId(0), comittedAutoIncrementId(0) { }
#endif

  ObjectStoreInfo(ObjectStoreInfo& aOther);

private:
#ifdef NS_BUILD_REFCNT_LOGGING
  ~ObjectStoreInfo();
#else
  ~ObjectStoreInfo() {}
#endif
public:

  
  nsString name;
  PRInt64 id;
  nsString keyPath;
  nsTArray<nsString> keyPathArray;

  
  nsTArray<IndexInfo> indexes;

  
  
  PRInt64 nextAutoIncrementId;
  PRInt64 comittedAutoIncrementId;

  
  
  
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ObjectStoreInfo)
};

struct IndexUpdateInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  IndexUpdateInfo();
  ~IndexUpdateInfo();
#endif

  PRInt64 indexId;
  bool indexUnique;
  Key value;
};

END_INDEXEDDB_NAMESPACE

#endif
