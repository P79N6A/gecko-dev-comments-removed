



"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

let Path = OS.Path;
let profileDir;

do_register_cleanup(function() {
  Services.prefs.setBoolPref("toolkit.osfile.log", false);
});

function run_test() {
  run_next_test();
}





add_task(function init() {
  
  
  do_get_profile();
  profileDir = OS.Constants.Path.profileDir;
  Services.prefs.setBoolPref("toolkit.osfile.log", true);
});





add_task(function* test_basic() {
  let dir = Path.join(profileDir, "directory");

  
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir);

  
  yield OS.File.stat(dir);

  
  yield OS.File.makeDir(dir);

  
  yield OS.File.makeDir(dir, {ignoreExisting: true});

  
  let exception = null;
  try {
    yield OS.File.makeDir(dir, {ignoreExisting: false});
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);
  do_check_true(exception.becauseExists);
});


add_task(function* test_root() {
  if (OS.Constants.Win) {
    yield OS.File.makeDir("C:");
    yield OS.File.makeDir("C:\\");
  } else {
    yield OS.File.makeDir("/");
  }
});




add_task(function test_option_from() {
  let dir = Path.join(profileDir, "a", "b", "c");

  
  do_check_false((yield OS.File.exists(dir)));

  
  yield OS.File.makeDir(dir, {from: profileDir});

  
  yield OS.File.stat(dir);

  
  yield OS.File.makeDir(dir);

  
  yield OS.File.makeDir(dir, {ignoreExisting: true});

  
  let exception = null;
  try {
    yield OS.File.makeDir(dir, {ignoreExisting: false});
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);
  do_check_true(exception.becauseExists);

  
  let dir2 = Path.join(profileDir, "g", "h", "i");
  exception = null;
  try {
    yield OS.File.makeDir(dir2);
  } catch (ex) {
    exception = ex;
  }

  do_check_true(!!exception);
  do_check_true(exception instanceof OS.File.Error);
  do_check_true(exception.becauseNoSuchFile);

  

  let dir3 = Path.join(profileDir, "d", "", "e", "f");
  do_check_false((yield OS.File.exists(dir3)));
  yield OS.File.makeDir(dir3, {from: profileDir});
  do_check_true((yield OS.File.exists(dir3)));

  let dir4;
  if (OS.Constants.Win) {
    
    
    dir4 = profileDir + "\\\\g";
  } else {
    dir4 = profileDir + "////g";
  }
  do_check_false((yield OS.File.exists(dir4)));
  yield OS.File.makeDir(dir4, {from: profileDir});
  do_check_true((yield OS.File.exists(dir4)));
});
