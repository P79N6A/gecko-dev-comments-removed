






var addon1 = {
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");

var gAddon;


function run_test() {
  do_test_pending();

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  var dest = profileDir.clone();
  dest.append("addon1@tests.mozilla.org");
  writeInstallRDFToDir(addon1, dest);

  startupManager();

  run_test_1();
}



function run_test_1() {
  var count = 0;

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.name, "Test 1");

    if (count == 0)
      gAddon = a1;
    else
      do_check_eq(a1, gAddon);
    count++;
    if (count == 4)
      run_test_2();
  });

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.name, "Test 1");

    if (count == 0)
      gAddon = a1;
    else
      do_check_eq(a1, gAddon);
    count++;
    if (count == 4)
      run_test_2();
  });

  do_execute_soon(function() {
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_neq(a1, null);
      do_check_eq(a1.name, "Test 1");

      if (count == 0)
        gAddon = a1;
      else
        do_check_eq(a1, gAddon);
      count++;
      if (count == 4)
        run_test_2();
    });
  });

  do_execute_soon(function() {
    do_execute_soon(function() {
      AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
        do_check_neq(a1, null);
        do_check_eq(a1.name, "Test 1");

        if (count == 0)
          gAddon = a1;
        else
          do_check_eq(a1, gAddon);
        count++;
        if (count == 4)
          run_test_2();
      });
    });
  });
}


function run_test_2() {
  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.name, "Test 1");

    do_check_eq(a1, gAddon);

    do_test_finished();
  });

}
