






































#include "storage_test_harness.h"

#include "mozStorageHelper.h"





void
test_automatic_reset()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ));

  nsCOMPtr<mozIStorageStatement> stmt;
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt));

  
  PRInt32 state = -1;
  (void)stmt->GetState(&state);
  do_check_true(state == mozIStorageStatement::MOZ_STORAGE_STATEMENT_READY);

  
  {
    mozStorageStatementScoper scoper(stmt);
    PRBool hasMore;
    do_check_true(NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)));

    
    state = -1;
    (void)stmt->GetState(&state);
    do_check_true(state ==
                  mozIStorageStatement::MOZ_STORAGE_STATEMENT_EXECUTING);
  }

  
  state = -1;
  (void)stmt->GetState(&state);
  do_check_true(state == mozIStorageStatement::MOZ_STORAGE_STATEMENT_READY);
}

void
test_Abandon()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ));

  nsCOMPtr<mozIStorageStatement> stmt;
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt));

  
  PRInt32 state = -1;
  (void)stmt->GetState(&state);
  do_check_true(state == mozIStorageStatement::MOZ_STORAGE_STATEMENT_READY);

  
  {
    mozStorageStatementScoper scoper(stmt);
    PRBool hasMore;
    do_check_true(NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)));

    
    state = -1;
    (void)stmt->GetState(&state);
    do_check_true(state ==
                  mozIStorageStatement::MOZ_STORAGE_STATEMENT_EXECUTING);

    
    scoper.Abandon();
  }

  
  state = -1;
  (void)stmt->GetState(&state);
  do_check_true(state == mozIStorageStatement::MOZ_STORAGE_STATEMENT_EXECUTING);
}

void (*gTests[])(void) = {
  test_automatic_reset,
  test_Abandon,
};

const char *file = __FILE__;
#define TEST_NAME "statement scoper"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
