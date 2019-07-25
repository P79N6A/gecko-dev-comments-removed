







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabaseError.h"

#include "nsIIDBRequest.h"
#include "nsIVariant.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDatabaseRequest;
class IDBDatabaseRequest;

class IDBRequest : public nsDOMEventTargetHelper,
                   public nsIIDBRequest

{
  friend class IndexedDatabaseRequest;
  friend class IDBDatabaseRequest;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBRequest,
                                           nsDOMEventTargetHelper)

  nsresult SetResult(nsISupports* aResult);

protected:
  IDBRequest(nsISupports* aSource);
  ~IDBRequest();

protected:
  PRUint16 mReadyState;
  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<IDBDatabaseError> mError;
  nsCOMPtr<nsIWritableVariant> mResult;
  nsCOMPtr<nsISupports> mSource;
};

END_INDEXEDDB_NAMESPACE

#endif 
