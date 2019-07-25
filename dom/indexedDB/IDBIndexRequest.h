






































#ifndef mozilla_dom_indexeddb_idbindexrequest_h__
#define mozilla_dom_indexeddb_idbindexrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "nsIIDBIndexRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStore;
struct IndexInfo;

class IDBIndexRequest : public IDBRequest::Generator,
                        public nsIIDBIndexRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBINDEX
  NS_DECL_NSIIDBINDEXREQUEST

  static already_AddRefed<IDBIndexRequest>
  Create(IDBObjectStore* aObjectStore,
         const IndexInfo* aIndexInfo);

  IDBObjectStore* ObjectStore()
  {
    return mObjectStore;
  }

protected:
  IDBIndexRequest();
  ~IDBIndexRequest();

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
