


const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyGetter(this, "gDatareportingService",
  () => Cc["@mozilla.org/datareporting/service;1"]
          .getService(Ci.nsISupports)
          .wrappedJSObject);




Cu.import("resource://testing-common/AppInfo.jsm");
updateAppInfo();

let gGlobalScope = this;
function loadAddonManager() {
  let ns = {};
  Cu.import("resource://gre/modules/Services.jsm", ns);
  let head = "../../../mozapps/extensions/test/xpcshell/head_addons.js";
  let file = do_get_file(head);
  let uri = ns.Services.io.newFileURI(file);
  ns.Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  startupManager();
}

function getSimpleMeasurementsFromTelemetryController() {
  return TelemetrySession.getPayload().simpleMeasurements;
}

function initialiseTelemetry() {
  
  
  if ("@mozilla.org/datareporting/service;1" in Cc) {
    gDatareportingService.observe(null, "app-startup", null);
    gDatareportingService.observe(null, "profile-after-change", null);
  }

  return TelemetryController.setup().then(TelemetrySession.setup);
}

function run_test() {
  
  loadAddonManager();
  
  do_get_profile();

  do_test_pending();
  const Telemetry = Services.telemetry;
  Telemetry.asyncFetchTelemetryData(run_next_test);
}

add_task(function* actualTest() {
  yield initialiseTelemetry();

  
  let tmp = {};
  Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", tmp);
  let TelemetryTimestamps = tmp.TelemetryTimestamps;
  let now = Date.now();
  TelemetryTimestamps.add("foo");
  do_check_true(TelemetryTimestamps.get().foo != null); 
  do_check_true(TelemetryTimestamps.get().foo >= now); 

  
  
  
  const YEAR_4000_IN_MS = 64060588800000;
  TelemetryTimestamps.add("bar", YEAR_4000_IN_MS);
  do_check_eq(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS); 

  
  TelemetryTimestamps.add("bar", 2);
  do_check_eq(TelemetryTimestamps.get().bar, YEAR_4000_IN_MS); 

  let threw = false;
  try {
    TelemetryTimestamps.add("baz", "this isn't a number");
  } catch (ex) {
    threw = true;
  }
  do_check_true(threw); 
  do_check_null(TelemetryTimestamps.get().baz); 

  
  let simpleMeasurements = getSimpleMeasurementsFromTelemetryController();
  do_check_true(simpleMeasurements != null); 
  do_check_true(simpleMeasurements.foo > 1); 
  do_check_true(simpleMeasurements.bar > 1); 
  do_check_eq(undefined, simpleMeasurements.baz); 

  yield TelemetrySession.shutdown(false);

  do_test_finished();
});
