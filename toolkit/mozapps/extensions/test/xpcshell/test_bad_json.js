






var addon1 = {
  id: "addon1@tests.mozilla.org",
  version: "2.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");

function run_test() {
  do_test_pending("Bad JSON");

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  writeInstallRDFForExtension(addon1, profileDir);

  startupManager();

  shutdownManager();

  
  
  saveJSON({not: "what we expect to find"}, gExtensionsJSON);

  startupManager(false);
  
  AddonManager.getAddonsByIDs([addon1.id], callback_soon(after_db_rebuild));
}

function after_db_rebuild([a1]) {
  do_check_eq(a1.id, addon1.id);

  shutdownManager();

  
  let data = loadJSON(gExtensionsJSON);
  do_check_true("schemaVersion" in data);
  do_check_eq(data.addons[0].id, addon1.id);

  do_test_finished("Bad JSON");
}
