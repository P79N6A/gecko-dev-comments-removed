










































const INTEGER = 1;
const TEXT = "this is test text";
const REAL = 3.23;
const BLOB = [1, 2];































function execAsync(aStmt, aOptions, aResults)
{
  let caller = Components.stack.caller;
  if (aOptions == null)
    aOptions = {};

  let resultsExpected;
  let resultsChecker;
  if (aResults == null) {
    resultsExpected = 0;
  }
  else if (typeof(aResults) == "number") {
    resultsExpected = aResults;
  }
  else if (typeof(aResults) == "function") {
    resultsChecker = aResults;
  }
  else { 
    resultsExpected = aResults.length;
    resultsChecker = function(aResultNum, aTup, aCaller) {
      aResults[aResultNum](aTup, aCaller);
    };
  }
  let resultsSeen = 0;

  let errorCodeExpected = false;
  let reasonExpected = Ci.mozIStorageStatementCallback.REASON_FINISHED;
  let altReasonExpected = null;
  if ("error" in aOptions) {
    errorCodeExpected = aOptions.error;
    if (errorCodeExpected)
      reasonExpected = Ci.mozIStorageStatementCallback.REASON_ERROR;
  }
  let errorCodeSeen = false;

  if ("cancel" in aOptions && aOptions.cancel)
    altReasonExpected = Ci.mozIStorageStatementCallback.REASON_CANCELED;

  let completed = false;

  let listener = {
    handleResult: function(aResultSet)
    {
      let row, resultsSeenThisCall = 0;
      while ((row = aResultSet.getNextRow()) != null) {
        if (resultsChecker)
          resultsChecker(resultsSeen, row, caller);
        resultsSeen++;
        resultsSeenThisCall++;
      }

      if (!resultsSeenThisCall)
        do_throw("handleResult invoked with 0 result rows!");
    },
    handleError: function(aError)
    {
      if (errorCodeSeen != false)
        do_throw("handleError called when we already had an error!");
      errorCodeSeen = aError.result;
    },
    handleCompletion: function(aReason)
    {
      if (completed) 
        do_throw("Received a second handleCompletion notification!", caller);

      if (resultsSeen != resultsExpected)
        do_throw("Expected " + resultsExpected + " rows of results but " +
                 "got " + resultsSeen + " rows!", caller);

      if (errorCodeExpected == true && errorCodeSeen == false)
        do_throw("Expected an error, but did not see one.", caller);
      else if (errorCodeExpected != errorCodeSeen)
        do_throw("Expected error code " + errorCodeExpected + " but got " +
                 errorCodeSeen, caller);

      if (aReason != reasonExpected && aReason != altReasonExpected)
        do_throw("Expected reason " + reasonExpected +
                 (altReasonExpected ? (" or " + altReasonExpected) : "") +
                 " but got " + aReason, caller);

      completed = true;
    }
  };

  let pending;
  
  
  if (("cancel" in aOptions && aOptions.cancel) ||
      ("returnPending" in aOptions && aOptions.returnPending)) {
    pending = aStmt.executeAsync(listener);
  }
  else {
    aStmt.executeAsync(listener);
  }

  if ("cancel" in aOptions && aOptions.cancel)
    pending.cancel();

  let curThread = Components.classes["@mozilla.org/thread-manager;1"]
                            .getService().currentThread;
  while (!completed && !_quit)
    curThread.processNextEvent(true);

  return pending;
}






function test_illegal_sql_async_deferred()
{
  
  let stmt = makeTestStatement("I AM A ROBOT. DO AS I SAY.");
  execAsync(stmt, {error: Ci.mozIStorageError.ERROR});
  stmt.finalize();

  
  stmt = makeTestStatement("SELECT destination FROM funkytown");
  execAsync(stmt, {error: Ci.mozIStorageError.ERROR});
  stmt.finalize();

  run_next_test();
}
test_illegal_sql_async_deferred.asyncOnly = true;

function test_create_table()
{
  
  do_check_false(getOpenedDatabase().tableExists("test"));

  var stmt = makeTestStatement(
    "CREATE TABLE test (" +
      "id INTEGER, " +
      "string TEXT, " +
      "number REAL, " +
      "nuller NULL, " +
      "blober BLOB" +
    ")"
  );
  execAsync(stmt);
  stmt.finalize();

  
  do_check_true(getOpenedDatabase().tableExists("test"));

  
  let checkStmt = getOpenedDatabase().createStatement(
    "SELECT id, string, number, nuller, blober FROM test"
  );
  checkStmt.finalize();
  run_next_test();
}

