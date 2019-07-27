




#ifndef MOZSTORAGEHELPER_H
#define MOZSTORAGEHELPER_H

#include "nsAutoPtr.h"

#include "mozIStorageAsyncConnection.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsError.h"




















template<typename T, typename U>
class mozStorageTransactionBase
{
public:
  mozStorageTransactionBase(T* aConnection,
                            bool aCommitOnComplete,
                            int32_t aType = mozIStorageConnection::TRANSACTION_DEFERRED)
    : mConnection(aConnection),
      mHasTransaction(false),
      mCommitOnComplete(aCommitOnComplete),
      mCompleted(false)
  {
    
    if (mConnection)
      mHasTransaction = NS_SUCCEEDED(mConnection->BeginTransactionAs(aType));
  }
  ~mozStorageTransactionBase()
  {
    if (mConnection && mHasTransaction && ! mCompleted) {
      if (mCommitOnComplete)
        mConnection->CommitTransaction();
      else
        mConnection->RollbackTransaction();
    }
  }

  




  nsresult Commit()
  {
    if (!mConnection || mCompleted)
      return NS_OK; 
    mCompleted = true;
    if (! mHasTransaction)
      return NS_OK; 
    nsresult rv = mConnection->CommitTransaction();
    if (NS_SUCCEEDED(rv))
      mHasTransaction = false;

    return rv;
  }

  




  nsresult Rollback()
  {
    if (!mConnection || mCompleted)
      return NS_OK; 
    mCompleted = true;
    if (! mHasTransaction)
      return NS_ERROR_FAILURE;

    
    nsresult rv = NS_OK;
    do {
      rv = mConnection->RollbackTransaction();
      if (rv == NS_ERROR_STORAGE_BUSY)
        (void)PR_Sleep(PR_INTERVAL_NO_WAIT);
    } while (rv == NS_ERROR_STORAGE_BUSY);

    if (NS_SUCCEEDED(rv))
      mHasTransaction = false;

    return rv;
  }

  




  bool HasTransaction()
  {
    return mHasTransaction;
  }

  



  void SetDefaultAction(bool aCommitOnComplete)
  {
    mCommitOnComplete = aCommitOnComplete;
  }

protected:
  U mConnection;
  bool mHasTransaction;
  bool mCommitOnComplete;
  bool mCompleted;
};





typedef mozStorageTransactionBase<mozIStorageConnection,
                                  nsCOMPtr<mozIStorageConnection> >
mozStorageTransaction;










class MOZ_STACK_CLASS mozStorageStatementScoper
{
public:
  explicit mozStorageStatementScoper(mozIStorageStatement* aStatement)
      : mStatement(aStatement)
  {
  }
  ~mozStorageStatementScoper()
  {
    if (mStatement)
      mStatement->Reset();
  }

  



  void Abandon()
  {
    mStatement = nullptr;
  }

protected:
  nsCOMPtr<mozIStorageStatement> mStatement;
};




#define MOZ_STORAGE_UNIQUIFY_QUERY_STR "/* " __FILE__ " */ "

#endif 
