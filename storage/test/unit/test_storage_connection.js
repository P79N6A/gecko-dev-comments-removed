









































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
    do_check_eq(Cr.NS_ERROR_UNEXPECTED, e.result);
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
    do_check_eq(Cr.NS_ERROR_UNEXPECTED, e.result);
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

function test_clone_optional_param()
{
  let db1 = getService().openUnsharedDatabase(getTestDB());
  let db2 = db1.clone();
  do_check_true(db2.connectionReady);

  
  let stmt = db2.createStatement("INSERT INTO test (name) VALUES (:name)");
  stmt.params.name = "dwitte";
  stmt.execute();
  stmt.finalize();

  
  stmt = db2.createStatement("SELECT * FROM test");
  do_check_true(stmt.executeStep());
  stmt.finalize();

  
  do_check_true(db1.databaseFile.equals(db2.databaseFile));

  run_next_test();
}

function test_clone_readonly()
{
  let db1 = getService().openUnsharedDatabase(getTestDB());
  let db2 = db1.clone(true);
  do_check_true(db2.connectionReady);

  
  let stmt = db2.createStatement("INSERT INTO test (name) VALUES (:name)");
  stmt.params.name = "reed";
  expectError(Cr.NS_ERROR_FILE_READ_ONLY, function() stmt.execute());
  stmt.finalize();

  
  stmt = db2.createStatement("SELECT * FROM test");
  do_check_true(stmt.executeStep());
  stmt.finalize();

  run_next_test();
}

function test_clone_shared_readonly()
{
  let db1 = getService().openDatabase(getTestDB());
  let db2 = db1.clone(true);
  do_check_true(db2.connectionReady);

  
  let stmt = db2.createStatement("INSERT INTO test (name) VALUES (:name)");
  stmt.params.name = "reed";
  
  
  
  
  stmt.execute(); 
  
  stmt.finalize();

  
  stmt = db2.createStatement("SELECT * FROM test");
  do_check_true(stmt.executeStep());
  stmt.finalize();

  run_next_test();
}

function test_close_clone_fails()
{
  let calls = [
    "openDatabase",
    "openUnsharedDatabase",
  ];
  calls.forEach(function(methodName) {
    let db = getService()[methodName](getTestDB());
    db.close();
    expectError(Cr.NS_ERROR_NOT_INITIALIZED, function() db.clone());
  });

  run_next_test();
}

function test_memory_clone_fails()
{
  let db = getService().openSpecialDatabase("memory");
  db.close();
  expectError(Cr.NS_ERROR_NOT_INIALIZED, function() db.clone());

  run_next_test();
}

function test_clone_copies_functions()
{
  const FUNC_NAME = "test_func";
  let calls = [
    "openDatabase",
    "openUnsharedDatabase",
  ];
  let functionMethods = [
    "createFunction",
    "createAggregateFunction",
  ];
  calls.forEach(function(methodName) {
    [true, false].forEach(function(readOnly) {
      functionMethods.forEach(function(functionMethod) {
        let db1 = getService()[methodName](getTestDB());
        
        db1[functionMethod](FUNC_NAME, 1, {
          onFunctionCall: function() 0,
          onStep: function() 0,
          onFinal: function() 0,
        });

        
        let db2 = db1.clone(readOnly);
        
        let stmt = db2.createStatement("SELECT " + FUNC_NAME + "(id) FROM test");
        stmt.finalize();
        db1.close();
        db2.close();
      });
    });
  });

  run_next_test();
}

function test_clone_copies_overridden_functions()
{
  const FUNC_NAME = "lower";
  function test_func() {
    this.called = false;
  }
  test_func.prototype = {
    onFunctionCall: function() {
      this.called = true;
    },
    onStep: function() {
      this.called = true;
    },
    onFinal: function() 0,
  };

  let calls = [
    "openDatabase",
    "openUnsharedDatabase",
  ];
  let functionMethods = [
    "createFunction",
    "createAggregateFunction",
  ];
  calls.forEach(function(methodName) {
    [true, false].forEach(function(readOnly) {
      functionMethods.forEach(function(functionMethod) {
        let db1 = getService()[methodName](getTestDB());
        
        let func = new test_func();
        db1[functionMethod](FUNC_NAME, 1, func);
        do_check_false(func.called);

        
        let db2 = db1.clone(readOnly);
        let stmt = db2.createStatement("SELECT " + FUNC_NAME + "(id) FROM test");
        stmt.executeStep();
        do_check_true(func.called);
        stmt.finalize();
        db1.close();
        db2.close();
      });
    });
  });

  run_next_test();
}

function test_getInterface()
{
  let db = getOpenedDatabase();
  let target = db.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIEventTarget);
  
  
  do_check_true(target != null);

  run_next_test();
}




[
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
  test_clone_optional_param,
  test_clone_readonly,
  test_close_clone_fails,
  test_clone_copies_functions,
  test_clone_copies_overridden_functions,
  test_getInterface,
].forEach(add_test);

function run_test()
{
  run_next_test();
}
