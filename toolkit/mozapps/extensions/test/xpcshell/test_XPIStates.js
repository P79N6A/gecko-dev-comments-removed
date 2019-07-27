





Components.utils.import("resource://gre/modules/Promise.jsm");

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

const profileDir = gProfD.clone();
profileDir.append("extensions");




function run_test() {
  
  do_register_cleanup(promiseShutdownManager);
  
  run_next_test();
}



writeInstallRDFToXPI({
  id: "packed-enabled@tests.mozilla.org",
  version: "1.0",
  bootstrap: true,
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }],
  name: "Packed, Enabled",
}, profileDir);


writeInstallRDFToXPI({
  id: "packed-disabled@tests.mozilla.org",
  version: "1.0",
  bootstrap: true,
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }],
  name: "Packed, Disabled",
}, profileDir);


writeInstallRDFToDir({
  id: "unpacked-enabled@tests.mozilla.org",
  version: "1.0",
  bootstrap: true,
  unpack: true,
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }],
  name: "Unpacked, Enabled",
}, profileDir, null, "extraFile.js");



writeInstallRDFToDir({
  id: "unpacked-disabled@tests.mozilla.org",
  version: "1.0",
  bootstrap: true,
  unpack: true,
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }],
  name: "Unpacked, disabled",
}, profileDir, null, "extraFile.js");




let lastTimestamp = Date.now();







function checkChange(XS, aPath, aChange) {
  do_check_true(aPath.exists());
  lastTimestamp += 10000;
  do_print("Touching file " + aPath.path + " with " + lastTimestamp);
  aPath.lastModifiedTime = lastTimestamp;
  do_check_eq(XS.getInstallState(), aChange);
  
  XS.save();
}


function getXS() {
  let XPI = Components.utils.import("resource://gre/modules/addons/XPIProvider.jsm");
  return XPI.XPIStates;
}

add_task(function* detect_touches() {
  startupManager();
  let [pe, pd, ue, ud] = yield promiseAddonsByIDs([
         "packed-enabled@tests.mozilla.org",
         "packed-disabled@tests.mozilla.org",
         "unpacked-enabled@tests.mozilla.org",
         "unpacked-disabled@tests.mozilla.org"
         ]);

  do_print("Disable test add-ons");
  pd.userDisabled = true;
  ud.userDisabled = true;

  let XS = getXS();

  
  do_check_false(XS.getInstallState());

  let states = XS.getLocation("app-profile");

  
  do_check_true(states.get("packed-enabled@tests.mozilla.org").enabled);
  do_check_false(states.get("packed-disabled@tests.mozilla.org").enabled);
  do_check_true(states.get("unpacked-enabled@tests.mozilla.org").enabled);
  do_check_false(states.get("unpacked-disabled@tests.mozilla.org").enabled);

  

  
  let peFile = profileDir.clone();
  peFile.append("packed-enabled@tests.mozilla.org.xpi");
  checkChange(XS, peFile, true);

  
  let pdFile = profileDir.clone();
  pdFile.append("packed-disabled@tests.mozilla.org.xpi");
  checkChange(XS, pdFile, true);

  
  let ueDir = profileDir.clone();
  ueDir.append("unpacked-enabled@tests.mozilla.org");
  let manifest = ueDir.clone();
  manifest.append("install.rdf");
  checkChange(XS, manifest, true);
  
  let otherFile = ueDir.clone();
  otherFile.append("extraFile.js");
  checkChange(XS, otherFile, true);

  
  let udDir = profileDir.clone();
  udDir.append("unpacked-disabled@tests.mozilla.org");
  manifest = udDir.clone();
  manifest.append("install.rdf");
  checkChange(XS, manifest, true);
  
  
  otherFile = udDir.clone();
  otherFile.append("extraFile.js");
  checkChange(XS, otherFile, false);

  




  ud.userDisabled = false;
  let xState = XS.getAddon("app-profile", ud.id);
  do_check_true(xState.enabled);
  do_check_eq(xState.scanTime, ud.updateDate.getTime());
});





add_task(function* uninstall_bootstrap() {
  let [pe, pd, ue, ud] = yield promiseAddonsByIDs([
         "packed-enabled@tests.mozilla.org",
         "packed-disabled@tests.mozilla.org",
         "unpacked-enabled@tests.mozilla.org",
         "unpacked-disabled@tests.mozilla.org"
         ]);
  pe.uninstall();
  let xpiState = Services.prefs.getCharPref("extensions.xpiState");
  do_check_false(xpiState.includes("\"packed-enabled@tests.mozilla.org\""));
});




add_task(function* install_bootstrap() {
  let XS = getXS();

  let installer = yield new Promise((resolve, reject) =>
    AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_1"), resolve));

  let promiseInstalled = new Promise((resolve, reject) => {
    AddonManager.addInstallListener({
      onInstallFailed: reject,
      onInstallEnded: (install, newAddon) => resolve(newAddon)
    });
  });

  installer.install();

  let newAddon = yield promiseInstalled;
  let xState = XS.getAddon("app-profile", newAddon.id);
  do_check_true(!!xState);
  do_check_true(xState.enabled);
  do_check_eq(xState.scanTime, newAddon.updateDate.getTime());
  newAddon.uninstall();
});








add_task(function* install_restart() {
  let XS = getXS();

  let installer = yield new Promise((resolve, reject) =>
    AddonManager.getInstallForFile(do_get_addon("test_bootstrap1_4"), resolve));

  let promiseInstalled = new Promise((resolve, reject) => {
    AddonManager.addInstallListener({
      onInstallFailed: reject,
      onInstallEnded: (install, newAddon) => resolve(newAddon)
    });
  });

  installer.install();

  let newAddon = yield promiseInstalled;
  let newID = newAddon.id;
  let xState = XS.getAddon("app-profile", newID);
  do_check_false(xState);

  
  
  XS = null;
  newAddon = null;
  yield promiseRestartManager();
  XS = getXS();

  newAddon = yield promiseAddonByID(newID);
  xState = XS.getAddon("app-profile", newID);
  do_check_true(xState);
  do_check_true(xState.enabled);
  do_check_eq(xState.scanTime, newAddon.updateDate.getTime());

  
  
  newAddon.userDisabled = true;
  do_check_false(xState.enabled);
  XS = null;
  newAddon = null;
  yield promiseRestartManager();
  XS = getXS();
  xState = XS.getAddon("app-profile", newID);
  do_check_true(xState);
  do_check_false(xState.enabled);

  newAddon = yield promiseAddonByID(newID);
  newAddon.userDisabled = false;
  do_check_true(xState.enabled);
  XS = null;
  newAddon = null;
  yield promiseRestartManager();
  XS = getXS();
  xState = XS.getAddon("app-profile", newID);
  do_check_true(xState);
  do_check_true(xState.enabled);

  
  
  newAddon = yield promiseAddonByID(newID);
  newAddon.uninstall();
  xState = XS.getAddon("app-profile", newID);
  do_check_true(xState);
  do_check_false(xState.enabled);

  
  XS = null;
  newAddon = null;
  yield promiseRestartManager();
  XS = getXS();
  xState = XS.getAddon("app-profile", newID);
  do_check_false(xState);
});
