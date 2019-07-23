






































#ifndef _mozStorageAsyncStatementExecution_h_
#define _mozStorageAsyncStatementExecution_h_

#include "nscore.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"

#include "mozIStoragePendingStatement.h"
#include "mozIStorageStatementCallback.h"

struct sqlite3_stmt;
class mozStorageTransaction;

namespace mozilla {
namespace storage {

class Connection;
class ResultSet;
class StatementData;

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

  typedef nsTArray<StatementData> StatementDataArray;

  













  static nsresult execute(StatementDataArray &aStatements,
                          Connection *aConnection,
                          mozIStorageStatementCallback *aCallback,
                          mozIStoragePendingStatement **_stmt);

  








  bool shouldNotify();

private:
  AsyncExecuteStatements(StatementDataArray &aStatements,
                         Connection *aConnection,
                         mozIStorageStatementCallback *aCallback);

  













  bool bindExecuteAndProcessStatement(StatementData &aData,
                                      bool aLastStatement);

  












  bool executeAndProcessStatement(sqlite3_stmt *aStatement,
                                  bool aLastStatement);

  








  bool executeStatement(sqlite3_stmt *aStatement);

  








  nsresult buildAndNotifyResults(sqlite3_stmt *aStatement);

  




  nsresult notifyComplete();

  











  nsresult notifyError(PRInt32 aErrorCode, const char *aMessage);
  nsresult notifyError(mozIStorageError *aError);

  




  nsresult notifyResults();

  StatementDataArray mStatements;
  nsRefPtr<Connection> mConnection;
  mozStorageTransaction *mTransactionManager;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;
  nsRefPtr<ResultSet> mResultSet;

  



  const TimeDuration mMaxWait;

  


  TimeStamp mIntervalStart;

  


  ExecutionState mState;

  


  bool mCancelRequested;

  






  Mutex &mMutex;
};

} 
} 

#endif 
