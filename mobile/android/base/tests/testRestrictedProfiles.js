




const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/ctypes.jsm");
Cu.import("resource://gre/modules/JNI.jsm");

add_task(function test_isUserRestricted() {
  
  do_check_true("@mozilla.org/parental-controls-service;1" in Cc);
  
  let pc = Cc["@mozilla.org/parental-controls-service;1"].createInstance(Ci.nsIParentalControlsService);
  
  
  
  do_check_false(pc.parentalControlsEnabled);

  
});






















run_next_test();
