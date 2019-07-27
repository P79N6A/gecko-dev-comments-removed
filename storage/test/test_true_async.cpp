





#include "storage_test_harness.h"
#include "prthread.h"
#include "nsIEventTarget.h"
#include "nsIInterfaceRequestorUtils.h"

#include "sqlite3.h"

#include "mozilla/ReentrantMonitor.h"

using mozilla::ReentrantMonitor;
using mozilla::ReentrantMonitorAutoEnter;












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


#define do_check_ok(aInvoc) do_check_true((aInvoc) == SQLITE_OK)

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
    ReentrantMonitorAutoEnter automon(mReentrantMonitor);

    if (!unwedged)
      automon.Wait();

    return NS_OK;
  }

  void unwedge()
  {
    ReentrantMonitorAutoEnter automon(mReentrantMonitor);
    unwedged = true;
    automon.Notify();
  }

private:
  ReentrantMonitor mReentrantMonitor;
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





void
test_TrueAsyncStatement()
{
  
  hook_sqlite_mutex();

  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  watch_for_mutex_use_on_this_thread();

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("CREATE TABLE test (id INTEGER PRIMARY KEY)"),
    getter_AddRefs(stmt)
  );
  blocking_async_execute(stmt);
  stmt->Finalize();
  do_check_false(mutex_used_on_watched_thread);

  
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("INSERT INTO test (id) VALUES (?)"),
    getter_AddRefs(stmt)
  );
  stmt->BindInt32ByIndex(0, 1);
  blocking_async_execute(stmt);
  stmt->Finalize();
  do_check_false(mutex_used_on_watched_thread);
  
  
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("INSERT INTO test (id) VALUES (:id)"),
    getter_AddRefs(stmt)
  );
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  nsCOMPtr<mozIStorageBindingParams> params;
  paramsArray->NewBindingParams(getter_AddRefs(params));
  params->BindInt32ByName(NS_LITERAL_CSTRING("id"), 2);
  paramsArray->AddParams(params);
  params = nullptr;
  stmt->BindParameters(paramsArray);
  paramsArray = nullptr;
  blocking_async_execute(stmt);
  stmt->Finalize();
  do_check_false(mutex_used_on_watched_thread);

  
  
  nsCOMPtr<mozIStorageStatement> syncStmt;
  db->CreateStatement(NS_LITERAL_CSTRING("SELECT * FROM test"),
                      getter_AddRefs(syncStmt));
  syncStmt->Finalize();
  do_check_true(mutex_used_on_watched_thread);

  blocking_async_close(db);
}





void
test_AsyncCancellation()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<nsIThread> target(get_conn_async_thread(db));
  do_check_true(target);
  nsRefPtr<ThreadWedger> wedger (new ThreadWedger(target));

  
  
  nsCOMPtr<mozIStorageAsyncStatement> asyncStmt;
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("CREATE TABLE asyncTable (id INTEGER PRIMARY KEY)"),
    getter_AddRefs(asyncStmt)
  );

  nsRefPtr<AsyncStatementSpinner> asyncSpin(new AsyncStatementSpinner());
  nsCOMPtr<mozIStoragePendingStatement> asyncPend;
  (void)asyncStmt->ExecuteAsync(asyncSpin, getter_AddRefs(asyncPend));
  do_check_true(asyncPend);
  asyncPend->Cancel();

  
  nsCOMPtr<mozIStorageStatement> syncStmt;
  db->CreateStatement(
    NS_LITERAL_CSTRING("CREATE TABLE syncTable (id INTEGER PRIMARY KEY)"),
    getter_AddRefs(syncStmt)
  );

  nsRefPtr<AsyncStatementSpinner> syncSpin(new AsyncStatementSpinner());
  nsCOMPtr<mozIStoragePendingStatement> syncPend;
  (void)syncStmt->ExecuteAsync(syncSpin, getter_AddRefs(syncPend));
  do_check_true(syncPend);
  syncPend->Cancel();

  
  wedger->unwedge();

  
  asyncSpin->SpinUntilCompleted();
  do_check_true(asyncSpin->completionReason ==
                mozIStorageStatementCallback::REASON_CANCELED);

  syncSpin->SpinUntilCompleted();
  do_check_true(syncSpin->completionReason ==
                mozIStorageStatementCallback::REASON_CANCELED);

  
  nsresult rv;
  bool exists;
  rv = db->TableExists(NS_LITERAL_CSTRING("asyncTable"), &exists);
  do_check_true(rv == NS_OK);
  do_check_false(exists);
  rv = db->TableExists(NS_LITERAL_CSTRING("syncTable"), &exists);
  do_check_true(rv == NS_OK);
  do_check_false(exists);

  
  asyncStmt->Finalize();
  syncStmt->Finalize();
  blocking_async_close(db);
}







void test_AsyncDestructorFinalizesOnAsyncThread()
{
  

  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());
  watch_for_mutex_use_on_this_thread();

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(
    NS_LITERAL_CSTRING("CREATE TABLE test (id INTEGER PRIMARY KEY)"),
    getter_AddRefs(stmt)
  );

  
  blocking_async_execute(stmt);
  do_check_false(mutex_used_on_watched_thread);

  
  stmt = nullptr;

  
  do_check_false(mutex_used_on_watched_thread);

  
  
  blocking_async_close(db);
}

void (*gTests[])(void) = {
  
  test_TrueAsyncStatement,
  test_AsyncCancellation,
  test_AsyncDestructorFinalizesOnAsyncThread
};

const char *file = __FILE__;
#define TEST_NAME "true async statement"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
