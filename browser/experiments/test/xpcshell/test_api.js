


"use strict";

Cu.import("resource://testing-common/httpd.js");
XPCOMUtils.defineLazyModuleGetter(this, "Experiments",
  "resource:///modules/experiments/Experiments.jsm");

const FILE_MANIFEST            = "experiments.manifest";
const PREF_EXPERIMENTS_ENABLED = "experiments.enabled";
const PREF_LOGGING_LEVEL       = "experiments.logging.level";
const PREF_LOGGING_DUMP        = "experiments.logging.dump";
const PREF_MANIFEST_URI        = "experiments.manifest.uri";
const PREF_FETCHINTERVAL       = "experiments.manifest.fetchIntervalSeconds";

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
let gTimerScheduleOffset = -1;

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

  gPolicy = new Experiments.Policy();
  patchPolicy(gPolicy, {
    updatechannel: () => "nightly",
    healthReportPayload: () => Promise.resolve(payload),
    oneshotTimer: (callback, timeout, thisObj, name) => gTimerScheduleOffset = timeout,
  });
});




add_task(function* test_getExperiments() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate1 = futureDate(baseDate,  50 * MS_IN_ONE_DAY);
  let endDate1   = futureDate(baseDate, 100 * MS_IN_ONE_DAY);
  let startDate2 = futureDate(baseDate, 150 * MS_IN_ONE_DAY);
  let endDate2   = futureDate(baseDate, 200 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
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
  gTimerScheduleOffset = -1;
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate1, 5 * MS_IN_ONE_DAY);
  gTimerScheduleOffset = -1;
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");

  experimentListData[1].active = true;
  experimentListData[1].endDate = now.getTime() + 10 * MS_IN_ONE_DAY;
  for (let k of Object.keys(experimentListData[1])) {
    Assert.equal(experimentListData[1][k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  Assert.equal(gTimerScheduleOffset, 10 * MS_IN_ONE_DAY,
               "Experiment re-evaluation should have been scheduled correctly.");

  

  now = futureDate(endDate1, 1000);
  gTimerScheduleOffset = -1;
  defineNow(gPolicy, now);

  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  experimentListData[1].active = false;
  experimentListData[1].endDate = now.getTime();
  for (let k of Object.keys(experimentListData[1])) {
    Assert.equal(experimentListData[1][k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  Assert.equal(gTimerScheduleOffset, startDate2 - now,
               "Experiment re-evaluation should have been scheduled correctly.");

  
  

  now = startDate2;
  gTimerScheduleOffset = -1;
  defineNow(gPolicy, now);

  experiments.notify();
  yield experiments._pendingTasksDone();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 2, "Experiment list should have 2 entries now.");

  experimentListData[0].active = true;
  experimentListData[0].endDate = now.getTime() + 10 * MS_IN_ONE_DAY;
  for (let i=0; i<experimentListData.length; ++i) {
    let entry = experimentListData[i];
    for (let k of Object.keys(entry)) {
      Assert.equal(entry[k], list[i][k],
                   "Entry " + i + " - Property '" + k + "' should match reference data.");
    }
  }

  Assert.equal(gTimerScheduleOffset, 10 * MS_IN_ONE_DAY,
               "Experiment re-evaluation should have been scheduled correctly.");

  

  now = futureDate(startDate2, 10 * MS_IN_ONE_DAY + 1000);
  gTimerScheduleOffset = -1;
  defineNow(gPolicy, now);
  experiments.notify();
  yield experiments._pendingTasksDone();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 2, "Experiment list should have 2 entries now.");

  experimentListData[0].active = false;
  experimentListData[0].endDate = now.getTime();
  for (let i=0; i<experimentListData.length; ++i) {
    let entry = experimentListData[i];
    for (let k of Object.keys(entry)) {
      Assert.equal(entry[k], list[i][k],
                   "Entry " + i + " - Property '" + k + "' should match reference data.");
    }
  }

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});

add_task(function* test_lastActiveToday() {
  let experiments = new Experiments.Experiments(gPolicy);

  replaceExperiments(experiments, FAKE_EXPERIMENTS_1);

  let e = yield experiments.getExperiments();
  Assert.equal(e.length, 1, "Monkeypatch successful.");
  Assert.equal(e[0].id, "id1", "ID looks sane");
  Assert.ok(e[0].active, "Experiment is active.");

  let lastActive = yield experiments.lastActiveToday();
  Assert.equal(e[0], lastActive, "Last active object is expected.");

  replaceExperiments(experiments, FAKE_EXPERIMENTS_2);
  e = yield experiments.getExperiments();
  Assert.equal(e.length, 2, "Monkeypatch successful.");

  defineNow(gPolicy, e[0].endDate);

  lastActive = yield experiments.lastActiveToday();
  Assert.ok(lastActive, "Have a last active experiment");
  Assert.equal(lastActive, e[0], "Last active object is expected.");

  yield experiments.uninit();
  yield removeCacheFile();
});



add_task(function* test_disableExperiment() {
  

  let startDate = new Date(2004, 10, 9, 12);
  let endDate   = futureDate(startDate, 100 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  

  let experimentInfo = {
    id: EXPERIMENT1_ID,
    name: EXPERIMENT1_NAME,
    description: "Yet another experiment that experiments experimentally.",
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = futureDate(startDate, 5 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();

  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");

  experimentInfo.active = true;
  experimentInfo.endDate = now.getTime() + 10 * MS_IN_ONE_DAY;
  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.disableExperiment(EXPERIMENT1_ID);

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  experimentInfo.active = false;
  experimentInfo.endDate = now.getTime();
  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  yield experiments.uninit();
  yield removeCacheFile();
});

add_task(function* test_disableExperimentsFeature() {
  

  let startDate = new Date(2004, 10, 9, 12);
  let endDate   = futureDate(startDate, 100 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  

  let experimentInfo = {
    id: EXPERIMENT1_ID,
    name: EXPERIMENT1_NAME,
    description: "Yet another experiment that experiments experimentally.",
  };

  let experiments = new Experiments.Experiments(gPolicy);
  Assert.equal(experiments.enabled, true, "Experiments feature should be enabled.");

  

  let now = futureDate(startDate, 5 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();

  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");

  experimentInfo.active = true;
  experimentInfo.endDate = now.getTime() + 10 * MS_IN_ONE_DAY;
  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  yield experiments._toggleExperimentsEnabled(false);
  Assert.equal(experiments.enabled, false, "Experiments feature should be disabled now.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  experimentInfo.active = false;
  experimentInfo.endDate = now.getTime();
  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  try {
    yield experiments.updateManifest();
  } catch (e) {
    
  }

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry.");

  for (let k of Object.keys(experimentInfo)) {
    Assert.equal(experimentInfo[k], list[0][k],
                 "Property " + k + " should match reference data.");
  }

  

  yield experiments.uninit();
  yield removeCacheFile();
});





add_task(function* test_installFailure() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
      {
        id:               EXPERIMENT2_ID,
        xpiURL:           gDataRoot + EXPERIMENT2_XPI_NAME,
        xpiHash:          EXPERIMENT2_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  

  let experimentListData = [
    {
      id: EXPERIMENT1_ID,
      name: EXPERIMENT1_NAME,
      description: "Yet another experiment that experiments experimentally.",
    },
    {
      id: EXPERIMENT2_ID,
      name: "Test experiment 2",
      description: "And yet another experiment that experiments experimentally.",
    },
  ];

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  
  
  
  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].xpiHash = "sha1:0000000000000000000000000000000000000000";
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT2_ID, "Experiment 2 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 2 should be active.");

  

  now = futureDate(now, 20 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  experimentListData[0].active = false;
  experimentListData[0].endDate = now;

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT2_ID, "Experiment 2 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment should not be active.");

  
  

  now = futureDate(now, 20 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].xpiHash = EXPERIMENT1_XPI_SHA1;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  experimentListData[0].active = true;
  experimentListData[0].endDate = now.getTime() + 10 * MS_IN_ONE_DAY;

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 2, "Experiment list should have 2 entries now.");

  for (let i=0; i<experimentListData.length; ++i) {
    let entry = experimentListData[i];
    for (let k of Object.keys(entry)) {
      Assert.equal(entry[k], list[i][k],
                   "Entry " + i + " - Property '" + k + "' should match reference data.");
    }
  }

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_userDisabledAndUpdated() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");
  let todayActive = yield experiments.lastActiveToday();
  Assert.ok(todayActive, "Last active for today reports a value.");
  Assert.equal(todayActive.id, list[0].id, "The entry is what we expect.");

  

  now = futureDate(now, 20 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.disableExperiment(EXPERIMENT1_ID);
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment should not be active anymore.");
  todayActive = yield experiments.lastActiveToday();
  Assert.ok(todayActive, "Last active for today still returns a value.");
  Assert.equal(todayActive.id, list[0].id, "The ID is still the same.");

  

  now = futureDate(now, 20 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  experiments._experiments.get(EXPERIMENT1_ID)._manifestData.xpiHash =
    "sha1:0000000000000000000000000000000000000000";
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, expectedObserverFireCount,
               "Experiments observer should not have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment should still be inactive.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_updateActiveExperiment() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  let todayActive = yield experiments.lastActiveToday();
  Assert.equal(todayActive, null, "No experiment active today.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");
  Assert.equal(list[0].name, EXPERIMENT1_NAME, "Experiments name should match.");
  todayActive = yield experiments.lastActiveToday();
  Assert.ok(todayActive, "todayActive() returns a value.");
  Assert.equal(todayActive.id, list[0].id, "It returns the active experiment.");

  
  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].xpiHash = EXPERIMENT1A_XPI_SHA1;
  gManifestObject.experiments[0].xpiURL = gDataRoot + EXPERIMENT1A_XPI_NAME;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should still be active.");
  Assert.equal(list[0].name, EXPERIMENT1A_NAME, "Experiments name should have been updated.");
  todayActive = yield experiments.lastActiveToday();
  Assert.equal(todayActive.id, list[0].id, "last active today is still sane.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_disableActiveExperiment() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].disabled = true;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment 1 should be disabled.");

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  delete gManifestObject.experiments[0].disabled;
  yield experiments.updateManifest();

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment 1 should still be disabled.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});






add_task(function* test_freezePendingExperiment() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].frozen = true;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should have no entries yet.");

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  delete gManifestObject.experiments[0].frozen;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active now.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_freezeActiveExperiment() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");
  Assert.equal(list[0].name, EXPERIMENT1_NAME, "Experiments name should match.");

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].frozen = true;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should still be active.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_removeActiveExperiment() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate  = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate    = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);
  let startDate2 = futureDate(baseDate, 20000 * MS_IN_ONE_DAY);
  let endDate2   = futureDate(baseDate, 30000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
      {
        id:               EXPERIMENT2_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT2_XPI_SHA1,
        startTime:        dateToSeconds(startDate2),
        endTime:          dateToSeconds(endDate2),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");
  Assert.equal(list[0].name, EXPERIMENT1_NAME, "Experiments name should match.");

  

  now = futureDate(now, 1 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gManifestObject.experiments[0].frozen = true;
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should still be active.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});



add_task(function* test_invalidUrl() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate   = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME + ".invalid",
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        0,
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  gTimerScheduleOffset = null;

  yield experiments.updateManifest();
  Assert.equal(observerFireCount, expectedObserverFireCount,
               "Experiments observer should not have been called.");
  Assert.equal(gTimerScheduleOffset, null, "No new timer should have been scheduled.");

  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});




add_task(function* test_unexpectedUninstall() {
  const OBSERVER_TOPIC = "experiments-changed";
  let observerFireCount = 0;
  let expectedObserverFireCount = 0;
  let observer = () => ++observerFireCount;
  Services.obs.addObserver(observer, OBSERVER_TOPIC, false);

  

  let baseDate   = new Date(2014, 5, 1, 12);
  let startDate  = futureDate(baseDate,   100 * MS_IN_ONE_DAY);
  let endDate    = futureDate(baseDate, 10000 * MS_IN_ONE_DAY);

  

  gManifestObject = {
    "version": 1,
    experiments: [
      {
        id:               EXPERIMENT1_ID,
        xpiURL:           gDataRoot + EXPERIMENT1_XPI_NAME,
        xpiHash:          EXPERIMENT1_XPI_SHA1,
        startTime:        dateToSeconds(startDate),
        endTime:          dateToSeconds(endDate),
        maxActiveSeconds: 10 * SEC_IN_ONE_DAY,
        appName:          ["XPCShell"],
        channel:          ["nightly"],
      },
    ],
  };

  let experiments = new Experiments.Experiments(gPolicy);

  

  let now = baseDate;
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, 0,
               "Experiments observer should not have been called yet.");
  let list = yield experiments.getExperiments();
  Assert.equal(list.length, 0, "Experiment list should be empty.");

  

  now = futureDate(startDate, 10 * MS_IN_ONE_DAY);
  defineNow(gPolicy, now);
  yield experiments.updateManifest();
  Assert.equal(observerFireCount, ++expectedObserverFireCount,
               "Experiments observer should have been called.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, true, "Experiment 1 should be active.");

  
  

  let success = yield uninstallAddon(EXPERIMENT1_ID);
  Assert.ok(success, "Addon should have been uninstalled.");

  list = yield experiments.getExperiments();
  Assert.equal(list.length, 1, "Experiment list should have 1 entry now.");
  Assert.equal(list[0].id, EXPERIMENT1_ID, "Experiment 1 should be the sole entry.");
  Assert.equal(list[0].active, false, "Experiment 1 should not be active anymore.");

  

  Services.obs.removeObserver(observer, OBSERVER_TOPIC);
  yield experiments.uninit();
  yield removeCacheFile();
});


add_task(function* shutdown() {
  yield gReporter._shutdown();
  yield removeCacheFile();
});
