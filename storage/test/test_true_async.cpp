





#include "storage_test_harness.h"




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
