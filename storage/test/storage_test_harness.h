





#include "TestHarness.h"

#include "nsMemory.h"
#include "prthread.h"
#include "nsThreadUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "mozilla/ReentrantMonitor.h"

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatementCallback.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStorageBindingParamsArray.h"
#include "mozIStorageBindingParams.h"
#include "mozIStorageAsyncStatement.h"
#include "mozIStorageStatement.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageError.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIEventTarget.h"

#include "sqlite3.h"

static int gTotalTests = 0;
static int gPassedTests = 0;

#define do_check_true(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (aCondition) { \
      gPassedTests++; \
    } else { \
      fail("%s | Expected true, got false at line %d", __FILE__, __LINE__); \
    } \
  PR_END_MACRO

#define do_check_false(aCondition) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (!aCondition) { \
      gPassedTests++; \
    } else { \
      fail("%s | Expected false, got true at line %d", __FILE__, __LINE__); \
    } \
  PR_END_MACRO

#define do_check_success(aResult) \
  do_check_true(NS_SUCCEEDED(aResult))

#ifdef LINUX


#define do_check_eq(aExpected, aActual) \
  do_check_true(aExpected == aActual)
#else
#include <sstream>

std::ostream& operator<<(std::ostream& aStream, const nsresult aInput)
{
  return aStream << static_cast<uint32_t>(aInput);
}
#define do_check_eq(aExpected, aActual) \
  PR_BEGIN_MACRO \
    gTotalTests++; \
    if (aExpected == aActual) { \
      gPassedTests++; \
    } else { \
      std::ostringstream temp; \
      temp << __FILE__ << " | Expected '" << aExpected << "', got '"; \
      temp << aActual <<"' at line " << __LINE__; \
      fail(temp.str().c_str()); \
    } \
  PR_END_MACRO
#endif

#define do_check_ok(aInvoc) do_check_true((aInvoc) == SQLITE_OK)

already_AddRefed<mozIStorageService>
getService()
{
  nsCOMPtr<mozIStorageService> ss =
    do_GetService("@mozilla.org/storage/service;1");
  do_check_true(ss);
  return ss.forget();
}

already_AddRefed<mozIStorageConnection>
getMemoryDatabase()
{
  nsCOMPtr<mozIStorageService> ss = getService();
  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = ss->OpenSpecialDatabase("memory", getter_AddRefs(conn));
  do_check_success(rv);
  return conn.forget();
}

already_AddRefed<mozIStorageConnection>
getDatabase()
{
  nsCOMPtr<nsIFile> dbFile;
  (void)NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                               getter_AddRefs(dbFile));
  NS_ASSERTION(dbFile, "The directory doesn't exists?!");

  nsresult rv = dbFile->Append(NS_LITERAL_STRING("storage_test_db.sqlite"));
  do_check_success(rv);

  nsCOMPtr<mozIStorageService> ss = getService();
  nsCOMPtr<mozIStorageConnection> conn;
  rv = ss->OpenDatabase(dbFile, getter_AddRefs(conn));
  do_check_success(rv);
  return conn.forget();
}


class AsyncStatementSpinner : public mozIStorageStatementCallback
                            , public mozIStorageCompletionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK

  AsyncStatementSpinner();

  void SpinUntilCompleted();

  uint16_t completionReason;

protected:
  virtual ~AsyncStatementSpinner() {}
  volatile bool mCompleted;
};

NS_IMPL_ISUPPORTS(AsyncStatementSpinner,
                  mozIStorageStatementCallback,
                  mozIStorageCompletionCallback)

AsyncStatementSpinner::AsyncStatementSpinner()
: completionReason(0)
, mCompleted(false)
{
}

