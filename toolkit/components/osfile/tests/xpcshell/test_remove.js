



"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

do_register_cleanup(function() {
  Services.prefs.setBoolPref("toolkit.osfile.log", false);
});

function run_test() {
  Services.prefs.setBoolPref("toolkit.osfile.log", true);
  run_next_test();
}

add_task(function* test_ignoreAbsent() {
  let absent_file_name = "test_osfile_front_absent.tmp";

  
  yield Assert.rejects(OS.File.remove(absent_file_name, {ignoreAbsent: false}),
                       "OS.File.remove throws if there is no such file.");

  
  
  let exception = null;
  try {
    yield OS.File.remove(absent_file_name, {ignoreAbsent: true});
    yield OS.File.remove(absent_file_name);
  } catch (ex) {
    exception = ex;
  }
  Assert.ok(!exception, "OS.File.remove should not throw when not requested.");
});
