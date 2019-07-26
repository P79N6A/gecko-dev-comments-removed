



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

  let dir = OS.Constants.Path.profileDir;

  
  do_check_true((yield OS.File.exists(dir)));

  
  let availableBytes = yield OS.File.getAvailableFreeSpace(dir);

  do_check_true(!!availableBytes);
  do_check_true(availableBytes > 0);
});
