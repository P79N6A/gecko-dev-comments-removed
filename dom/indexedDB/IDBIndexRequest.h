






































#ifndef mozilla_dom_indexeddb_idbindexrequest_h__
#define mozilla_dom_indexeddb_idbindexrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "nsIIDBIndexRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStoreRequest;
struct IndexInfo;

class IDBIndexRequest : public IDBRequest::Generator,
                        public nsIIDBIndexRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBINDEX
  NS_DECL_NSIIDBINDEXREQUEST

  static already_AddRefed<IDBIndexRequest>
  Create(IDBObjectStoreRequest* aObjectStore,
         const IndexInfo* aIndexInfo);

protected:
  IDBIndexRequest();
  ~IDBIndexRequest();

private:
  nsRefPtr<IDBObjectStoreRequest> mObjectStore;

  nsString mName;
  nsString mKeyPath;
  bool mUnique;
  bool mAutoIncrement;
};

END_INDEXEDDB_NAMESPACE

#endif 
