





































#ifndef _MOZSTORAGEHELPER_H_
#define _MOZSTORAGEHELPER_H_

#include "nsAutoPtr.h"

#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozStorage.h"

















class mozStorageTransaction
{
public:
  mozStorageTransaction(mozIStorageConnection* aConnection,
                        PRBool aCommitOnComplete,
                        PRInt32 aType = mozIStorageConnection::TRANSACTION_DEFERRED)
    : mConnection(aConnection),
      mHasTransaction(PR_FALSE),
      mCommitOnComplete(aCommitOnComplete),
      mCompleted(PR_FALSE)
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
    mCompleted = PR_TRUE;
    if (! mHasTransaction)
      return NS_OK; 
    return mConnection->CommitTransaction();
  }

  




  nsresult Rollback()
  {
    if (!mConnection || mCompleted)
      return NS_OK; 
    mCompleted = PR_TRUE;
    if (! mHasTransaction)
      return NS_ERROR_FAILURE;

    
    nsresult rv = NS_OK;
    do {
      rv = mConnection->RollbackTransaction();
      if (rv == NS_ERROR_STORAGE_BUSY)
        (void)PR_Sleep(PR_INTERVAL_NO_WAIT);
    } while (rv == NS_ERROR_STORAGE_BUSY);

    return rv;
  }

  




  PRBool HasTransaction()
  {
    return mHasTransaction;
  }

  



  void SetDefaultAction(PRBool aCommitOnComplete)
  {
    mCommitOnComplete = aCommitOnComplete;
  }

protected:
  nsCOMPtr<mozIStorageConnection> mConnection;
  PRBool mHasTransaction;
  PRBool mCommitOnComplete;
  PRBool mCompleted;
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
