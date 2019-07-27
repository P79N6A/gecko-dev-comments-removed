




const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/ctypes.jsm");
Cu.import("resource://gre/modules/JNI.jsm");

add_task(function test_isUserRestricted() {
  
  do_check_true("@mozilla.org/parental-controls-service;1" in Cc);
  
  let pc = Cc["@mozilla.org/parental-controls-service;1"].createInstance(Ci.nsIParentalControlsService);
  
  
  
  do_check_false(pc.parentalControlsEnabled);

  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.DOWNLOAD));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.INSTALL_EXTENSION));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.INSTALL_APP));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.VISIT_FILE_URLS));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.SHARE));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.BOOKMARK));
  do_check_true(pc.isAllowed(Ci.nsIParentalControlsService.INSTALL_EXTENSION));

  run_next_test();
});

add_task(function test_getUserRestrictions() {
  
  
  let restrictions = "{}";

  var jenv = null;
  try {
    jenv = JNI.GetForThread();
    var geckoAppShell = JNI.LoadClass(jenv, "org.mozilla.gecko.RestrictedProfile", {
      static_methods: [
        { name: "getUserRestrictions", sig: "()Ljava/lang/String;" },
      ],
    });
    restrictions = JNI.ReadString(jenv, geckoAppShell.getUserRestrictions());
  } finally {
    if (jenv) {
      JNI.UnloadClasses(jenv);
    }
  }

  do_check_eq(restrictions, "{}");
});

run_next_test();
