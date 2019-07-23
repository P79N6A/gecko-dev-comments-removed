









































function test_connectionReady_open()
{
  
  
  
  

  var msc = getOpenedDatabase();
  do_check_true(msc.connectionReady);
  run_next_test();
}

function test_connectionReady_closed()
{
  

  var msc = getOpenedDatabase();
  msc.close();
  do_check_false(msc.connectionReady);
  gDBConn = null; 
  run_next_test();
}

function test_databaseFile()
{
  var msc = getOpenedDatabase();
  do_check_true(getTestDB().equals(msc.databaseFile));
  run_next_test();
}

function test_tableExists_not_created()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.tableExists("foo"));
  run_next_test();
}

function test_indexExists_not_created()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.indexExists("foo"));
  run_next_test();
}

function test_createTable_not_created()
{
  var msc = getOpenedDatabase();
  msc.createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
  do_check_true(msc.tableExists("test"));
  run_next_test();
}

function test_indexExists_created()
{
  var msc = getOpenedDatabase();
  msc.executeSimpleSQL("CREATE INDEX name_ind ON test (name)");
  do_check_true(msc.indexExists("name_ind"));
  run_next_test();
}

function test_createTable_already_created()
{
  var msc = getOpenedDatabase();
  do_check_true(msc.tableExists("test"));
  try {
    msc.createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
    do_throw("We shouldn't get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
  run_next_test();
}

function test_lastInsertRowID()
{
  var msc = getOpenedDatabase();
  msc.executeSimpleSQL("INSERT INTO test (name) VALUES ('foo')");
  do_check_eq(1, msc.lastInsertRowID);
  run_next_test();
}

function test_transactionInProgress_no()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
  run_next_test();
}

function test_transactionInProgress_yes()
{
  var msc = getOpenedDatabase();
  msc.beginTransaction();
  do_check_true(msc.transactionInProgress);
  msc.commitTransaction();
  do_check_false(msc.transactionInProgress);

  msc.beginTransaction();
  do_check_true(msc.transactionInProgress);
  msc.rollbackTransaction();
  do_check_false(msc.transactionInProgress);
  run_next_test();
}

function test_commitTransaction_no_transaction()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
  try {
    msc.commitTransaction();
    do_throw("We should not get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
  run_next_test();
}

function test_rollbackTransaction_no_transaction()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
  try {
    msc.rollbackTransaction();
    do_throw("We should not get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
  run_next_test();
}

function test_get_schemaVersion_not_set()
{
  do_check_eq(0, getOpenedDatabase().schemaVersion);
  run_next_test();
}

function test_set_schemaVersion()
{
  var msc = getOpenedDatabase();
  const version = 1;
  msc.schemaVersion = version;
  do_check_eq(version, msc.schemaVersion);
  run_next_test();
}

function test_set_schemaVersion_same()
{
  var msc = getOpenedDatabase();
  const version = 1;
  msc.schemaVersion = version; 
  do_check_eq(version, msc.schemaVersion);
  run_next_test();
}

function test_set_schemaVersion_negative()
{
  var msc = getOpenedDatabase();
  const version = -1;
  msc.schemaVersion = version;
  do_check_eq(version, msc.schemaVersion);
  run_next_test();
}

function test_createTable(){
  var temp = getTestDB().parent;
  temp.append("test_db_table");
  try {
    var con = getService().openDatabase(temp);
    con.createTable("a","");
  } catch (e) {
    if (temp.exists()) try {
      temp.remove(false);
    } catch (e2) {}
    do_check_true(e.result==Cr.NS_ERROR_NOT_INITIALIZED ||
                  e.result==Cr.NS_ERROR_FAILURE);
  }
  run_next_test();
}

function test_defaultSynchronousAtNormal()
{
  var msc = getOpenedDatabase();
  var stmt = createStatement("PRAGMA synchronous;");
  try {
    stmt.executeStep();
    do_check_eq(1, stmt.getInt32(0));
  }
  finally {
    stmt.reset();
    stmt.finalize();
  }
  run_next_test();
}

function test_close_does_not_spin_event_loop()
{
  
  
  let event = {
    ran: false,
    run: function()
    {
      this.ran = true;
    },
  };

  
  
  let thread = Cc["@mozilla.org/thread-manager;1"].
               getService(Ci.nsIThreadManager).
               currentThread;
  thread.dispatch(event, Ci.nsIThread.DISPATCH_NORMAL);

  
  do_check_false(event.ran);
  getOpenedDatabase().close();
  do_check_false(event.ran);

  
  gDBConn = null;
  run_next_test();
}

function test_asyncClose_succeeds_with_finalized_async_statement()
{
  
  
  

  
  
  
  let stmt = createStatement("SELECT * FROM test");
  stmt.executeAsync();
  stmt.finalize();

  getOpenedDatabase().asyncClose(function() {
    
    gDBConn = null;
    run_next_test();
  });
}

function test_close_fails_with_async_statement_ran()
{
  let stmt = createStatement("SELECT * FROM test");
  stmt.executeAsync();
  stmt.finalize();

  let db = getOpenedDatabase();
  try {
    db.close();
    do_throw("should have thrown");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_UNEXPECTED);
  }
  finally {
    
    db.asyncClose(function() {
      
      gDBConn = null;
      run_next_test();
    });
  }
}




var tests = [
  test_connectionReady_open,
  test_connectionReady_closed,
  test_databaseFile,
  test_tableExists_not_created,
  test_indexExists_not_created,
  test_createTable_not_created,
  test_indexExists_created,
  test_createTable_already_created,
  test_lastInsertRowID,
  test_transactionInProgress_no,
  test_transactionInProgress_yes,
  test_commitTransaction_no_transaction,
  test_rollbackTransaction_no_transaction,
  test_get_schemaVersion_not_set,
  test_set_schemaVersion,
  test_set_schemaVersion_same,
  test_set_schemaVersion_negative,
  test_createTable,
  test_defaultSynchronousAtNormal,
  test_close_does_not_spin_event_loop, 
  test_asyncClose_succeeds_with_finalized_async_statement,
  test_close_fails_with_async_statement_ran,
];
let index = 0;

function run_next_test()
{
  function _run_next_test() {
    if (index < tests.length) {
      do_test_pending();
      print("Running the next test: " + tests[index].name);

      
      try {
        tests[index++]();
      }
      catch (e) {
        do_throw(e);
      }
    }

    do_test_finished();
  }

  
  do_execute_soon(_run_next_test);
}

function run_test()
{
  cleanup();

  do_test_pending();
  run_next_test();
}
