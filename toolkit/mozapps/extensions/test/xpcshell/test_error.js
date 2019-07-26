





Components.utils.import("resource://gre/modules/Services.jsm");

const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  startupManager();

  do_test_pending();
  run_test_1();
}


function run_test_1() {
  AddonManager.getInstallForFile(do_get_file("data/unsigned.xpi"), function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOADED);
    do_check_eq(install.error, 0);

    install.cancel();

    run_test_2();
  });
}


function run_test_2() {
  AddonManager.getInstallForFile(do_get_file("data/corrupt.xpi"), function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq(install.error, AddonManager.ERROR_CORRUPT_FILE);

    run_test_3();
  });
}


function run_test_3() {
  AddonManager.getInstallForFile(do_get_file("data/empty.xpi"), function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq(install.error, AddonManager.ERROR_CORRUPT_FILE);

    run_test_4();
  });
}


function run_test_4() {
  let url = Services.io.newFileURI(do_get_file("data/unsigned.xpi")).spec;
  AddonManager.getInstallForURL(url, function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq(install.error, AddonManager.ERROR_INCORRECT_HASH);

    run_test_5();
  }, "application/x-xpinstall", "sha1:foo");
}


function run_test_4() {
  let file = do_get_file("data");
  file.append("missing.xpi");
  AddonManager.getInstallForFile(file, function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq(install.error, AddonManager.ERROR_NETWORK_FAILURE);

    run_test_5();
  });
}


function run_test_5() {
  AddonManager.getInstallForFile(do_get_addon("test_bug567173"), function(install) {
    do_check_neq(install, null);
    do_check_eq(install.state, AddonManager.STATE_DOWNLOAD_FAILED);
    do_check_eq(install.error, AddonManager.ERROR_CORRUPT_FILE);

    do_test_finished();
  });
}
