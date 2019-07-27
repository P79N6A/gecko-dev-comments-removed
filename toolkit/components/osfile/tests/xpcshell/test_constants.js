"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Services.jsm", this);

function run_test() {
  run_next_test();
}


add_task(function* check_definition() {
  do_check_true(OS.Constants!=null);
  do_check_true(!!OS.Constants.Win || !!OS.Constants.libc);
  do_check_true(OS.Constants.Path!=null);
  do_check_true(OS.Constants.Sys!=null);
  
  if (OS.Constants.Sys.Name == "Gonk") {
  
    do_check_eq(Services.appinfo.OS, "Android");
  } else {
    do_check_eq(Services.appinfo.OS, OS.Constants.Sys.Name);
  }

  
  if (Components.classes["@mozilla.org/xpcom/debug;1"].getService(Components.interfaces.nsIDebug2).isDebugBuild == true) {
    do_check_true(OS.Constants.Sys.DEBUG);
  } else {
    do_check_true(typeof(OS.Constants.Sys.DEBUG) == 'undefined');
  }
});
