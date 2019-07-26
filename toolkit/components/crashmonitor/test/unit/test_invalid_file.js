







add_task(function test_invalid_file() {
  
  let data = "1234";
  yield OS.File.writeAtomic(sessionCheckpointsPath, data,
                            {tmpPath: sessionCheckpointsPath + ".tmp"});

  
  try {
    let status = yield CrashMonitor.init();
    do_check_true(false);
  } catch (ex) {
    do_check_true(true);
  }

  
  try {
    let checkpoints = yield CrashMonitor.previousCheckpoints;
    do_check_true(false);
  } catch (ex) {
    do_check_true(true);
  }
});
