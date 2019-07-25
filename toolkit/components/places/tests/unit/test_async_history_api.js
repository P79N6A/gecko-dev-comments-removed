









function test_interface_exists()
{
  let history = Cc["@mozilla.org/browser/history;1"].getService(Ci.nsISupports);
  do_check_true(history instanceof Ci.mozIAsyncHistory);
  run_next_test();
}




let gTests = [
  test_interface_exists,
];

function run_test()
{
  run_next_test();
}
