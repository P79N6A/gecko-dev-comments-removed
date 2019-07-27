














"use strict"

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/TelemetryPing.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gDatareportingService",
  () => Cc["@mozilla.org/datareporting/service;1"]
          .getService(Ci.nsISupports)
          .wrappedJSObject);


Services.prefs.setBoolPref(TelemetryPing.Constants.PREF_ENABLED, true);


Cu.import("resource://testing-common/AppInfo.jsm", this);
updateAppInfo();



add_task(function* test_firstRun() {
  yield TelemetrySession.reset();
  let metadata = TelemetrySession.getMetadata();
  do_check_false("previousBuildID" in metadata);
  let appBuildID = getAppInfo().appBuildID;
  let buildIDPref = Services.prefs.getCharPref(TelemetrySession.Constants.PREF_PREVIOUS_BUILDID);
  do_check_eq(appBuildID, buildIDPref);
});



add_task(function* test_secondRun() {
  yield TelemetrySession.reset();
  let metadata = TelemetrySession.getMetadata();
  do_check_false("previousBuildID" in metadata);
});




const NEW_BUILD_ID = "20130314";
add_task(function* test_newBuild() {
  let info = getAppInfo();
  let oldBuildID = info.appBuildID;
  info.appBuildID = NEW_BUILD_ID;
  yield TelemetrySession.reset();
  let metadata = TelemetrySession.getMetadata();
  do_check_eq(metadata.previousBuildId, oldBuildID);
  let buildIDPref = Services.prefs.getCharPref(TelemetrySession.Constants.PREF_PREVIOUS_BUILDID);
  do_check_eq(NEW_BUILD_ID, buildIDPref);
});


function run_test() {
  
  do_get_profile();

  
  
  if ("@mozilla.org/datareporting/service;1" in Cc) {
    gDatareportingService.observe(null, "app-startup", null);
    gDatareportingService.observe(null, "profile-after-change", null);
  }

  run_next_test();
}
