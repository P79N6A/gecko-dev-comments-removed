






































#ifndef mozilla_dom_indexeddb_idbtransactionrequest_h__
#define mozilla_dom_indexeddb_idbtransactionrequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBTransactionRequest.h"
#include "nsIRunnable.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "mozilla/Storage.h"

class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class CommitHelper;
class ObjectStoreInfo;
class TransactionThreadPool;

class IDBTransactionRequest : public nsDOMEventTargetHelper,
                              public IDBRequest::Generator,
                              public nsIIDBTransactionRequest
{
  friend class AsyncConnectionHelper;
  friend class CommitHelper;
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

  bool StartSavepoint();
  nsresult ReleaseSavepoint();
  void RollbackSavepoint();

  already_AddRefed<mozIStorageStatement> AddStatement(bool aCreate,
                                                      bool aOverwrite,
                                                      bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> RemoveStatement(bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> GetStatement(bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> IndexGetStatement(bool aUnique,
                                                           bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> IndexGetObjectStatement(bool aUnique,
                                                                 bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement> IndexRemoveStatement(bool aUnique,
                                                              bool aAutoIncrement);

#ifdef DEBUG
  bool TransactionIsOpen() const;
  bool IsWriteAllowed() const;
#else
  bool TransactionIsOpen() const
  {
    return mReadyState == nsIIDBTransaction::INITIAL ||
           mReadyState == nsIIDBTransaction::LOADING;
  }

  bool IsWriteAllowed() const
  {
    return mMode == nsIIDBTransaction::READ_WRITE;
  }
#endif

private:
  IDBTransactionRequest();
  ~IDBTransactionRequest();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

  void CloseConnection();

  nsresult CommitOrRollback();

  nsRefPtr<IDBDatabaseRequest> mDatabase;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mTimeout;
  PRUint32 mPendingRequests;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;

  
  nsCOMPtr<mozIStorageConnection> mConnection;
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
  nsCOMPtr<mozIStorageStatement> mIndexGetUniqueAIStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetAIStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetUniqueStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetObjectUniqueAIStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetObjectAIStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetObjectUniqueStmt;
  nsCOMPtr<mozIStorageStatement> mIndexGetObjectStmt;

  
  PRUint32 mSavepointCount;

  bool mHasInitialSavepoint;
  bool mAborted;
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

class CloseConnectionRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  template<class T>
  bool AddDoomedObject(nsCOMPtr<T>& aCOMPtr)
  {
    if (aCOMPtr) {
      if (!mDoomedObjects.AppendElement(do_QueryInterface(aCOMPtr))) {
        NS_ERROR("Out of memory!");
        return false;
      }
      aCOMPtr = nsnull;
    }
    return true;
  }

private:
  nsAutoTArray<nsCOMPtr<nsISupports>, 10> mDoomedObjects;
};

END_INDEXEDDB_NAMESPACE

#endif 
