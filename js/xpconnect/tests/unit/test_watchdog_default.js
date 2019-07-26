



function testBody() {
  
  
  checkWatchdog(isWatchdogEnabled(), continueTest);
  yield;
  do_test_finished();
  yield;
}
