





#include "storage_test_harness.h"

#include "mozStorageHelper.h"
#include "mozStorageConnection.h"

using namespace mozilla;
using namespace mozilla::storage;

bool has_transaction(mozIStorageConnection* aDB) {
  return !(static_cast<Connection *>(aDB)->getAutocommit());
}





void
test_Commit()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  {
    mozStorageTransaction transaction(db, false);
    do_check_true(has_transaction(db));
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
    (void)transaction.Commit();
  }
  do_check_false(has_transaction(db));

  bool exists = false;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_true(exists);
}

void
test_Rollback()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  {
    mozStorageTransaction transaction(db, true);
    do_check_true(has_transaction(db));
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
    (void)transaction.Rollback();
  }
  do_check_false(has_transaction(db));

  bool exists = true;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_false(exists);
}

void
test_AutoCommit()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  {
    mozStorageTransaction transaction(db, true);
    do_check_true(has_transaction(db));
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
  }
  do_check_false(has_transaction(db));

  bool exists = false;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_true(exists);
}

void
test_AutoRollback()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  
  {
    mozStorageTransaction transaction(db, false);
    do_check_true(has_transaction(db));
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
  }
  do_check_false(has_transaction(db));

  bool exists = true;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_false(exists);
}

void
test_null_database_connection()
{
  
  
  mozStorageTransaction transaction(nullptr, false);
  do_check_true(NS_SUCCEEDED(transaction.Commit()));
  do_check_true(NS_SUCCEEDED(transaction.Rollback()));
}

void
test_async_Commit()
{
  
  hook_sqlite_mutex();

  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<nsIThread> target(get_conn_async_thread(db));
  do_check_true(target);
  nsRefPtr<ThreadWedger> wedger (new ThreadWedger(target));

  {
    mozStorageTransaction transaction(db, false,
                                      mozIStorageConnection::TRANSACTION_DEFERRED,
                                      true);
    do_check_true(has_transaction(db));
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
    (void)transaction.Commit();
  }
  do_check_true(has_transaction(db));

  
  wedger->unwedge();

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  (void)db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT NULL"
  ), getter_AddRefs(stmt));
  blocking_async_execute(stmt);
  stmt->Finalize();
  do_check_false(has_transaction(db));
  bool exists = false;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_true(exists);

  blocking_async_close(db);
}

void (*gTests[])(void) = {
  test_Commit,
  test_Rollback,
  test_AutoCommit,
  test_AutoRollback,
  test_null_database_connection,
  test_async_Commit,
};

const char *file = __FILE__;
#define TEST_NAME "transaction helper"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
