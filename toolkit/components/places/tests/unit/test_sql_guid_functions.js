













function check_invariants(aGuid)
{
  print("TEST-INFO | " + tests[index - 1].name + " | Checking guid '" +
        aGuid + "'");

  do_check_valid_places_guid(aGuid);
}




function test_guid_invariants()
{
  const kExpectedChars = 64;
  const kAllowedChars =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
  do_check_eq(kAllowedChars.length, kExpectedChars);
  let checkedChars = {};
  for (let i = 0; i < kAllowedChars; i++) {
    checkedChars[kAllowedChars[i]] = false;
  }

  
  let seenChars = 0;
  let stmt = DBConn().createStatement("SELECT GENERATE_GUID()");
  while (seenChars != kExpectedChars) {
    do_check_true(stmt.executeStep());
    let guid = stmt.getString(0);
    check_invariants(guid);

    for (let i = 0; i < guid.length; i++) {
      if (!checkedChars[guid[i]]) {
        checkedChars[guid[i]] = true;
        seenChars++;
      }
    }
    stmt.reset();
  }
  stmt.finalize();

  
  for (let i = 0; i < kAllowedChars; i++) {
    do_check_true(checkedChars[kAllowedChars[i]]);
  }

  run_next_test();
}

function test_guid_on_background()
{
  
  let stmt = DBConn().createAsyncStatement("SELECT GENERATE_GUID()");
  let checked = false;
  stmt.executeAsync({
    handleResult: function(aResult) {
      try {
        let row = aResult.getNextRow();
        check_invariants(row.getResultByIndex(0));
        do_check_eq(aResult.getNextRow(), null);
        checked = true;
      }
      catch (e) {
        do_throw(e);
      }
    },
    handleCompletion: function(aReason) {
      do_check_eq(aReason, Ci.mozIStorageStatementCallback.REASON_FINISHED);
      do_check_true(checked);
      run_next_test();
    }
  });
  stmt.finalize();
}




let tests = [
  test_guid_invariants,
  test_guid_on_background,
];
let index = 0;

function run_next_test()
{
  function _run_next_test() {
    if (index < tests.length) {
      do_test_pending();
      print("TEST-INFO | " + _TEST_FILE + " | Starting " + tests[index].name);

      
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
  do_test_pending();
  run_next_test();
}
