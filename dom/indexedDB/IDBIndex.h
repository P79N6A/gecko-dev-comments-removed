






































#ifndef mozilla_dom_indexeddb_idbindex_h__
#define mozilla_dom_indexeddb_idbindex_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "nsIIDBIndex.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStore;
struct IndexInfo;

class IDBIndex : public IDBRequest::Generator,
                 public nsIIDBIndex
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBINDEX

  static already_AddRefed<IDBIndex>
  Create(IDBObjectStore* aObjectStore,
         const IndexInfo* aIndexInfo);

  IDBObjectStore* ObjectStore()
  {
    return mObjectStore;
  }

protected:
  IDBIndex();
  ~IDBIndex();

private:
  nsRefPtr<IDBObjectStore> mObjectStore;

  PRInt64 mId;
  nsString mName;
  nsString mKeyPath;
  bool mUnique;
  bool mAutoIncrement;
};

END_INDEXEDDB_NAMESPACE

#endif 
