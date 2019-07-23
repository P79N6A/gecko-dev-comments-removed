






































#ifndef _mozStorageAsyncStatementExecution_h_
#define _mozStorageAsyncStatementExecution_h_

#include "nscore.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

#include "mozIStoragePendingStatement.h"
#include "mozIStorageStatementCallback.h"

struct sqlite3_stmt;
class mozStorageTransaction;

namespace mozilla {
namespace storage {

class Connection;
class ResultSet;

class AsyncExecuteStatements : public nsIRunnable
                             , public mozIStoragePendingStatement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_MOZISTORAGEPENDINGSTATEMENT

  


  enum ExecutionState {
    PENDING = -1,
    COMPLETED = mozIStorageStatementCallback::REASON_FINISHED,
    CANCELED = mozIStorageStatementCallback::REASON_CANCELED,
    ERROR = mozIStorageStatementCallback::REASON_ERROR
  };

  typedef nsTArray<sqlite3_stmt *> sqlite3_stmt_array;

  













  static nsresult execute(sqlite3_stmt_array &aStatements,
                          Connection *aConnection,
                          mozIStorageStatementCallback *aCallback,
                          mozIStoragePendingStatement **_stmt);

  






  bool shouldNotify();

private:
  AsyncExecuteStatements(sqlite3_stmt_array &aStatements,
                         mozIStorageConnection *aConnection,
                         mozIStorageStatementCallback *aCallback);

  


  nsresult initialize();

  ~AsyncExecuteStatements();

  












  bool executeAndProcessStatement(sqlite3_stmt *aStatement,
                                  bool aLastStatement);

  








  nsresult buildAndNotifyResults(sqlite3_stmt *aStatement);

  




  nsresult notifyComplete();

  









  nsresult notifyError(PRInt32 aErrorCode, const char *aMessage);

  




  nsresult notifyResults();

  sqlite3_stmt_array mStatements;
  mozIStorageConnection *mConnection;
  mozStorageTransaction *mTransactionManager;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;
  nsRefPtr<ResultSet> mResultSet;

  



  const PRIntervalTime mMaxIntervalWait;

  


  PRIntervalTime mIntervalStart;

  


  ExecutionState mState;

  


  bool mCancelRequested;

  







  PRLock *mLock;
};

} 
} 

#endif 