NS_IMETHODIMP
AsyncStatementSpinner::HandleResult(mozIStorageResultSet *aResultSet)
{
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementSpinner::HandleError(mozIStorageError *aError)
{
  int32_t result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString warnMsg;
  warnMsg.AppendLiteral("An error occurred while executing an async statement: ");
  warnMsg.AppendInt(result);
  warnMsg.Append(' ');
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());

  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementSpinner::HandleCompletion(uint16_t aReason)
{
  completionReason = aReason;
  mCompleted = true;
  return NS_OK;
}

NS_IMETHODIMP
AsyncStatementSpinner::Complete(nsresult, nsISupports*)
{
  mCompleted = true;
  return NS_OK;
}

void AsyncStatementSpinner::SpinUntilCompleted()
{
  nsCOMPtr<nsIThread> thread(::do_GetCurrentThread());
  nsresult rv = NS_OK;
  bool processed = true;
  while (!mCompleted && NS_SUCCEEDED(rv)) {
    rv = thread->ProcessNextEvent(true, &processed);
  }
}

#define NS_DECL_ASYNCSTATEMENTSPINNER \
  NS_IMETHOD HandleResult(mozIStorageResultSet *aResultSet);








void
blocking_async_execute(mozIStorageBaseStatement *stmt)
{
  nsRefPtr<AsyncStatementSpinner> spinner(new AsyncStatementSpinner());

  nsCOMPtr<mozIStoragePendingStatement> pendy;
  (void)stmt->ExecuteAsync(spinner, getter_AddRefs(pendy));
  spinner->SpinUntilCompleted();
}





void
blocking_async_close(mozIStorageConnection *db)
{
  nsRefPtr<AsyncStatementSpinner> spinner(new AsyncStatementSpinner());

  db->AsyncClose(spinner);
  spinner->SpinUntilCompleted();
}













sqlite3_mutex_methods orig_mutex_methods;
sqlite3_mutex_methods wrapped_mutex_methods;

bool mutex_used_on_watched_thread = false;
PRThread *watched_thread = nullptr;









PRThread *last_non_watched_thread = nullptr;





extern "C" void wrapped_MutexEnter(sqlite3_mutex *mutex)
{
  PRThread *curThread = ::PR_GetCurrentThread();
  if (curThread == watched_thread)
    mutex_used_on_watched_thread = true;
  else
    last_non_watched_thread = curThread;
  orig_mutex_methods.xMutexEnter(mutex);
}

extern "C" int wrapped_MutexTry(sqlite3_mutex *mutex)
{
  if (::PR_GetCurrentThread() == watched_thread)
    mutex_used_on_watched_thread = true;
  return orig_mutex_methods.xMutexTry(mutex);
}

void hook_sqlite_mutex()
{
  
  
  do_check_ok(sqlite3_initialize());
  do_check_ok(sqlite3_shutdown());
  do_check_ok(::sqlite3_config(SQLITE_CONFIG_GETMUTEX, &orig_mutex_methods));
  do_check_ok(::sqlite3_config(SQLITE_CONFIG_GETMUTEX, &wrapped_mutex_methods));
  wrapped_mutex_methods.xMutexEnter = wrapped_MutexEnter;
  wrapped_mutex_methods.xMutexTry = wrapped_MutexTry;
  do_check_ok(::sqlite3_config(SQLITE_CONFIG_MUTEX, &wrapped_mutex_methods));
}








void watch_for_mutex_use_on_this_thread()
{
  watched_thread = ::PR_GetCurrentThread();
  mutex_used_on_watched_thread = false;
}












class ThreadWedger : public nsRunnable
{
public:
  explicit ThreadWedger(nsIEventTarget *aTarget)
  : mReentrantMonitor("thread wedger")
  , unwedged(false)
  {
    aTarget->Dispatch(this, aTarget->NS_DISPATCH_NORMAL);
  }

  NS_IMETHOD Run()
  {
    mozilla::ReentrantMonitorAutoEnter automon(mReentrantMonitor);

    if (!unwedged)
      automon.Wait();

    return NS_OK;
  }

  void unwedge()
  {
    mozilla::ReentrantMonitorAutoEnter automon(mReentrantMonitor);
    unwedged = true;
    automon.Notify();
  }

private:
  mozilla::ReentrantMonitor mReentrantMonitor;
  bool unwedged;
};









already_AddRefed<nsIThread>
get_conn_async_thread(mozIStorageConnection *db)
{
  
  watch_for_mutex_use_on_this_thread();

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("SELECT 1"),
    getter_AddRefs(stmt));
  blocking_async_execute(stmt);
  stmt->Finalize();

  nsCOMPtr<nsIThreadManager> threadMan =
    do_GetService("@mozilla.org/thread-manager;1");
  nsCOMPtr<nsIThread> asyncThread;
  threadMan->GetThreadFromPRThread(last_non_watched_thread,
                                   getter_AddRefs(asyncThread));

  
  
  nsCOMPtr<nsIEventTarget> target = do_GetInterface(db);
  nsCOMPtr<nsIThread> allegedAsyncThread = do_QueryInterface(target);
  PRThread *allegedPRThread;
  (void)allegedAsyncThread->GetPRThread(&allegedPRThread);
  do_check_eq(allegedPRThread, last_non_watched_thread);
  return asyncThread.forget();
}
