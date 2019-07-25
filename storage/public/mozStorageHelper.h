





































#ifndef MOZSTORAGEHELPER_H
#define MOZSTORAGEHELPER_H

#include "nsAutoPtr.h"

#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozStorage.h"

















class mozStorageTransaction
{
public:
  mozStorageTransaction(mozIStorageConnection* aConnection,
                        bool aCommitOnComplete,
                        PRInt32 aType = mozIStorageConnection::TRANSACTION_DEFERRED)
    : mConnection(aConnection),
      mHasTransaction(false),
      mCommitOnComplete(aCommitOnComplete),
      mCompleted(false)
  {
    
    if (mConnection)
      mHasTransaction = NS_SUCCEEDED(mConnection->BeginTransactionAs(aType));
  }
  ~mozStorageTransaction()
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
  nsCOMPtr<mozIStorageConnection> mConnection;
  bool mHasTransaction;
  bool mCommitOnComplete;
  bool mCompleted;
};









class NS_STACK_CLASS mozStorageStatementScoper
{
public:
  mozStorageStatementScoper(mozIStorageStatement* aStatement)
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
    mStatement = nsnull;
  }

protected:
  nsCOMPtr<mozIStorageStatement> mStatement;
};

#endif 