function test_add_data()
{
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (?, ?, ?, ?, ?)"
  );
  stmt.bindBlobParameter(4, BLOB, BLOB.length);
  stmt.bindNullParameter(3);
  stmt.bindDoubleParameter(2, REAL);
  stmt.bindStringParameter(1, TEXT);
  stmt.bindInt32Parameter(0, INTEGER);

  execAsync(stmt);
  stmt.finalize();

  
  verifyQuery("SELECT string, number, nuller, blober FROM test WHERE id = ?",
              INTEGER,
              [TEXT, REAL, null, BLOB]);
  run_next_test();
}

function test_get_data()
{
  var stmt = makeTestStatement(
    "SELECT string, number, nuller, blober, id FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, INTEGER);
  execAsync(stmt, {}, [
    function(tuple)
    {
      do_check_neq(null, tuple);

      
      do_check_false(tuple.getIsNull(0));
      do_check_eq(tuple.getResultByName("string"), tuple.getResultByIndex(0));
      do_check_eq(TEXT, tuple.getResultByName("string"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_TEXT,
                  tuple.getTypeOfIndex(0));

      do_check_false(tuple.getIsNull(1));
      do_check_eq(tuple.getResultByName("number"), tuple.getResultByIndex(1));
      do_check_eq(REAL, tuple.getResultByName("number"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_FLOAT,
                  tuple.getTypeOfIndex(1));

      do_check_true(tuple.getIsNull(2));
      do_check_eq(tuple.getResultByName("nuller"), tuple.getResultByIndex(2));
      do_check_eq(null, tuple.getResultByName("nuller"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_NULL,
                  tuple.getTypeOfIndex(2));

      do_check_false(tuple.getIsNull(3));
      var blobByName = tuple.getResultByName("blober");
      do_check_eq(BLOB.length, blobByName.length);
      var blobByIndex = tuple.getResultByIndex(3);
      do_check_eq(BLOB.length, blobByIndex.length);
      for (var i = 0; i < BLOB.length; i++) {
        do_check_eq(BLOB[i], blobByName[i]);
        do_check_eq(BLOB[i], blobByIndex[i]);
      }
      var count = { value: 0 };
      var blob = { value: null };
      tuple.getBlob(3, count, blob);
      do_check_eq(BLOB.length, count.value);
      for (var i = 0; i < BLOB.length; i++)
        do_check_eq(BLOB[i], blob.value[i]);
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_BLOB,
                  tuple.getTypeOfIndex(3));

      do_check_false(tuple.getIsNull(4));
      do_check_eq(tuple.getResultByName("id"), tuple.getResultByIndex(4));
      do_check_eq(INTEGER, tuple.getResultByName("id"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_INTEGER,
                  tuple.getTypeOfIndex(4));
    }]);
  stmt.finalize();
  run_next_test();
}

function test_tuple_out_of_bounds()
{
  var stmt = makeTestStatement(
    "SELECT string FROM test"
  );
  execAsync(stmt, {}, [
    function(tuple) {
      do_check_neq(null, tuple);

      
      var methods = [
        "getTypeOfIndex",
        "getInt32",
        "getInt64",
        "getDouble",
        "getUTF8String",
        "getString",
        "getIsNull",
      ];
      for (var i in methods) {
        try {
          tuple[methods[i]](tuple.numEntries);
          do_throw("did not throw :(");
        }
        catch (e) {
          do_check_eq(Cr.NS_ERROR_ILLEGAL_VALUE, e.result);
        }
      }

      
      try {
        var blob = { value: null };
        var size = { value: 0 };
        tuple.getBlob(tuple.numEntries, blob, size);
        do_throw("did not throw :(");
      }
      catch (e) {
        do_check_eq(Cr.NS_ERROR_ILLEGAL_VALUE, e.result);
      }
    }]);
  stmt.finalize();
  run_next_test();
}

function test_no_listener_works_on_success()
{
  var stmt = makeTestStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  stmt.executeAsync();
  stmt.finalize();

  
  run_next_test();
}

function test_no_listener_works_on_results()
{
  var stmt = makeTestStatement(
    "SELECT ?"
  );
  stmt.bindInt32Parameter(0, 1);
  stmt.executeAsync();
  stmt.finalize();

  
  run_next_test();
}

function test_no_listener_works_on_error()
{
  
  var stmt = makeTestStatement(
    "COMMIT"
  );
  stmt.executeAsync();
  stmt.finalize();

  
  run_next_test();
}

function test_partial_listener_works()
{
  var stmt = makeTestStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  stmt.executeAsync({
    handleResult: function(aResultSet)
    {
    }
  });
  stmt.executeAsync({
    handleError: function(aError)
    {
    }
  });
  stmt.executeAsync({
    handleCompletion: function(aReason)
    {
    }
  });
  stmt.finalize();

  
  run_next_test();
}







function test_immediate_cancellation()
{
  var stmt = makeTestStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  execAsync(stmt, {cancel: true});
  stmt.finalize();
  run_next_test();
}




function test_double_cancellation()
{
  var stmt = makeTestStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  let pendingStatement = execAsync(stmt, {cancel: true});
  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() pendingStatement.cancel());

  stmt.finalize();
  run_next_test();
}





function test_cancellation_after_execution()
{
  var stmt = makeTestStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  let pendingStatement = execAsync(stmt, {returnPending: true});
  
  
  pendingStatement.cancel();

  stmt.finalize();
  run_next_test();
}







function test_double_execute()
{
  var stmt = makeTestStatement(
    "SELECT 1"
  );
  execAsync(stmt, null, 1);
  execAsync(stmt, null, 1);
  stmt.finalize();
  run_next_test();
}

function test_finalized_statement_does_not_crash()
{
  var stmt = makeTestStatement(
    "SELECT * FROM TEST"
  );
  stmt.finalize();
  
  try {
    stmt.executeAsync();
  }
  catch (ex) {}

  
  run_next_test();
}




function test_bind_direct_binding_params_by_index()
{
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (?, ?, ?, ?, ?)"
  );
  let insertId = nextUniqueId++;
  stmt.bindByIndex(0, insertId);
  stmt.bindByIndex(1, TEXT);
  stmt.bindByIndex(2, REAL);
  stmt.bindByIndex(3, null);
  stmt.bindBlobByIndex(4, BLOB, BLOB.length);
  execAsync(stmt);
  stmt.finalize();
  verifyQuery("SELECT string, number, nuller, blober FROM test WHERE id = ?",
              insertId,
              [TEXT, REAL, null, BLOB]);
  run_next_test();
}




function test_bind_direct_binding_params_by_name()
{
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (:int, :text, :real, :null, :blob)"
  );
  let insertId = nextUniqueId++;
  stmt.bindByName("int", insertId);
  stmt.bindByName("text", TEXT);
  stmt.bindByName("real", REAL);
  stmt.bindByName("null", null);
  stmt.bindBlobByName("blob", BLOB, BLOB.length);
  execAsync(stmt);
  stmt.finalize();
  verifyQuery("SELECT string, number, nuller, blober FROM test WHERE id = ?",
              insertId,
              [TEXT, REAL, null, BLOB]);
  run_next_test();
}

function test_bind_js_params_helper_by_index()
{
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (?, ?, ?, ?, NULL)"
  );
  let insertId = nextUniqueId++;
  
  stmt.params[3] = null;
  stmt.params[2] = REAL;
  stmt.params[1] = TEXT;
  stmt.params[0] = insertId;
  execAsync(stmt);
  stmt.finalize();
  verifyQuery("SELECT string, number, nuller FROM test WHERE id = ?", insertId,
              [TEXT, REAL, null]);
  run_next_test();
}

