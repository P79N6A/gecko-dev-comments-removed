




const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://testing-common/httpd.js", this);
Cu.import("resource://gre/modules/PromiseUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/TelemetryFile.jsm", this);
Cu.import("resource://gre/modules/TelemetryPing.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);

const PREF_ENABLED = "toolkit.telemetry.enabled";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";

let gHttpServer = null;

function run_test() {
  do_test_pending();
  do_get_profile();

  Services.prefs.setBoolPref(PREF_ENABLED, true);
  Services.prefs.setBoolPref(PREF_FHR_UPLOAD_ENABLED, true);

  
  gHttpServer = new HttpServer();
  gHttpServer.start(-1);

  run_next_test();
}

add_task(function* testSendPendingOnIdleDaily() {
  
  const PENDING_PING = {
    id: "2133234d-4ea1-44f4-909e-ce8c6c41e0fc",
    type: "test-ping",
    version: 4,
    application: {},
    payload: {},
  };
  yield TelemetryFile.savePing(PENDING_PING, true);

  
  yield TelemetryPing.setup();
  TelemetryPing.setServer("http://localhost:" + gHttpServer.identity.primaryPort);

  let pendingPromise = new Promise(resolve =>
    gHttpServer.registerPrefixHandler("/submit/telemetry/", request => resolve(request)));

  let gatherPromise = PromiseUtils.defer();
  Services.obs.addObserver(gatherPromise.resolve, "gather-telemetry", false);

  
  TelemetrySession.observe(null, "idle-daily", null);
  yield gatherPromise;
  Assert.ok(true, "Received gather-telemetry notification.");

  Services.obs.removeObserver(gatherPromise.resolve, "gather-telemetry");

  
  let request = yield pendingPromise;
  let ping = decodeRequestPayload(request);

  
  Assert.equal(ping.id, PENDING_PING.id);
  Assert.equal(ping.type, PENDING_PING.type);

  gHttpServer.stop(do_test_finished);
});
