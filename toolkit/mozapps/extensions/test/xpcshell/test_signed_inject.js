
Services.prefs.setBoolPref(PREF_XPI_SIGNATURES_REQUIRED, true);

Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);

const DATA = "data/signing_checks/";
const ADDONS = {
  bootstrap: {
    unsigned: "unsigned_bootstrap_2.xpi",
    badid: "signed_bootstrap_badid_2.xpi",
    signed: "signed_bootstrap_2.xpi",
    preliminary: "preliminary_bootstrap_2.xpi",
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


function breakAddon(file) {
  if (TEST_UNPACKED) {
    file.append("test.txt");
    file.remove(true);
  }
  else {
    var zipW = AM_Cc["@mozilla.org/zipwriter;1"].
               createInstance(AM_Ci.nsIZipWriter);
    zipW.open(file, FileUtils.MODE_RDWR | FileUtils.MODE_APPEND);
    zipW.removeEntry("test.txt", false);
    zipW.close();
  }
}

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

function clearCache(file) {
  if (TEST_UNPACKED)
    return;

  Services.obs.notifyObservers(file, "flush-cache-entry", null);
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


add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.unsigned), profileDir, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_MISSING);
  do_check_eq(getActiveVersion(), -1);

  addon.uninstall();
  yield promiseShutdownManager();
  resetPrefs();

  do_check_false(file.exists());
  clearCache(file);
});

add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.signed), profileDir, ID);
  breakAddon(file);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_eq(getActiveVersion(), -1);

  addon.uninstall();
  yield promiseShutdownManager();
  resetPrefs();

  do_check_false(file.exists());
  clearCache(file);
});

add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.badid), profileDir, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_eq(getActiveVersion(), -1);

  addon.uninstall();
  yield promiseShutdownManager();
  resetPrefs();

  do_check_false(file.exists());
  clearCache(file);
});


add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.signed), profileDir, ID);

  
  
  
  yield promiseSetExtensionModifiedTime(file.path, Date.now() - 600000);

  startupManager();
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_false(addon.appDisabled);
  do_check_true(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_SIGNED);
  do_check_eq(getActiveVersion(), 2);

  yield promiseShutdownManager();
  do_check_eq(getActiveVersion(), 0);

  clearCache(file);
  breakAddon(file);
  resetPrefs();

  startupManager();

  addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_eq(getActiveVersion(), -1);

  let ids = AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_DISABLED);
  do_check_eq(ids.length, 1);
  do_check_eq(ids[0], ID);

  addon.uninstall();
  yield promiseShutdownManager();
  resetPrefs();

  do_check_false(file.exists());
  clearCache(file);
});


add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.nonbootstrap.unsigned), profileDir, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_MISSING);
  do_check_false(isExtensionInAddonsList(profileDir, ID));

  addon.uninstall();
  yield promiseRestartManager();
  yield promiseShutdownManager();

  do_check_false(file.exists());
  clearCache(file);
});

add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.nonbootstrap.signed), profileDir, ID);
  breakAddon(file);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_false(isExtensionInAddonsList(profileDir, ID));

  addon.uninstall();
  yield promiseRestartManager();
  yield promiseShutdownManager();

  do_check_false(file.exists());
  clearCache(file);
});

add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.nonbootstrap.badid), profileDir, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_false(isExtensionInAddonsList(profileDir, ID));

  addon.uninstall();
  yield promiseRestartManager();
  yield promiseShutdownManager();

  do_check_false(file.exists());
  clearCache(file);
});


add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.nonbootstrap.signed), profileDir, ID);

  
  
  
  yield promiseSetExtensionModifiedTime(file.path, Date.now() - 60000);

  startupManager();
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_false(addon.appDisabled);
  do_check_true(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_SIGNED);
  do_check_true(isExtensionInAddonsList(profileDir, ID));

  yield promiseShutdownManager();

  clearCache(file);
  breakAddon(file);

  startupManager();

  addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_BROKEN);
  do_check_false(isExtensionInAddonsList(profileDir, ID));

  let ids = AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_DISABLED);
  do_check_eq(ids.length, 1);
  do_check_eq(ids[0], ID);

  addon.uninstall();
  yield promiseRestartManager();
  yield promiseShutdownManager();

  do_check_false(file.exists());
  clearCache(file);
});


add_task(function*() {
  startupManager();
  yield promiseInstallAllFiles([do_get_file(DATA + ADDONS.nonbootstrap.signed)]);
  yield promiseShutdownManager();

  let staged = profileDir.clone();
  staged.append("staged");
  staged.append(do_get_expected_addon_name(ID));
  do_check_true(staged.exists());

  breakAddon(staged);
  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_eq(addon, null);

  clearCache(staged);

  yield promiseShutdownManager();
});


add_task(function*() {
  let stage = profileDir.clone();
  stage.append("staged");

  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.signed), stage, ID);
  breakAddon(file);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_eq(addon, null);
  do_check_eq(getActiveVersion(), -1);

  do_check_false(file.exists());
  clearCache(file);

  yield promiseShutdownManager();
  resetPrefs();
});


add_task(function*() {
  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.preliminary), profileDir, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_neq(addon, null);
  do_check_true(addon.appDisabled);
  do_check_false(addon.isActive);
  do_check_eq(addon.signedState, AddonManager.SIGNEDSTATE_PRELIMINARY);
  do_check_eq(getActiveVersion(), -1);

  addon.uninstall();
  yield promiseShutdownManager();
  resetPrefs();

  do_check_false(file.exists());
  clearCache(file);
});

add_task(function*() {
  let stage = profileDir.clone();
  stage.append("staged");

  let file = manuallyInstall(do_get_file(DATA + ADDONS.bootstrap.preliminary), stage, ID);

  startupManager();

  
  let addon = yield promiseAddonByID(ID);
  do_check_eq(addon, null);
  do_check_eq(getActiveVersion(), -1);

  do_check_false(file.exists());
  clearCache(file);

  yield promiseShutdownManager();
  resetPrefs();
});
