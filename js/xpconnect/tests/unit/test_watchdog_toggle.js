



function testBody() {
  var defaultBehavior = isWatchdogEnabled();
  setWatchdogEnabled(!defaultBehavior);
  setWatchdogEnabled(defaultBehavior);
  checkWatchdog(defaultBehavior, continueTest);
  yield;
  do_test_finished();
  yield;
}
