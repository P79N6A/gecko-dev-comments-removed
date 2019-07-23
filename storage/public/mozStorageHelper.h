





































#ifndef _MOZSTORAGEHELPER_H_
#define _MOZSTORAGEHELPER_H_

#include "mozIStorageConnection.h"

















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
    if (mConnection) {
      PRBool transactionInProgress = PR_FALSE;
      mConnection->GetTransactionInProgress(&transactionInProgress);
      mHasTransaction = ! transactionInProgress;
      if (mHasTransaction)
        mConnection->BeginTransactionAs(aType);
    }
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
    return mConnection->RollbackTransaction();
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









class mozStorageStatementScoper
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
