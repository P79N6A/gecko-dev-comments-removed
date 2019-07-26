


"use strict";

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/TelemetryLog.jsm");
Cu.import("resource://gre/modules/TelemetryPing.jsm");
let bsp = Cu.import("resource:///modules/experiments/Experiments.jsm");


const FILE_MANIFEST            = "experiments.manifest";
const MANIFEST_HANDLER         = "manifests/handler";

const SEC_IN_ONE_DAY  = 24 * 60 * 60;
const MS_IN_ONE_DAY   = SEC_IN_ONE_DAY * 1000;


let gProfileDir          = null;
let gHttpServer          = null;
let gHttpRoot            = null;
let gDataRoot            = null;
let gReporter            = null;
let gPolicy              = null;
let gManifestObject      = null;
let gManifestHandlerURI  = null;

const TLOG = bsp.TELEMETRY_LOG;

let gGlobalScope = this;
function loadAddonManager() {
  let ns = {};
  Cu.import("resource://gre/modules/Services.jsm", ns);
  let head = "../../../../toolkit/mozapps/extensions/test/xpcshell/head_addons.js";
  let file = do_get_file(head);
  let uri = ns.Services.io.newFileURI(file);
  ns.Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  startupManager();
}

function checkEvent(event, id, data)
{
  do_print("Checking message " + id);
  Assert.equal(event[0], id, "id should match");
  Assert.ok(event[1] > 0, "timestamp should be greater than 0");

  if (data === undefined) {
   Assert.equal(event.length, 2, "event array should have 2 entries");
  } else {
    Assert.equal(event.length, data.length + 2, "event entry count should match expected count");
    for (var i = 0; i < data.length; ++i) {
      Assert.equal(typeof(event[i + 2]), "string", "event entry should be a string");
      Assert.equal(event[i + 2], data[i], "event entry should match expected entry");
    }
  }
}

function run_test() {
  run_next_test();
}

add_task(function* test_setup() {
  loadAddonManager();
  gProfileDir = do_get_profile();

  gHttpServer = new HttpServer();
  gHttpServer.start(-1);
  let port = gHttpServer.identity.primaryPort;
  gHttpRoot = "http://localhost:" + port + "/";
  gDataRoot = gHttpRoot + "data/";
  gManifestHandlerURI = gHttpRoot + MANIFEST_HANDLER;
  gHttpServer.registerDirectory("/data/", do_get_cwd());
  gHttpServer.registerPathHandler("/" + MANIFEST_HANDLER, (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.write(JSON.stringify(gManifestObject));
    response.processAsync();
    response.finish();
  });
  do_register_cleanup(() => gHttpServer.stop(() => {}));

  disableCertificateChecks();

  Services.prefs.setBoolPref(PREF_EXPERIMENTS_ENABLED, true);
  Services.prefs.setIntPref(PREF_LOGGING_LEVEL, 0);
  Services.prefs.setBoolPref(PREF_LOGGING_DUMP, true);
  Services.prefs.setCharPref(PREF_MANIFEST_URI, gManifestHandlerURI);
  Services.prefs.setIntPref(PREF_FETCHINTERVAL, 0);

  gReporter = yield getReporter("json_payload_simple");
  yield gReporter.collectMeasurements();
  let payload = yield gReporter.getJSONPayload(true);
  do_register_cleanup(() => gReporter._shutdown());

  gPolicy = new Experiments.Policy();
  let dummyTimer = { cancel: () => {}, clear: () => {} };
  patchPolicy(gPolicy, {
    updatechannel: () => "nightly",
    healthReportPayload: () => Promise.resolve(payload),
    oneshotTimer: (callback, timeout, thisObj, name) => dummyTimer,
  });

  yield removeCacheFile();
});



