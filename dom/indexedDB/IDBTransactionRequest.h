






































#ifndef mozilla_dom_indexeddb_idbtransactionrequest_h__
#define mozilla_dom_indexeddb_idbtransactionrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBTransactionRequest.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "mozilla/Storage.h"

class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class ObjectStoreInfo;
class TransactionThreadPool;

class IDBTransactionRequest : public nsDOMEventTargetHelper,
                              public IDBRequest::Generator,
                              public nsIIDBTransactionRequest
{
  friend class AsyncConnectionHelper;
  friend class TransactionThreadPool;

public:
  NS_DECL_ISUPPORTS_INHERITED
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

  PRInt64 GetUniqueNumberForName()
  {
    return ++mLastUniqueNumber;
  }

  bool StartSavepoint(const nsCString& aName);
  void RevertToSavepoint(const nsCString& aName);

  already_AddRefed<mozIStorageStatement> AddStatement(bool aCreate,
                                                      bool aOverwrite,
                                                      bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> RemoveStatement(bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> GetStatement(bool aAutoIncrement);

  void CloseConnection();

private:
  IDBTransactionRequest();
  ~IDBTransactionRequest();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

  nsRefPtr<IDBDatabaseRequest> mDatabase;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mTimeout;
  PRUint32 mPendingRequests;

  PRInt64 mLastUniqueNumber;

  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;

  
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsAutoPtr<mozStorageTransaction> mDBTransaction;

  nsCOMPtr<mozIStorageStatement> mAddStmt;
  nsCOMPtr<mozIStorageStatement> mAddAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mModifyStmt;
  nsCOMPtr<mozIStorageStatement> mModifyAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mAddOrModifyStmt;
  nsCOMPtr<mozIStorageStatement> mAddOrModifyAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mGetStmt;
  nsCOMPtr<mozIStorageStatement> mGetAutoIncrementStmt;
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
