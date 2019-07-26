







add_task(function test_register() {
  let cm = Components.classes["@mozilla.org/toolkit/crashmonitor;1"]
                             .createInstance(Components.interfaces.nsIObserver);

  
  cm.observe(null, "profile-after-change", null);

  
  
  try {
    CrashMonitor.init();
    do_check_true(false);
  } catch (ex) {
    do_check_true(true);
  }
});
