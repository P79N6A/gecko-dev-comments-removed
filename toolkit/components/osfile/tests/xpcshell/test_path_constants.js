



"use strict";

Cu.import("resource://gre/modules/ctypes.jsm", this);
Cu.import("resource://testing-common/AppData.jsm", this);


function run_test() {
  run_next_test();
}

function compare_paths(ospath, key) {
  let file;
  try {
    file = Services.dirsvc.get(key, Components.interfaces.nsIFile);
  } catch(ex) {}

  if (file) {
    do_check_true(!!ospath);
    do_check_eq(ospath, file.path);
  } else {
    do_print("WARNING: " + key + " is not defined. Test may not be testing anything!");
    do_check_false(!!ospath);
  }
}



add_task(function* test_before_after_profile() {
  do_check_null(OS.Constants.Path.profileDir);
  do_check_null(OS.Constants.Path.localProfileDir);
  do_check_null(OS.Constants.Path.userApplicationDataDir);

  do_get_profile();
  do_check_true(!!OS.Constants.Path.profileDir);
  do_check_true(!!OS.Constants.Path.localProfileDir);

  
  
  
  do_check_null(OS.Constants.Path.userApplicationDataDir);

  yield makeFakeAppDir();
  do_check_true(!!OS.Constants.Path.userApplicationDataDir);

  
});


add_task(function() {
  do_check_true(!!OS.Constants.Path.tmpDir);
  do_check_eq(OS.Constants.Path.tmpDir, Services.dirsvc.get("TmpD", Components.interfaces.nsIFile).path);

  do_check_true(!!OS.Constants.Path.homeDir);
  do_check_eq(OS.Constants.Path.homeDir, Services.dirsvc.get("Home", Components.interfaces.nsIFile).path);

  do_check_true(!!OS.Constants.Path.desktopDir);
  do_check_eq(OS.Constants.Path.desktopDir, Services.dirsvc.get("Desk", Components.interfaces.nsIFile).path);

  compare_paths(OS.Constants.Path.userApplicationDataDir, "UAppData");

  compare_paths(OS.Constants.Path.winAppDataDir, "AppData");
  compare_paths(OS.Constants.Path.winStartMenuProgsDir, "Progs");

  compare_paths(OS.Constants.Path.macUserLibDir, "ULibDir");
  compare_paths(OS.Constants.Path.macLocalApplicationsDir, "LocApp");
});


add_task(function() {
  ctypes.open(OS.Constants.Path.libxul);
  do_print("Linked to libxul");
});
