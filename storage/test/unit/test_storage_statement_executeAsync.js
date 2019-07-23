






































const INTEGER = 1;
const TEXT = "this is test text";
const REAL = 3.23;
const BLOB = [1, 2];

function test_create_table()
{
  dump("test_create_table()\n");

  
  do_check_false(getOpenedDatabase().tableExists("test"));

  var stmt = getOpenedDatabase().createStatement(
    "CREATE TABLE test (" +
      "id INTEGER PRIMARY KEY, " +
      "string TEXT, " +
      "number REAL, " +
      "nuller NULL, " +
      "blober BLOB" +
    ")"
  );

  do_test_pending();
  stmt.executeAsync({
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_throw("unexpected results obtained!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);

      
      do_check_true(getOpenedDatabase().tableExists("test"));

      
      var stmt = getOpenedDatabase().createStatement(
        "SELECT id, string, number, nuller, blober FROM test"
      );
      stmt.finalize();

      
      for (var i = 0; i < tests.length; i++)
        tests[i]();

      do_test_finished();
    }
  });
  stmt.finalize();
}

function test_add_data()
{
  dump("test_add_data()\n");

  var stmt = getOpenedDatabase().createStatement(
    "INSERT INTO test (id, string, number, nuller, blober) VALUES (?, ?, ?, ?, ?)"
  );
  stmt.bindInt32Parameter(0, INTEGER);
  stmt.bindStringParameter(1, TEXT);
  stmt.bindDoubleParameter(2, REAL);
  stmt.bindNullParameter(3);
  stmt.bindBlobParameter(4, BLOB, BLOB.length);

  do_test_pending();
  stmt.executeAsync({
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_throw("unexpected results obtained!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);

      
      var stmt = getOpenedDatabase().createStatement(
        "SELECT string, number, nuller, blober FROM test WHERE id = ?"
      );
      stmt.bindInt32Parameter(0, INTEGER);
      try {
        do_check_true(stmt.executeStep());
        do_check_eq(TEXT, stmt.getString(0));
        do_check_eq(REAL, stmt.getDouble(1));
        do_check_true(stmt.getIsNull(2));
        var count = { value: 0 };
        var blob = { value: null };
        stmt.getBlob(3, count, blob);
        do_check_eq(BLOB.length, count.value);
        for (var i = 0; i < BLOB.length; i++)
          do_check_eq(BLOB[i], blob.value[i]);
      }
      finally {
        stmt.reset();
        stmt.finalize();
      }

      do_test_finished();
    }
  });
  stmt.finalize();
}

function test_get_data()
{
  dump("test_get_data()\n");

  var stmt = getOpenedDatabase().createStatement(
    "SELECT string, number, nuller, blober, id FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 1);

  do_test_pending();
  stmt.executeAsync({
    resultObtained: false,
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_check_false(this.resultObtained);
      this.resultObtained = true;

      
      var tuple = aResultSet.getNextRow();
      do_check_neq(null, tuple);

      
      do_check_eq(tuple.getResultByName("string"), tuple.getResultByIndex(0));
      do_check_eq(TEXT, tuple.getResultByName("string"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_TEXT,
                  tuple.getTypeOfIndex(0));

      do_check_eq(tuple.getResultByName("number"), tuple.getResultByIndex(1));
      do_check_eq(REAL, tuple.getResultByName("number"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_FLOAT,
                  tuple.getTypeOfIndex(1));

      do_check_eq(tuple.getResultByName("nuller"), tuple.getResultByIndex(2));
      do_check_eq(null, tuple.getResultByName("nuller"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_NULL,
                  tuple.getTypeOfIndex(2));

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

      do_check_eq(tuple.getResultByName("id"), tuple.getResultByIndex(4));
      do_check_eq(INTEGER, tuple.getResultByName("id"));
      do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_INTEGER,
                  tuple.getTypeOfIndex(4));

      
      tuple = aResultSet.getNextRow();
      do_check_eq(null, tuple);
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);
      do_check_true(this.resultObtained);
      do_test_finished();
    }
  });
  stmt.finalize();
}

function test_tuple_out_of_bounds()
{
  dump("test_tuple_out_of_bounds()\n");

  var stmt = getOpenedDatabase().createStatement(
    "SELECT string FROM test"
  );

  do_test_pending();
  stmt.executeAsync({
    resultObtained: false,
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_check_false(this.resultObtained);
      this.resultObtained = true;

      
      var tuple = aResultSet.getNextRow();
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
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);
      do_check_true(this.resultObtained);
      do_test_finished();
    }
  });
  stmt.finalize();
}