function test_bind_js_params_helper_by_name()
{
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (:int, :text, :real, :null, NULL)"
  );
  let insertId = nextUniqueId++;
  
  stmt.params.null = null;
  stmt.params.real = REAL;
  stmt.params.text = TEXT;
  stmt.params.int = insertId;
  execAsync(stmt);
  stmt.finalize();
  verifyQuery("SELECT string, number, nuller FROM test WHERE id = ?", insertId,
              [TEXT, REAL, null]);
  run_next_test();
}

function test_bind_multiple_rows_by_index()
{
  const AMOUNT_TO_ADD = 5;
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (?, ?, ?, ?, ?)"
  );
  var array = stmt.newBindingParamsArray();
  for (let i = 0; i < AMOUNT_TO_ADD; i++) {
    let bp = array.newBindingParams();
    bp.bindByIndex(0, INTEGER);
    bp.bindByIndex(1, TEXT);
    bp.bindByIndex(2, REAL);
    bp.bindByIndex(3, null);
    bp.bindBlobByIndex(4, BLOB, BLOB.length);
    array.addParams(bp);
    do_check_eq(array.length, i + 1);
  }
  stmt.bindParameters(array);

  let rowCount = getTableRowCount("test");
  execAsync(stmt);
  do_check_eq(rowCount + AMOUNT_TO_ADD, getTableRowCount("test"));
  stmt.finalize();
  run_next_test();
}

