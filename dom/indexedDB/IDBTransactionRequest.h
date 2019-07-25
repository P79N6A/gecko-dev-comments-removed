






































#ifndef mozilla_dom_indexeddb_idbtransactionrequest_h__
#define mozilla_dom_indexeddb_idbtransactionrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBTransactionRequest.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBTransactionRequest : public nsDOMEventTargetHelper,
                              public IDBRequest::Generator,
                              public nsIIDBTransactionRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBTRANSACTION
  NS_DECL_NSIIDBTRANSACTIONREQUEST

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBTransactionRequest,
                                           nsDOMEventTargetHelper)

  already_AddRefed<IDBTransactionRequest>
  Create();

private:
  nsRefPtr<IDBDatabaseRequest> mDatabase;

  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
