







add_task(function test_invalid_file() {
  
  let data = "[}";
  yield OS.File.writeAtomic(sessionCheckpointsPath, data,
                            {tmpPath: sessionCheckpointsPath + ".tmp"});

  CrashMonitor.init();
  let checkpoints = yield CrashMonitor.previousCheckpoints;
  do_check_eq(checkpoints, null);
});
