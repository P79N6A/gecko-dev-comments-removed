






































#ifndef mozilla_dom_indexeddb_idbindex_h__
#define mozilla_dom_indexeddb_idbindex_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBIndex.h"

#include "nsDOMEventTargetHelper.h"

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class IDBObjectStore;
struct IndexInfo;

class IDBIndex : public nsDOMEventTargetHelper,
                 public nsIIDBIndex
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBINDEX

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBIndex,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBIndex>
  Create(IDBObjectStore* aObjectStore,
         const IndexInfo* aIndexInfo);

  IDBObjectStore* ObjectStore()
  {
    return mObjectStore;
  }

  const PRInt64 Id() const
  {
    return mId;
  }

  const nsString& Name() const
  {
    return mName;
  }

  bool IsUnique() const
  {
    return mUnique;
  }

  bool IsAutoIncrement() const
  {
    return mAutoIncrement;
  }

  const nsString& KeyPath() const
  {
    return mName;
  }

private:
  IDBIndex();
  ~IDBIndex();

  nsRefPtr<IDBObjectStore> mObjectStore;

  PRInt64 mId;
  nsString mName;
  nsString mKeyPath;
  bool mUnique;
  bool mAutoIncrement;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
