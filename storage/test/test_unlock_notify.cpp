






































#include "storage_test_harness.h"

#include "mozilla/Monitor.h"
#include "nsThreadUtils.h"
#include "mozIStorageStatement.h"









enum State {
  STARTING,
  WRITE_LOCK,
  READ_LOCK,
  TEST_DONE
};

class DatabaseLocker : public nsRunnable
{
public:
  DatabaseLocker(const char* aSQL)
  : monitor("DatabaseLocker::monitor")
  , mSQL(aSQL)
  , mState(STARTING)
  {
  }

  void RunInBackground()
  {
    (void)NS_NewThread(getter_AddRefs(mThread));
    do_check_true(mThread);

    do_check_success(mThread->Dispatch(this, NS_DISPATCH_NORMAL));
  }

  NS_IMETHOD Run()
  {
    mozilla::MonitorAutoEnter lock(monitor);

    nsCOMPtr<mozIStorageConnection> db(getDatabase());

    nsCString sql(mSQL);
    nsCOMPtr<mozIStorageStatement> stmt;
    do_check_success(db->CreateStatement(sql, getter_AddRefs(stmt)));

    PRBool hasResult;
    do_check_success(stmt->ExecuteStep(&hasResult));

    Notify(WRITE_LOCK);
    WaitFor(TEST_DONE);

    return NS_OK;
  }

  void WaitFor(State aState)
  {
    monitor.AssertCurrentThreadIn();
    while (mState != aState) {
      do_check_success(monitor.Wait());
    }
  }

  void Notify(State aState)
  {
    monitor.AssertCurrentThreadIn();
    mState = aState;
    do_check_success(monitor.Notify());
  }

  mozilla::Monitor monitor;

protected:
  nsCOMPtr<nsIThread> mThread;
  const char *const mSQL;
  State mState;
};

class DatabaseTester : public DatabaseLocker
{
public:
  DatabaseTester(mozIStorageConnection *aConnection,
                 const char* aSQL)
  : DatabaseLocker(aSQL)
  , mConnection(aConnection)
  {
  }

  NS_IMETHOD Run()
  {
    mozilla::MonitorAutoEnter lock(monitor);
    WaitFor(READ_LOCK);

    nsCString sql(mSQL);
    nsCOMPtr<mozIStorageStatement> stmt;
    do_check_success(mConnection->CreateStatement(sql, getter_AddRefs(stmt)));

    PRBool hasResult;
    nsresult rv = stmt->ExecuteStep(&hasResult);
    do_check_eq(rv, NS_ERROR_FILE_IS_LOCKED);

    
    
    rv = stmt->Finalize();
    do_check_eq(rv, NS_ERROR_FILE_IS_LOCKED);
    mConnection = nsnull;

    Notify(TEST_DONE);

    return NS_OK;
  }

private:
  nsCOMPtr<mozIStorageConnection> mConnection;
  State mState;
};




void
setup()
{
  nsCOMPtr<mozIStorageConnection> db(getDatabase());

  
  nsresult rv = db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (id INTEGER PRIMARY KEY, data STRING)"
  ));
  do_check_success(rv);
  rv = db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO test (data) VALUES ('foo')"
  ));
  do_check_success(rv);
  rv = db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO test (data) VALUES ('bar')"
  ));
  do_check_success(rv);
  rv = db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE UNIQUE INDEX unique_data ON test (data)"
  ));
  do_check_success(rv);
}

void
test_step_locked_does_not_block_main_thread()
{
  nsCOMPtr<mozIStorageConnection> db(getDatabase());

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (data) VALUES ('test1')"
  ), getter_AddRefs(stmt));
  do_check_success(rv);

  nsRefPtr<DatabaseLocker> locker(new DatabaseLocker("SELECT * FROM test"));
  do_check_true(locker);
  mozilla::MonitorAutoEnter lock(locker->monitor);
  locker->RunInBackground();

  
  locker->WaitFor(WRITE_LOCK);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  do_check_eq(rv, NS_ERROR_FILE_IS_LOCKED);

  locker->Notify(TEST_DONE);
}

void
test_drop_index_does_not_loop()
{
  nsCOMPtr<mozIStorageConnection> db(getDatabase());

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM test"
  ), getter_AddRefs(stmt));
  do_check_success(rv);

  nsRefPtr<DatabaseTester> tester =
    new DatabaseTester(db, "DROP INDEX unique_data");
  do_check_true(tester);
  mozilla::MonitorAutoEnter lock(tester->monitor);
  tester->RunInBackground();

  
  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  do_check_success(rv);
  do_check_true(hasResult);
  tester->Notify(READ_LOCK);

  
  tester->WaitFor(TEST_DONE);
}

void
test_drop_table_does_not_loop()
{
  nsCOMPtr<mozIStorageConnection> db(getDatabase());

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT * FROM test"
  ), getter_AddRefs(stmt));
  do_check_success(rv);

  nsRefPtr<DatabaseTester> tester(new DatabaseTester(db, "DROP TABLE test"));
  do_check_true(tester);
  mozilla::MonitorAutoEnter lock(tester->monitor);
  tester->RunInBackground();

  
  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  do_check_success(rv);
  do_check_true(hasResult);
  tester->Notify(READ_LOCK);

  
  tester->WaitFor(TEST_DONE);
}

void (*gTests[])(void) = {
  setup,
  test_step_locked_does_not_block_main_thread,
  test_drop_index_does_not_loop,
  test_drop_table_does_not_loop,
};

const char *file = __FILE__;
#define TEST_NAME "sqlite3_unlock_notify"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
