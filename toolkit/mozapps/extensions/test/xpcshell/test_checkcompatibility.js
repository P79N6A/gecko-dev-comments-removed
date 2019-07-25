





var ADDONS = [{
  
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test 1",
  targetApplications: [{
    id: "unknown@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
}, {
  
  
  id: "addon2@tests.mozilla.org",
  version: "1.0",
  name: "Test 2",
  targetApplications: [{
    id: "toolkit@mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
}, {
  
  
  id: "addon3@tests.mozilla.org",
  version: "1.0",
  name: "Test 3",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
}, { 
  id: "addon4@tests.mozilla.org",
  version: "1.0",
  name: "Test 4",
  targetApplications: [{
    id: "toolkit@mozilla.org",
    minVersion: "1",
    maxVersion: "2"
  }]
}, { 
  id: "addon5@tests.mozilla.org",
  version: "1.0",
  name: "Test 5",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "3"
  }]
}];

const profileDir = gProfD.clone();
profileDir.append("extensions");

var gIsNightly = false;

function run_test() {
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2.2.3", "2");

  ADDONS.forEach(function(a) {
    writeInstallRDFForExtension(a, profileDir);
  });

  var channel = "default";
  try {
    channel = Services.prefs.getCharPref("app.update.channel");
  }
  catch (e) { }

  gIsNightly = channel != "aurora" &&
               channel != "beta" &&
               channel != "release";

  startupManager();

  run_test_1();
}
















function check_state(overridden, a1, a2, a3, a4, a5) {
  do_check_neq(a1, null);
  do_check_false(a1.isActive);
  do_check_false(a1.isCompatible);

  do_check_neq(a2, null);
  if (overridden)
    do_check_true(a2.isActive);
  else
    do_check_false(a2.isActive);
  do_check_false(a2.isCompatible);

  do_check_neq(a3, null);
  if (overridden)
    do_check_true(a3.isActive);
  else
    do_check_false(a3.isActive);
  do_check_false(a3.isCompatible);

  do_check_neq(a4, null);
  do_check_true(a4.isActive);
  do_check_true(a4.isCompatible);

  do_check_neq(a5, null);
  do_check_true(a5.isActive);
  do_check_true(a5.isCompatible);
}



function run_test_1() {
  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org"],
                               function([a1, a2, a3, a4, a5]) {
    check_state(false, a1, a2, a3, a4, a5);

    run_test_2();
  });
}



function run_test_2() {
  if (gIsNightly)
    Services.prefs.setBoolPref("extensions.checkCompatibility.nightly", false);
  else
    Services.prefs.setBoolPref("extensions.checkCompatibility.2.2", false);
  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org"],
                               function([a1, a2, a3, a4, a5]) {
    check_state(true, a1, a2, a3, a4, a5);

    run_test_3();
  });
}



function run_test_3() {
  if (!gIsNightly)
    Services.prefs.setBoolPref("extensions.checkCompatibility.2.1a", false);
  restartManager("2.1a4");

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org"],
                               function([a1, a2, a3, a4, a5]) {
    check_state(true, a1, a2, a3, a4, a5);

    run_test_4();
  });
}



function run_test_4() {
  if (gIsNightly)
    Services.prefs.setBoolPref("extensions.checkCompatibility.nightly", true);
  else
    Services.prefs.setBoolPref("extensions.checkCompatibility.2.1a", true);
  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org"],
                               function([a1, a2, a3, a4, a5]) {
    check_state(false, a1, a2, a3, a4, a5);

    do_test_finished();
  });
}
