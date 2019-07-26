





#include "storage_test_harness.h"

#include "mozStorageHelper.h"





void
test_HasTransaction()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  {
    mozStorageTransaction transaction(db, false);
    do_check_true(transaction.HasTransaction());
    (void)transaction.Commit();
    
    do_check_false(transaction.HasTransaction());
  }

  
  {
    mozStorageTransaction transaction(db, false);
    do_check_true(transaction.HasTransaction());
    (void)transaction.Rollback();
    do_check_false(transaction.HasTransaction());
  }

  
  mozStorageTransaction outerTransaction(db, false);
  do_check_true(outerTransaction.HasTransaction());
  {
    mozStorageTransaction innerTransaction(db, false);
    do_check_false(innerTransaction.HasTransaction());
  }
}

void
test_Commit()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  {
    mozStorageTransaction transaction(db, false);
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
    (void)transaction.Commit();
  }

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
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
    (void)transaction.Rollback();
  }

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
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
  }

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
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test (id INTEGER PRIMARY KEY)"
    ));
  }

  bool exists = true;
  (void)db->TableExists(NS_LITERAL_CSTRING("test"), &exists);
  do_check_false(exists);
}

void
test_SetDefaultAction()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  
  {
    mozStorageTransaction transaction(db, true);
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test1 (id INTEGER PRIMARY KEY)"
    ));
    transaction.SetDefaultAction(false);
  }
  bool exists = true;
  (void)db->TableExists(NS_LITERAL_CSTRING("test1"), &exists);
  do_check_false(exists);

  
  
  {
    mozStorageTransaction transaction(db, false);
    (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE test2 (id INTEGER PRIMARY KEY)"
    ));
    transaction.SetDefaultAction(true);
  }
  exists = false;
  (void)db->TableExists(NS_LITERAL_CSTRING("test2"), &exists);
  do_check_true(exists);
}

void
test_null_database_connection()
{
  
  
  mozStorageTransaction transaction(nullptr, false);

  do_check_false(transaction.HasTransaction());
  do_check_true(NS_SUCCEEDED(transaction.Commit()));
  do_check_true(NS_SUCCEEDED(transaction.Rollback()));
}

void (*gTests[])(void) = {
  test_HasTransaction,
  test_Commit,
  test_Rollback,
  test_AutoCommit,
  test_AutoRollback,
  test_SetDefaultAction,
  test_null_database_connection,
};

const char *file = __FILE__;
#define TEST_NAME "transaction helper"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
