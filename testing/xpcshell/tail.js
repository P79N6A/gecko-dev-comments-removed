




































function _execute_test() {
  try {
    do_test_pending();
    run_test();
    do_test_finished();
    _do_main();
  } catch (e) {
    _fail = true;
    dump(e + "\n");
  }

  if (_fail)
    dump("*** FAIL ***\n");
  else
    dump("*** PASS ***\n");
}