function test_bind_multiple_rows_by_name()
{
  const AMOUNT_TO_ADD = 5;
  var stmt = makeTestStatement(
    "INSERT INTO test (id, string, number, nuller, blober) " +
    "VALUES (:int, :text, :real, :null, :blob)"
  );
  var array = stmt.newBindingParamsArray();
  for (let i = 0; i < AMOUNT_TO_ADD; i++) {
    let bp = array.newBindingParams();
    bp.bindByName("int", INTEGER);
    bp.bindByName("text", TEXT);
    bp.bindByName("real", REAL);
    bp.bindByName("null", null);
    bp.bindBlobByName("blob", BLOB, BLOB.length);
    array.addParams(bp);
    do_check_eq(array.length, i + 1);
  }
  stmt.bindParameters(array);

  let rowCount = getTableRowCount("test");
  execAsync(stmt);
  do_check_eq(rowCount + AMOUNT_TO_ADD, getTableRowCount("test"));
  stmt.finalize();
  run_next_test();
}





function test_bind_out_of_bounds_sync_immediate()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (?)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();

  
  expectError(Cr.NS_ERROR_INVALID_ARG,
              function() bp.bindByIndex(1, INTEGER));
  
  expectError(Cr.NS_ERROR_INVALID_ARG,
              function() bp.bindBlobByIndex(1, BLOB, BLOB.length));

  stmt.finalize();
  run_next_test();
}
test_bind_out_of_bounds_sync_immediate.syncOnly = true;





function test_bind_out_of_bounds_async_deferred()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (?)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();

  
  bp.bindByIndex(1, INTEGER);
  array.addParams(bp);
  stmt.bindParameters(array);
  execAsync(stmt, {error: Ci.mozIStorageError.RANGE});

  stmt.finalize();
  run_next_test();
}
test_bind_out_of_bounds_async_deferred.asyncOnly = true;

function test_bind_no_such_name_sync_immediate()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:foo)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();

  
  expectError(Cr.NS_ERROR_INVALID_ARG,
              function() bp.bindByName("doesnotexist", INTEGER));
  
  expectError(Cr.NS_ERROR_INVALID_ARG,
              function() bp.bindBlobByName("doesnotexist", BLOB, BLOB.length));

  stmt.finalize();
  run_next_test();
}
test_bind_no_such_name_sync_immediate.syncOnly = true;

function test_bind_no_such_name_async_deferred()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:foo)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();

  bp.bindByName("doesnotexist", INTEGER);
  array.addParams(bp);
  stmt.bindParameters(array);
  execAsync(stmt, {error: Ci.mozIStorageError.RANGE});

  stmt.finalize();
  run_next_test();
}
test_bind_no_such_name_async_deferred.asyncOnly = true;

function test_bind_bogus_type_by_index()
{
  
  let stmt = makeTestStatement(
    "INSERT INTO test (blober) " +
    "VALUES (?)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();
  
  bp.bindByIndex(0, run_test);
  array.addParams(bp);
  stmt.bindParameters(array);

  execAsync(stmt, {error: Ci.mozIStorageError.MISMATCH});

  stmt.finalize();
  run_next_test();
}

function test_bind_bogus_type_by_name()
{
  
  let stmt = makeTestStatement(
    "INSERT INTO test (blober) " +
    "VALUES (:blob)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();
  
  bp.bindByName("blob", run_test);
  array.addParams(bp);
  stmt.bindParameters(array);

  execAsync(stmt, {error: Ci.mozIStorageError.MISMATCH});

  stmt.finalize();
  run_next_test();
}

function test_bind_params_already_locked()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();
  bp.bindByName("int", INTEGER);
  array.addParams(bp);

  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() bp.bindByName("int", INTEGER));

  stmt.finalize();
  run_next_test();
}

function test_bind_params_array_already_locked()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let array = stmt.newBindingParamsArray();
  let bp1 = array.newBindingParams();
  bp1.bindByName("int", INTEGER);
  array.addParams(bp1);
  let bp2 = array.newBindingParams();
  stmt.bindParameters(array);
  bp2.bindByName("int", INTEGER);

  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() array.addParams(bp2));

  stmt.finalize();
  run_next_test();
}

