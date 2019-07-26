



function testBody() {
  setWatchdogEnabled(false);
  checkWatchdog(false, continueTest);
  yield;
  do_test_finished();
  yield;
}
