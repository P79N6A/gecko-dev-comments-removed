







add_task(function test_init() {
  CrashMonitor.init();
  try {
    CrashMonitor.init();
    do_check_true(false);
  } catch (ex) {
    do_check_true(true);
  }
});
