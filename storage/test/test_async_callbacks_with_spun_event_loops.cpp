


#include "storage_test_harness.h"
#include "prthread.h"
#include "nsIEventTarget.h"
#include "nsIInterfaceRequestorUtils.h"
#include "mozilla/Attributes.h"

#include "sqlite3.h"







void
spin_events_loop_until_true(const bool* const aCondition)
{
  nsCOMPtr<nsIThread> thread(::do_GetCurrentThread());
  nsresult rv = NS_OK;
  bool processed = true;
  while (!(*aCondition) && NS_SUCCEEDED(rv)) {
    rv = thread->ProcessNextEvent(true, &processed);
  }
}




class UnownedCallback final : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS

  
  static bool sAlive;
  
  static bool sResult;
  
  static bool sError;

  explicit UnownedCallback(mozIStorageConnection* aDBConn)
  : mDBConn(aDBConn)
  , mCompleted(false)
  {
    sAlive = true;
    sResult = false;
    sError = false;
  }

private:
  ~UnownedCallback()
  {
    sAlive = false;
    blocking_async_close(mDBConn);
  }

public:
  NS_IMETHOD HandleResult(mozIStorageResultSet* aResultSet) override
  {
    sResult = true;
    spin_events_loop_until_true(&mCompleted);
    if (!sAlive) {
      NS_RUNTIMEABORT("The statement callback was destroyed prematurely.");
    }
    return NS_OK;
  }

  NS_IMETHOD HandleError(mozIStorageError* aError) override
  {
    sError = true;
    spin_events_loop_until_true(&mCompleted);
    if (!sAlive) {
      NS_RUNTIMEABORT("The statement callback was destroyed prematurely.");
    }
    return NS_OK;
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason) override
  {
    mCompleted = true;
    return NS_OK;
  }

protected:
  nsCOMPtr<mozIStorageConnection> mDBConn;
  bool mCompleted;
};

NS_IMPL_ISUPPORTS(UnownedCallback, mozIStorageStatementCallback)

bool UnownedCallback::sAlive = false;
bool UnownedCallback::sResult = false;
bool UnownedCallback::sError = false;




void
test_SpinEventsLoopInHandleResult()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt));
  stmt->Execute();
  stmt->Finalize();

  db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (id) VALUES (?)"
  ), getter_AddRefs(stmt));
  for (int32_t i = 0; i < 30; ++i) {
    stmt->BindInt32ByIndex(0, i);
    stmt->Execute();
    stmt->Reset();
  }
  stmt->Finalize();

  db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM test"
  ), getter_AddRefs(stmt));
  nsCOMPtr<mozIStoragePendingStatement> ps;
  do_check_success(stmt->ExecuteAsync(new UnownedCallback(db),
                                      getter_AddRefs(ps)));
  stmt->Finalize();

  spin_events_loop_until_true(&UnownedCallback::sResult);
}

void
test_SpinEventsLoopInHandleError()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  db->CreateStatement(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY)"
  ), getter_AddRefs(stmt));
  stmt->Execute();
  stmt->Finalize();

  db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (id) VALUES (1)"
  ), getter_AddRefs(stmt));
  stmt->Execute();
  stmt->Finalize();

  
  db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (id) VALUES (1)"
  ), getter_AddRefs(stmt));
  nsCOMPtr<mozIStoragePendingStatement> ps;
  do_check_success(stmt->ExecuteAsync(new UnownedCallback(db),
                                      getter_AddRefs(ps)));
  stmt->Finalize();

  spin_events_loop_until_true(&UnownedCallback::sError);
}

void (*gTests[])(void) = {
  test_SpinEventsLoopInHandleResult,
  test_SpinEventsLoopInHandleError,
};

const char *file = __FILE__;
#define TEST_NAME "test async callbacks with spun event loops"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
