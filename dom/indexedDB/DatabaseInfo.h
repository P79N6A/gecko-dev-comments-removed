






































#ifndef mozilla_dom_indexeddb_databaseinfo_h__
#define mozilla_dom_indexeddb_databaseinfo_h__


#include "IndexedDatabase.h"

#include "IDBObjectStore.h"

BEGIN_INDEXEDDB_NAMESPACE

struct DatabaseInfo
{
  nsString name;
  nsString description;
  nsString version;
  PRUint32 id;
  nsString filePath;

  nsAutoRefCnt referenceCount;

#ifdef NS_BUILD_REFCNT_LOGGING
  DatabaseInfo();
  ~DatabaseInfo();
#else
  DatabaseInfo()
  : id(0) { }
#endif

  static bool Get(PRUint32 aId,
                  DatabaseInfo** aInfo);

  static bool Put(DatabaseInfo* aInfo);

  static void Remove(PRUint32 aId);

  bool GetObjectStoreNames(nsTArray<nsString>& aNames);
  bool ContainsStoreName(const nsAString& aName);
};

struct IndexInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  IndexInfo();
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
};

struct ObjectStoreInfo
{
  nsString name;
  PRInt64 id;
  nsString keyPath;
  bool autoIncrement;
  PRUint32 databaseId;
  nsTArray<IndexInfo> indexes;

#ifdef NS_BUILD_REFCNT_LOGGING
  ObjectStoreInfo();
  ~ObjectStoreInfo();
#else
  ObjectStoreInfo()
  : id(0), autoIncrement(false), databaseId(0) { }
#endif

  static bool Get(PRUint32 aDatabaseId,
                  const nsAString& aName,
                  ObjectStoreInfo** aInfo);

  static bool Put(ObjectStoreInfo* aInfo);

  static void Remove(PRUint32 aDatabaseId,
                     const nsAString& aName);
};

struct IndexUpdateInfo
{
#ifdef NS_BUILD_REFCNT_LOGGING
  IndexUpdateInfo();
  ~IndexUpdateInfo();
#endif

  IndexInfo info;
  Key value;
};

END_INDEXEDDB_NAMESPACE

#endif 