function test_no_listener_works_on_success()
{
  dump("test_no_listener_works_on_success()\n");

  var stmt = getOpenedDatabase().createStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  stmt.executeAsync();
  stmt.finalize();
}

function test_no_listener_works_on_results()
{
  dump("test_no_listener_works_on_results()\n");

  var stmt = getOpenedDatabase().createStatement(
    "SELECT ?"
  );
  stmt.bindInt32Parameter(0, 1);
  stmt.executeAsync();
  stmt.finalize();
}

function test_no_listener_works_on_error()
{
  return;
  dump("test_no_listener_works_on_error()\n");

  
  var stmt = getOpenedDatabase().createStatement(
    "COMMIT"
  );
  stmt.executeAsync();
  stmt.finalize();
}

function test_partial_listener_works()
{
  dump("test_partial_listener_works()\n");

  var stmt = getOpenedDatabase().createStatement(
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
}

function test_immediate_cancellation()
{
  dump("test_immediate_cancelation()\n");

  var stmt = getOpenedDatabase().createStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  let reason = Ci.mozIStorageStatementCallback.REASON_CANCELED;
  var pendingStatement = stmt.executeAsync({
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_throw("unexpected result!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(reason, aReason);
      do_test_finished();
    }
  });
  do_test_pending();

  
  if (!pendingStatement.cancel()) {
    
    reason = Ci.mozIStorageStatementCallback.REASON_FINISHED;
  }

  stmt.finalize();
}

function test_double_cancellation()
{
  dump("test_double_cancelation()\n");

  var stmt = getOpenedDatabase().createStatement(
    "DELETE FROM test WHERE id = ?"
  );
  stmt.bindInt32Parameter(0, 0);
  let reason = Ci.mozIStorageStatementCallback.REASON_CANCELED;
  var pendingStatement = stmt.executeAsync({
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
      do_throw("unexpected result!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(reason, aReason);
      do_test_finished();
    }
  });
  do_test_pending();

  
  if (!pendingStatement.cancel()) {
    
    reason = Ci.mozIStorageStatementCallback.REASON_FINISHED;
  }

  
  try {
    pendingStatement.cancel();
    do_throw("function call should have thrown!");
  }
  catch (e) {
    do_check_eq(Cr.NS_ERROR_UNEXPECTED, e.result);
  }

  stmt.finalize();
}

function test_double_execute()
{
  dump("test_double_execute()\n");

  var stmt = getOpenedDatabase().createStatement(
    "SELECT * FROM test"
  );

  var listener = {
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+");\n");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError+");\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+");\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);
      do_test_finished();
    }
  }
  do_test_pending();
  stmt.executeAsync(listener);
  do_test_pending();
  stmt.executeAsync(listener);
  stmt.finalize();
}

function test_finalized_statement_does_not_crash()
{
  dump("test_finalized_statement_does_not_crash()\n");

  var stmt = getOpenedDatabase().createStatement(
    "SELECT * FROM TEST"
  );
  stmt.finalize();
  
  try {
    stmt.executeAsync();
  }
  catch (ex) {}
}

var tests =
[
  test_add_data,
  test_get_data,
  test_tuple_out_of_bounds,
  test_no_listener_works_on_success,
  test_no_listener_works_on_results,
  test_no_listener_works_on_error,
  test_partial_listener_works,
  test_immediate_cancellation,
  test_double_cancellation,
  test_double_execute,
  test_finalized_statement_does_not_crash,
];

function run_test()
{
  cleanup();

  
  
  test_create_table();
}
