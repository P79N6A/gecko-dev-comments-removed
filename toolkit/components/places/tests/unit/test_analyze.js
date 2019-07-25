





function run_test() {
  do_test_pending();

  let stmt = DBConn().createAsyncStatement(
    "SELECT ROWID FROM sqlite_stat1"
  );
  stmt.executeAsync({
    _gotResult: false,
    handleResult: function(aResultSet) {
      this._gotResult = true;
    },
    handleError: function(aError) {
      do_throw("Unexpected error (" + aError.result + "): " + aError.message);
    },
    handleCompletion: function(aReason) {
      do_check_false(this._gotResult);
       do_test_finished();
    }
  });
  stmt.finalize();
}
