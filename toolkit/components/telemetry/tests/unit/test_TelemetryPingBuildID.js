














Components.utils.import("resource://gre/modules/Services.jsm");


Services.scriptloader.loadSubScript("resource://gre/components/TelemetryPing.js");


Services.prefs.setBoolPref(PREF_ENABLED, true);


Cu.import("resource://testing-common/AppInfo.jsm");
updateAppInfo();



function testFirstRun() {
  let ping = new TelemetryPing();
  ping.setup();
  let metadata = ping.getMetadata();
  do_check_false("previousBuildID" in metadata);
  let appBuildID = getAppInfo().appBuildID;
  let buildIDPref = Services.prefs.getCharPref(PREF_PREVIOUS_BUILDID);
  do_check_eq(appBuildID, buildIDPref);
}



function testSecondRun() {
  let ping = new TelemetryPing();
  ping.setup();
  let metadata = ping.getMetadata();
  do_check_false("previousBuildID" in metadata);
}





const NEW_BUILD_ID = "20130314";
function testNewBuild() {
  let info = getAppInfo();
  let oldBuildID = info.appBuildID;
  info.appBuildID = NEW_BUILD_ID;
  let ping = new TelemetryPing();
  ping.setup();
  let metadata = ping.getMetadata();
  do_check_eq(metadata.previousBuildID, oldBuildID);
  let buildIDPref = Services.prefs.getCharPref(PREF_PREVIOUS_BUILDID);
  do_check_eq(NEW_BUILD_ID, buildIDPref);
}


function run_test() {
  
  do_get_profile();
  testFirstRun();
  testSecondRun();
  testNewBuild();
}
