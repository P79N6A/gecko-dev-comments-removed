


#include "storage_test_harness.h"

#include "nsIEventTarget.h"
#include "mozStorageConnection.h"

#include "sqlite3.h"

using namespace mozilla::storage;










int commit_hook(void *aArg)
{
  int *arg = static_cast<int *>(aArg);
  (*arg)++;
  return 0;
}














void
check_transaction(mozIStorageConnection *aDB,
                  mozIStorageBaseStatement **aStmts,
                  PRUint32 aStmtsLen,
                  bool aTransactionExpected)
{
  
  int commit = 0;
  ::sqlite3_commit_hook(*static_cast<Connection *>(aDB), commit_hook, &commit);

  nsRefPtr<AsyncStatementSpinner> asyncSpin(new AsyncStatementSpinner());
  nsCOMPtr<mozIStoragePendingStatement> asyncPend;
  do_check_success(aDB->ExecuteAsync(aStmts, aStmtsLen, asyncSpin,
                                     getter_AddRefs(asyncPend)));
  do_check_true(asyncPend);

  
  asyncSpin->SpinUntilCompleted();

  
  ::sqlite3_commit_hook(*static_cast<Connection *>(aDB), NULL, NULL);

  
  do_check_eq(aTransactionExpected, !!commit);

  
  if (aTransactionExpected) {
    do_check_eq(1, commit);
  }

  
  for (PRUint32 i = 0; i < aStmtsLen; ++i) {
    aStmts[i]->Finalize();
  }
  blocking_async_close(aDB);
}








void
test_MultipleAsyncReadStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt1;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageAsyncStatement> stmt2;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_MultipleReadStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt1;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageStatement> stmt2;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_MultipleAsyncReadWriteStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt1;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageAsyncStatement> stmt2;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}




void
test_MultipleReadWriteStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt1;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageStatement> stmt2;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}





void
test_MultipleAsyncWriteStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt1;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test1 (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageAsyncStatement> stmt2;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test2 (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}





void
test_MultipleWriteStatements()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt1;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test1 (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt1));

  nsCOMPtr<mozIStorageStatement> stmt2;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test2 (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt2));

  mozIStorageBaseStatement *stmts[] = {
    stmt1,
    stmt2,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}





void
test_SingleAsyncReadStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt));

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_SingleReadStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM sqlite_master"
  ), getter_AddRefs(stmt));

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_SingleAsyncWriteStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt));

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}




void
test_SingleWriteStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt));

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}





void
test_MultipleParamsAsyncReadStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT :param FROM sqlite_master"
  ), getter_AddRefs(stmt));

  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  for (PRInt32 i = 0; i < 2; i++) {
    nsCOMPtr<mozIStorageBindingParams> params;
    paramsArray->NewBindingParams(getter_AddRefs(params));
    params->BindInt32ByName(NS_LITERAL_CSTRING("param"), 1);
    paramsArray->AddParams(params);
  }
  stmt->BindParameters(paramsArray);
  paramsArray = nsnull;

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_MultipleParamsReadStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT :param FROM sqlite_master"
  ), getter_AddRefs(stmt));

  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  for (PRInt32 i = 0; i < 2; i++) {
    nsCOMPtr<mozIStorageBindingParams> params;
    paramsArray->NewBindingParams(getter_AddRefs(params));
    params->BindInt32ByName(NS_LITERAL_CSTRING("param"), 1);
    paramsArray->AddParams(params);
  }
  stmt->BindParameters(paramsArray);
  paramsArray = nsnull;

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), false);
}





void
test_MultipleParamsAsyncWriteStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> tableStmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(tableStmt));
  tableStmt->Execute();
  tableStmt->Finalize();

  
  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  db->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "DELETE FROM test WHERE id = :param"
  ), getter_AddRefs(stmt));

  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  for (PRInt32 i = 0; i < 2; i++) {
    nsCOMPtr<mozIStorageBindingParams> params;
    paramsArray->NewBindingParams(getter_AddRefs(params));
    params->BindInt32ByName(NS_LITERAL_CSTRING("param"), 1);
    paramsArray->AddParams(params);
  }
  stmt->BindParameters(paramsArray);
  paramsArray = nsnull;

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}





void
test_MultipleParamsWriteStatement()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> tableStmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(tableStmt));
  tableStmt->Execute();
  tableStmt->Finalize();

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM test WHERE id = :param"
  ), getter_AddRefs(stmt));

  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  for (PRInt32 i = 0; i < 2; i++) {
    nsCOMPtr<mozIStorageBindingParams> params;
    paramsArray->NewBindingParams(getter_AddRefs(params));
    params->BindInt32ByName(NS_LITERAL_CSTRING("param"), 1);
    paramsArray->AddParams(params);
  }
  stmt->BindParameters(paramsArray);
  paramsArray = nsnull;

  mozIStorageBaseStatement *stmts[] = {
    stmt,
  };

  check_transaction(db, stmts, NS_ARRAY_LENGTH(stmts), true);
}

void (*gTests[])(void) = {
  test_MultipleAsyncReadStatements,
  test_MultipleReadStatements,
  test_MultipleAsyncReadWriteStatements,
  test_MultipleReadWriteStatements,
  test_MultipleAsyncWriteStatements,
  test_MultipleWriteStatements,
  test_SingleAsyncReadStatement,
  test_SingleReadStatement,
  test_SingleAsyncWriteStatement,
  test_SingleWriteStatement,
  test_MultipleParamsAsyncReadStatement,
  test_MultipleParamsReadStatement,
  test_MultipleParamsAsyncWriteStatement,
  test_MultipleParamsWriteStatement,
};

const char *file = __FILE__;
#define TEST_NAME "async statement execution transaction"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
