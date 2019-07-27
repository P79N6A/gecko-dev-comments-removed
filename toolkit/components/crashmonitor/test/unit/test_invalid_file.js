







add_task(function test_invalid_file() {
  
  let data = "1234";
  yield OS.File.writeAtomic(sessionCheckpointsPath, data,
                            {tmpPath: sessionCheckpointsPath + ".tmp"});

  
  let status = yield CrashMonitor.init();
  do_check_true(status === null ? true : false);

  
  let checkpoints = yield CrashMonitor.previousCheckpoints;
  do_check_true(checkpoints === null ? true : false);
});
