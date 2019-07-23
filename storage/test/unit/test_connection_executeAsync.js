






































const INTEGER = 1;
const TEXT = "this is test text";
const REAL = 3.23;
const BLOB = [1, 2];

function test_create_and_add()
{
  dump("test_create_and_add()\n");

  getOpenedDatabase().executeSimpleSQL(
    "CREATE TABLE test (" +
      "id INTEGER PRIMARY KEY, " +
      "string TEXT, " +
      "number REAL, " +
      "nuller NULL, " +
      "blober BLOB" +
    ")"
  );

  let stmts = [];
  stmts[0] = getOpenedDatabase().createStatement(
    "INSERT INTO test (id, string, number, nuller, blober) VALUES (?, ?, ?, ?, ?)"
  );
  stmts[0].bindInt32Parameter(0, INTEGER);
  stmts[0].bindStringParameter(1, TEXT);
  stmts[0].bindDoubleParameter(2, REAL);
  stmts[0].bindNullParameter(3);
  stmts[0].bindBlobParameter(4, BLOB, BLOB.length);
  stmts[1] = getOpenedDatabase().createStatement(
    "INSERT INTO test (string, number, nuller, blober) VALUES (?, ?, ?, ?)"
  );
  stmts[1].bindStringParameter(0, TEXT);
  stmts[1].bindDoubleParameter(1, REAL);
  stmts[1].bindNullParameter(2);
  stmts[1].bindBlobParameter(3, BLOB, BLOB.length);

  do_test_pending();
  getOpenedDatabase().executeAsync(stmts, stmts.length, {
    handleResult: function(aResultSet)
    {
      dump("handleResult("+aResultSet+")\n");
      do_throw("unexpected results obtained!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError.result+")\n");
      do_throw("unexpected error!");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+")\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);

      
      let stmt = getOpenedDatabase().createStatement(
        "SELECT string, number, nuller, blober FROM test WHERE id = ?"
      );
      stmt.bindInt32Parameter(0, INTEGER);
      try {
        do_check_true(stmt.executeStep());
        do_check_eq(TEXT, stmt.getString(0));
        do_check_eq(REAL, stmt.getDouble(1));
        do_check_true(stmt.getIsNull(2));
        let count = { value: 0 };
        let blob = { value: null };
        stmt.getBlob(3, count, blob);
        do_check_eq(BLOB.length, count.value);
        for (let i = 0; i < BLOB.length; i++)
          do_check_eq(BLOB[i], blob.value[i]);
      }
      finally {
        stmt.finalize();
      }

      
      stmt = getOpenedDatabase().createStatement(
        "SELECT COUNT(1) FROM test"
      );
      try {
        do_check_true(stmt.executeStep());
        do_check_eq(2, stmt.getInt32(0));
      }
      finally {
        stmt.finalize();
      }

      do_test_finished();
    }
  });
  stmts[0].finalize();
  stmts[1].finalize();
}

function test_transaction_created()
{
  dump("test_transaction_created()\n");

  let stmts = [];
  stmts[0] = getOpenedDatabase().createStatement(
    "BEGIN"
  );
  stmts[1] = getOpenedDatabase().createStatement(
    "SELECT * FROM test"
  );

  do_test_pending()
  getOpenedDatabase().executeAsync(stmts, stmts.length, {
    handleResult: function(aResultSet)
    {
      dump("handleResults("+aResultSet+")\n");
      do_throw("unexpected results obtained!");
    },
    handleError: function(aError)
    {
      dump("handleError("+aError.result+")\n");
    },
    handleCompletion: function(aReason)
    {
      dump("handleCompletion("+aReason+")\n");
      do_check_eq(Ci.mozIStorageStatementCallback.REASON_ERROR, aReason);
      do_test_finished();
    }
  });
  stmts[0].finalize();
  stmts[1].finalize();
}


let tests =
[
  test_create_and_add,
  test_transaction_created,
];

function run_test()
{
  cleanup();

  for (let i = 0; i < tests.length; i++)
    tests[i]();
}