add_task(function* test_telemetryBasics() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedLogLength = 0;

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate1 = futureDate(baseDate,  50 * MS_IN_ONE_DAY);
  let endDate1   = futureDate(baseDate, 100 * MS_IN_ONE_DAY);
  let startDate2 = futureDate(baseDate, 150 * MS_IN_ONE_DAY);
  let endDate2   = futureDate(baseDate, 200 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate1),
        endTime:          dateToSeconds(endDate1),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
      {
        id:               EXPERIMENT2_ID,
        xpiURL:           gDataRoot + EXPERIMENT2_XPI_NAME,
        xpiHash:          EXPERIMENT2_XPI_SHA1,
        startTime:        dateToSeconds(startDate2),
        endTime:          dateToSeconds(endDate2),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  

  let experimentListData = [
    {
      id: EXPERIMENT2_ID,
      name: "Test experiment 2",
      description: "And yet another experiment that experiments experimentally.",
    },
    {
      id: EXPERIMENT1_ID,
      name: EXPERIMENT1_NAME,
      description: "Yet another experiment that experiments experimentally.",
    },
  ];

  let experiments = new Experiments.Experiments(gPolicy);

  
  

  let now = baseDate;
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  expectedLogLength += 2;
  let log = TelemetryPing.getPayload().log;
  do_print("Telemetry log: " + JSON.stringify(log));
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-2], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.REJECTED, EXPERIMENT1_ID, "startTime"]);
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.REJECTED, EXPERIMENT2_ID, "startTime"]);

  

  now = futureDate(startDate1, 5 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.ACTIVATED, EXPERIMENT1_ID]);

  

  now = futureDate(endDate1, 1000);
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  expectedLogLength += 2;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-2], TLOG.TERMINATION_KEY,
             [TLOG.TERMINATION.EXPIRED, EXPERIMENT1_ID]);
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.REJECTED, EXPERIMENT2_ID, "startTime"]);

  

  now = startDate2;
  defineNow(gPolicy, now);
  gManifestObject.experiments[1].xpiHash = "sha1:0000000000000000000000000000000000000000";

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.INSTALL_FAILURE, EXPERIMENT2_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[1].xpiHash = EXPERIMENT2_XPI_SHA1;

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 2, "Experiment list should have 2 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.ACTIVATED, EXPERIMENT2_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);

  yield experiments.disableExperiment(TLOG.TERMINATION.ADDON_UNINSTALLED);
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 2, "Experiment list should have 2 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.TERMINATION_KEY,
             [TLOG.TERMINATION.ADDON_UNINSTALLED, EXPERIMENT2_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].id      = EXPERIMENT3_ID;
  gManifestObject.experiments[0].endTime = dateToSeconds(futureDate(now, 50 * MS_IN_ONE_DAY));

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 3, "Experiment list should have 3 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.ACTIVATED, EXPERIMENT3_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);

  yield experiments.disableExperiment(TLOG.TERMINATION.FROM_API);
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 3, "Experiment list should have 3 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.TERMINATION_KEY,
             [TLOG.TERMINATION.FROM_API, EXPERIMENT3_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].id      = EXPERIMENT4_ID;
  gManifestObject.experiments[0].endTime = dateToSeconds(futureDate(now, 50 * MS_IN_ONE_DAY));

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 4, "Experiment list should have 4 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.ACTIVATION_KEY,
             [TLOG.ACTIVATION.ACTIVATED, EXPERIMENT4_ID]);

  

  now = futureDate(now, MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].os = "Plan9";

  yield experiments.updateManifest();
  list = yield experiments.getExperiments();
  Assert.equal(list.length, 4, "Experiment list should have 4 entries.");

  expectedLogLength += 1;
  log = TelemetryPing.getPayload().log;
  Assert.equal(log.length, expectedLogLength, "Telemetry log should have " + expectedLogLength + " entries.");
  checkEvent(log[log.length-1], TLOG.TERMINATION_KEY,
             [TLOG.TERMINATION.RECHECK, EXPERIMENT4_ID, "os"]);

  

  yield experiments.uninit();
  yield removeCacheFile();
});
