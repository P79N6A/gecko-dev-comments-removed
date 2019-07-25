






































#ifndef mozilla_dom_indexeddb_databaseinfo_h__
#define mozilla_dom_indexeddb_databaseinfo_h__


#include "IndexedDatabase.h"

#include "Key.h"
#include "IDBObjectStore.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"

BEGIN_INDEXEDDB_NAMESPACE

struct ObjectStoreInfo;

typedef nsClassHashtable<nsStringHashKey, ObjectStoreInfo>
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

  static void Remove(nsIAtom* aId);

public:
  bool GetObjectStoreNames(nsTArray<nsString>& aNames);
  bool ContainsStoreName(const nsAString& aName);

  bool GetObjectStore(const nsAString& aName,
                      ObjectStoreInfo** aInfo);

  bool PutObjectStore(ObjectStoreInfo* aInfo);

  void RemoveObjectStore(const nsAString& aName);

  already_AddRefed<DatabaseInfo> Clone();

  nsString name;
  PRUint64 version;
  nsIAtom* id;
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
  : id(LL_MININT), unique(false), autoIncrement(false) { }
#endif

  PRInt64 id;
  nsString name;
  nsString keyPath;
  bool unique;
  bool autoIncrement;
  bool multiEntry;
};

struct ObjectStoreInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  ObjectStoreInfo();
  ObjectStoreInfo(ObjectStoreInfo& aOther);
  ~ObjectStoreInfo();
#else
  ObjectStoreInfo()
  : id(0), autoIncrement(false), databaseId(0) { }
#endif

  nsString name;
  PRInt64 id;
  nsString keyPath;
  bool autoIncrement;
  nsIAtom* databaseId;
  nsTArray<IndexInfo> indexes;
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
