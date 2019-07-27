




#ifndef MOZSTORAGEHELPER_H
#define MOZSTORAGEHELPER_H

#include "nsAutoPtr.h"
#include "nsStringGlue.h"
#include "mozilla/DebugOnly.h"

#include "mozIStorageAsyncConnection.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozIStoragePendingStatement.h"
#include "nsError.h"








































class mozStorageTransaction
{
public:
  mozStorageTransaction(mozIStorageConnection* aConnection,
                        bool aCommitOnComplete,
                        int32_t aType = mozIStorageConnection::TRANSACTION_DEFERRED,
                        bool aAsyncCommit = false)
    : mConnection(aConnection),
      mHasTransaction(false),
      mCommitOnComplete(aCommitOnComplete),
      mCompleted(false),
      mAsyncCommit(aAsyncCommit)
  {
    if (mConnection) {
      nsAutoCString query("BEGIN");
      switch(aType) {
        case mozIStorageConnection::TRANSACTION_IMMEDIATE:
          query.AppendLiteral(" IMMEDIATE");
          break;
        case mozIStorageConnection::TRANSACTION_EXCLUSIVE:
          query.AppendLiteral(" EXCLUSIVE");
          break;
        case mozIStorageConnection::TRANSACTION_DEFERRED:
          query.AppendLiteral(" DEFERRED");
          break;
        default:
          MOZ_ASSERT(false, "Unknown transaction type");
      }
      
      
      mHasTransaction = NS_SUCCEEDED(mConnection->ExecuteSimpleSQL(query));
    }
  }

  ~mozStorageTransaction()
  {
    if (mConnection && mHasTransaction && !mCompleted) {
      if (mCommitOnComplete) {
        mozilla::DebugOnly<nsresult> rv = Commit();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                         "A transaction didn't commit correctly");
      }
      else {
        mozilla::DebugOnly<nsresult> rv = Rollback();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                         "A transaction didn't rollback correctly");
      }
    }
  }

  




  nsresult Commit()
  {
    if (!mConnection || mCompleted || !mHasTransaction)
      return NS_OK;
    mCompleted = true;

    
    
    nsresult rv;
    if (mAsyncCommit) {
      nsCOMPtr<mozIStoragePendingStatement> ps;
      rv = mConnection->ExecuteSimpleSQLAsync(NS_LITERAL_CSTRING("COMMIT"),
                                              nullptr, getter_AddRefs(ps));
    }
    else {
      rv = mConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("COMMIT"));
    }

    if (NS_SUCCEEDED(rv))
      mHasTransaction = false;

    return rv;
  }

  




  nsresult Rollback()
  {
    if (!mConnection || mCompleted || !mHasTransaction)
      return NS_OK;
    mCompleted = true;

    
    
    nsresult rv = NS_OK;
    do {
      rv = mConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("ROLLBACK"));
      if (rv == NS_ERROR_STORAGE_BUSY)
        (void)PR_Sleep(PR_INTERVAL_NO_WAIT);
    } while (rv == NS_ERROR_STORAGE_BUSY);

    if (NS_SUCCEEDED(rv))
      mHasTransaction = false;

    return rv;
  }

protected:
  nsCOMPtr<mozIStorageConnection> mConnection;
  bool mHasTransaction;
  bool mCommitOnComplete;
  bool mCompleted;
  bool mAsyncCommit;
};








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
