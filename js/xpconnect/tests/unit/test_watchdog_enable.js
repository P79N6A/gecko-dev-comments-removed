



function testBody() {
  setWatchdogEnabled(true);
  checkWatchdog(true, continueTest);
  yield;
  do_test_finished();
  yield;
}
