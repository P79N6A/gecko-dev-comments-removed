






































#ifndef mozilla_dom_indexeddb_databaseinfo_h__
#define mozilla_dom_indexeddb_databaseinfo_h__


#include "IndexedDatabase.h"

BEGIN_INDEXEDDB_NAMESPACE

struct DatabaseInfo
{
  nsString name;
  nsString description;
  nsString version;
  PRUint32 id;
  nsString filePath;

  nsAutoRefCnt referenceCount;

  DatabaseInfo()
  : id(0) { }

  static bool Get(PRUint32 aId,
                  DatabaseInfo** aInfo);

  static bool Put(DatabaseInfo* aInfo);

  static void Remove(PRUint32 aId);

  bool GetObjectStoreNames(nsTArray<nsString>& aNames);
  bool ContainsStoreName(const nsAString& aName);
};

struct ObjectStoreInfo
{
  nsString name;
  PRInt64 id;
  nsString keyPath;
  bool autoIncrement;
  PRUint32 databaseId;
  nsTArray<nsString> indexNames;

  ObjectStoreInfo()
  : id(0), autoIncrement(false), databaseId(0) { }

  static bool Get(PRUint32 aDatabaseId,
                  const nsAString& aName,
                  ObjectStoreInfo** aInfo);

  static bool Put(ObjectStoreInfo* aInfo);

  static void Remove(PRUint32 aDatabaseId,
                     const nsAString& aName);
};

END_INDEXEDDB_NAMESPACE

#endif 

