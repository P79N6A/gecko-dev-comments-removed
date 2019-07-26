



"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

do_register_cleanup(function() {
  Services.prefs.setBoolPref("toolkit.osfile.log", false);
});

function run_test() {
  Services.prefs.setBoolPref("toolkit.osfile.log", true);

  run_next_test();
}




add_task(function() {
  
  
  do_get_profile();

  let dir = OS.Path.join(OS.Constants.Path.profileDir, "directory");

  
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.removeEmptyDir(dir);

  
  yield OS.File.removeEmptyDir(dir, {ignoreAbsent: true});

  
  let exception = null;
  try {
    yield OS.File.removeEmptyDir(dir, {ignoreAbsent: false});
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);
  do_check_true(exception.becauseNoSuchFile);

  
  yield OS.File.makeDir(dir);
  yield OS.File.removeEmptyDir(dir);
  do_check_false((yield OS.File.exists(dir)));
});
