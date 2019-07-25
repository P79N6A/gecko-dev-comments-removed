






































#ifndef mozilla_dom_indexeddb_idbtransactionrequest_h__
#define mozilla_dom_indexeddb_idbtransactionrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBTransactionRequest.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "mozilla/Storage.h"

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class ObjectStoreInfo;

class IDBTransactionRequest : public nsDOMEventTargetHelper,
                              public IDBRequest::Generator,
                              public nsIIDBTransactionRequest
{
  friend class AsyncConnectionHelper;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBTRANSACTION
  NS_DECL_NSIIDBTRANSACTIONREQUEST

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBTransactionRequest,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBTransactionRequest>
  Create(IDBDatabaseRequest* aDatabase,
         nsTArray<nsString>& aObjectStoreNames,
         PRUint16 aMode,
         PRUint32 aTimeout);

  void OnNewRequest();
  void OnRequestFinished();

  nsresult Commit();

  PRInt64 GetUniqueNumberForName() { return ++mLastUniqueNumber; }
  bool StartSavepoint(const nsCString& aName);
  void RevertToSavepoint(const nsCString& aName);

private:
  IDBTransactionRequest();
  ~IDBTransactionRequest();

  nsRefPtr<IDBDatabaseRequest> mDatabase;
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mTimeout;
  PRUint32 mPendingRequests;

  PRInt64 mLastUniqueNumber;

  nsAutoPtr<mozStorageTransaction> mDBTransaction;

  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;
};

NS_STACK_CLASS
class AutoTransactionRequestNotifier
{
public:
  AutoTransactionRequestNotifier(IDBTransactionRequest* aTransaction)
  : mTransaction(aTransaction)
  {
    NS_ASSERTION(mTransaction, "Null pointer!");
    mTransaction->OnNewRequest();
  }

  ~AutoTransactionRequestNotifier()
  {
    mTransaction->OnRequestFinished();
  }

private:
  nsRefPtr<IDBTransactionRequest> mTransaction;
};

END_INDEXEDDB_NAMESPACE

#endif 
