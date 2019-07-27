
Services.prefs.setBoolPref(PREF_XPI_SIGNATURES_REQUIRED, true);

Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);

const DATA = "data/signing_checks/";
const ADDONS = {
  bootstrap: {
    unsigned: "unsigned_bootstrap_2.xpi",
    badid: "signed_bootstrap_badid_2.xpi",
    signed: "signed_bootstrap_2.xpi",
  },
  nonbootstrap: {
    unsigned: "unsigned_nonbootstrap_2.xpi",
    badid: "signed_nonbootstrap_badid_2.xpi",
    signed: "signed_nonbootstrap_2.xpi",
  }
};
const ID = "test@tests.mozilla.org";

const profileDir = gProfD.clone();
profileDir.append("extensions");

function resetPrefs() {
  Services.prefs.setIntPref("bootstraptest.active_version", -1);
  Services.prefs.setIntPref("bootstraptest.installed_version", -1);
  Services.prefs.setIntPref("bootstraptest.startup_reason", -1);
  Services.prefs.setIntPref("bootstraptest.shutdown_reason", -1);
  Services.prefs.setIntPref("bootstraptest.install_reason", -1);
  Services.prefs.setIntPref("bootstraptest.uninstall_reason", -1);
  Services.prefs.setIntPref("bootstraptest.startup_oldversion", -1);
  Services.prefs.setIntPref("bootstraptest.shutdown_newversion", -1);
  Services.prefs.setIntPref("bootstraptest.install_oldversion", -1);
  Services.prefs.setIntPref("bootstraptest.uninstall_newversion", -1);
}

function getActiveVersion() {
  return Services.prefs.getIntPref("bootstraptest.active_version");
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "4", "4");

  
  
  startupManager();
  shutdownManager();
  resetPrefs();

  run_next_test();
}



function stripDB() {
  let jData = loadJSON(gExtensionsJSON);
  jData.schemaVersion--;

  for (let addon of jData.addons)
    delete addon.signedState;

  saveJSON(jData, gExtensionsJSON);
}

function* test_breaking_migrate(addons, test, expectedSignedState) {
  
  gAppInfo.version = "4";
  startupManager(true);

  
  yield promiseInstallAllFiles([do_get_file(DATA + addons.signed)]);
  
  yield promiseRestartManager();
  yield promiseShutdownManager();
  resetPrefs();
  stripDB();

  
  
  manuallyUninstall(profileDir, ID);
  manuallyInstall(do_get_file(DATA + addons[test]), profileDir, ID);

  
  gAppInfo.version = "5";
  startupManager(true);

  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, expectedSignedState);

  
  if (addons == ADDONS.bootstrap)
    do_check_eq(getActiveVersion(), -1);
  else
    do_check_false(isExtensionInAddonsList(profileDir, ID));

  
  let changes = AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_DISABLED);
  do_check_eq(changes.length, 1);
  do_check_eq(changes[0], ID);

  addon.uninstall();
  
  yield promiseRestartManager();
  yield shutdownManager();
  resetPrefs();
}

function* test_working_migrate(addons, test, expectedSignedState) {
  
  gAppInfo.version = "4";
  startupManager(true);

  
  yield promiseInstallAllFiles([do_get_file(DATA + addons.signed)]);
  
  yield promiseRestartManager();
  yield promiseShutdownManager();
  resetPrefs();
  stripDB();

  
  
  manuallyUninstall(profileDir, ID);
  manuallyInstall(do_get_file(DATA + addons[test]), profileDir, ID);

  
  gAppInfo.version = "5";
  startupManager(true);

  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_false(addon.appDisabled);
  do_check_true(addon.isActive);
  do_check_eq(addon.signedState, expectedSignedState);

  if (addons == ADDONS.bootstrap)
    do_check_eq(getActiveVersion(), 2);
  else
    do_check_true(isExtensionInAddonsList(profileDir, ID));

  addon.uninstall();
  
  yield promiseRestartManager();
  yield shutdownManager();
  resetPrefs();
}

add_task(function*() {
  yield test_breaking_migrate(ADDONS.bootstrap, "unsigned", AddonManager.SIGNEDSTATE_MISSING);
  yield test_breaking_migrate(ADDONS.nonbootstrap, "unsigned", AddonManager.SIGNEDSTATE_MISSING);
});

add_task(function*() {
  yield test_breaking_migrate(ADDONS.bootstrap, "badid", AddonManager.SIGNEDSTATE_BROKEN);
  yield test_breaking_migrate(ADDONS.nonbootstrap, "badid", AddonManager.SIGNEDSTATE_BROKEN);
});

add_task(function*() {
  yield test_working_migrate(ADDONS.bootstrap, "signed", AddonManager.SIGNEDSTATE_SIGNED);
  yield test_working_migrate(ADDONS.nonbootstrap, "signed", AddonManager.SIGNEDSTATE_SIGNED);
});
