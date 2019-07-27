





#ifndef mozStorageAsyncStatementExecution_h
#define mozStorageAsyncStatementExecution_h

#include "nscore.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"
#include "nsIRunnable.h"

#include "SQLiteMutex.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageStatementCallback.h"
#include "mozStorageHelper.h"

struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class Connection;
class ResultSet;
class StatementData;

class AsyncExecuteStatements final : public nsIRunnable
                                   , public mozIStoragePendingStatement
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
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
                          sqlite3 *aNativeConnection,
                          mozIStorageStatementCallback *aCallback,
                          mozIStoragePendingStatement **_stmt);

  








  bool shouldNotify();

private:
  AsyncExecuteStatements(StatementDataArray &aStatements,
                         Connection *aConnection,
                         sqlite3 *aNativeConnection,
                         mozIStorageStatementCallback *aCallback);
  ~AsyncExecuteStatements();

  













  bool bindExecuteAndProcessStatement(StatementData &aData,
                                      bool aLastStatement);

  












  bool executeAndProcessStatement(sqlite3_stmt *aStatement,
                                  bool aLastStatement);

  








  bool executeStatement(sqlite3_stmt *aStatement);

  








  nsresult buildAndNotifyResults(sqlite3_stmt *aStatement);

  




  nsresult notifyComplete();

  












  nsresult notifyError(int32_t aErrorCode, const char *aMessage);
  nsresult notifyError(mozIStorageError *aError);

  




  nsresult notifyResults();

  





  bool statementsNeedTransaction();

  StatementDataArray mStatements;
  nsRefPtr<Connection> mConnection;
  sqlite3 *mNativeConnection;
  bool mHasTransaction;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;
  nsRefPtr<ResultSet> mResultSet;

  



  const TimeDuration mMaxWait;

  


  TimeStamp mIntervalStart;

  


  ExecutionState mState;

  


  bool mCancelRequested;

  






  Mutex &mMutex;

  





  SQLiteMutex &mDBMutex;
  
  




  TimeStamp mRequestStartDate;
};

} 
} 

#endif 
