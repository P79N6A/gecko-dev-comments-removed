







add_task(function test_valid_file() {
  
  let data = JSON.stringify({"final-ui-startup": true});
  yield OS.File.writeAtomic(sessionCheckpointsPath, data,
                            {tmpPath: sessionCheckpointsPath + ".tmp"});

  CrashMonitor.init();
  let checkpoints = yield CrashMonitor.previousCheckpoints;

  do_check_true(checkpoints["final-ui-startup"]);
  do_check_eq(Object.keys(checkpoints).length, 1);
});