function test_no_binding_params_from_locked_array()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let array = stmt.newBindingParamsArray();
  let bp = array.newBindingParams();
  bp.bindByName("int", INTEGER);
  array.addParams(bp);
  stmt.bindParameters(array);

  
  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() array.newBindingParams());

  stmt.finalize();
  run_next_test();
}

function test_not_right_owning_array()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let array1 = stmt.newBindingParamsArray();
  let array2 = stmt.newBindingParamsArray();
  let bp = array1.newBindingParams();
  bp.bindByName("int", INTEGER);

  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() array2.addParams(bp));

  stmt.finalize();
  run_next_test();
}

function test_not_right_owning_statement()
{
  let stmt1 = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );
  let stmt2 = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let array1 = stmt1.newBindingParamsArray();
  let array2 = stmt2.newBindingParamsArray();
  let bp = array1.newBindingParams();
  bp.bindByName("int", INTEGER);
  array1.addParams(bp);

  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() stmt2.bindParameters(array1));

  stmt1.finalize();
  stmt2.finalize();
  run_next_test();
}

function test_bind_empty_array()
{
  let stmt = makeTestStatement(
    "INSERT INTO test (id) " +
    "VALUES (:int)"
  );

  let paramsArray = stmt.newBindingParamsArray();

  
  
  expectError(Cr.NS_ERROR_UNEXPECTED,
              function() stmt.bindParameters(paramsArray));

  stmt.finalize();
  run_next_test();
}

function test_multiple_results()
{
  let expectedResults = getTableRowCount("test");
  
  do_check_true(expectedResults > 1);

  
  let stmt = makeTestStatement("SELECT * FROM test");
  execAsync(stmt, {}, expectedResults);

  stmt.finalize();
  run_next_test();
}





const TEST_PASS_SYNC = 0;
const TEST_PASS_ASYNC = 1;









let testPass = TEST_PASS_SYNC;








function makeTestStatement(aSQL) {
  if (testPass == TEST_PASS_SYNC)
    return getOpenedDatabase().createStatement(aSQL);
  else
    return getOpenedDatabase().createAsyncStatement(aSQL);
}

var tests =
[
  test_illegal_sql_async_deferred,
  test_create_table,
  test_add_data,
  test_get_data,
  test_tuple_out_of_bounds,
  test_no_listener_works_on_success,
  test_no_listener_works_on_results,
  test_no_listener_works_on_error,
  test_partial_listener_works,
  test_immediate_cancellation,
  test_double_cancellation,
  test_cancellation_after_execution,
  test_double_execute,
  test_finalized_statement_does_not_crash,
  test_bind_direct_binding_params_by_index,
  test_bind_direct_binding_params_by_name,
  test_bind_js_params_helper_by_index,
  test_bind_js_params_helper_by_name,
  test_bind_multiple_rows_by_index,
  test_bind_multiple_rows_by_name,
  test_bind_out_of_bounds_sync_immediate,
  test_bind_out_of_bounds_async_deferred,
  test_bind_no_such_name_sync_immediate,
  test_bind_no_such_name_async_deferred,
  test_bind_bogus_type_by_index,
  test_bind_bogus_type_by_name,
  test_bind_params_already_locked,
  test_bind_params_array_already_locked,
  test_bind_empty_array,
  test_no_binding_params_from_locked_array,
  test_not_right_owning_array,
  test_not_right_owning_statement,
  test_multiple_results,
];
let index = 0;

const STARTING_UNIQUE_ID = 2;
let nextUniqueId = STARTING_UNIQUE_ID;

function run_next_test()
{
  function _run_next_test() {
    
    while (index < tests.length) {
      let test = tests[index++];
      
      if ((testPass == TEST_PASS_SYNC && ("asyncOnly" in test)) ||
          (testPass == TEST_PASS_ASYNC && ("syncOnly" in test)))
        continue;

      
      try {
        print("****** Running the next test: " + test.name);
        test();
        return;
      }
      catch (e) {
        do_throw(e);
      }
    }

    
    if (testPass == TEST_PASS_SYNC) {
      print("********* Beginning mozIStorageAsyncStatement pass.");
      testPass++;
      index = 0;
      
      asyncCleanup();
      nextUniqueId = STARTING_UNIQUE_ID;
      _run_next_test();
      return;
    }

    
    asyncCleanup();
    do_test_finished();
  }

  
  if (!_quit) {
    
    do_execute_soon(_run_next_test);
  }
}

function run_test()
{
  cleanup();

  do_test_pending();
  run_next_test();
}
