







add_task(function test_missing_file() {
  CrashMonitor.init();
  let checkpoints = yield CrashMonitor.previousCheckpoints;
  do_check_eq(checkpoints, null);
});
