









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js", this);
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/ClientID.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryPing.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);
Cu.import("resource://gre/modules/TelemetryFile.jsm", this);
Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/osfile.jsm", this);

const PING_FORMAT_VERSION = 4;
const PING_TYPE_MAIN = "main";
const PING_TYPE_SAVED_SESSION = "saved-session";

const REASON_ABORTED_SESSION = "aborted-session";
const REASON_SAVED_SESSION = "saved-session";
const REASON_SHUTDOWN = "shutdown";
const REASON_TEST_PING = "test-ping";
const REASON_DAILY = "daily";
const REASON_ENVIRONMENT_CHANGE = "environment-change";

const PLATFORM_VERSION = "1.9.2";
const APP_VERSION = "1";
const APP_ID = "xpcshell@tests.mozilla.org";
const APP_NAME = "XPCShell";

const IGNORE_HISTOGRAM = "test::ignore_me";
const IGNORE_HISTOGRAM_TO_CLONE = "MEMORY_HEAP_ALLOCATED";
const IGNORE_CLONED_HISTOGRAM = "test::ignore_me_also";
const ADDON_NAME = "Telemetry test addon";
const ADDON_HISTOGRAM = "addon-histogram";

const SHUTDOWN_TIME = 10000;
const FAILED_PROFILE_LOCK_ATTEMPTS = 2;


const PR_WRONLY = 0x2;
const PR_CREATE_FILE = 0x8;
const PR_TRUNCATE = 0x20;
const RW_OWNER = parseInt("0600", 8);

const NUMBER_OF_THREADS_TO_LAUNCH = 30;
let gNumberOfThreadsLaunched = 0;

const MS_IN_ONE_HOUR  = 60 * 60 * 1000;
const MS_IN_ONE_DAY   = 24 * MS_IN_ONE_HOUR;

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_FHR_SERVICE_ENABLED = "datareporting.healthreport.service.enabled";

const DATAREPORTING_DIR = "datareporting";
const ABORTED_PING_FILE_NAME = "aborted-session-ping";
const ABORTED_SESSION_UPDATE_INTERVAL_MS = 5 * 60 * 1000;

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);

XPCOMUtils.defineLazyGetter(this, "DATAREPORTING_PATH", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIR);
});

let gHttpServer = new HttpServer();
let gServerStarted = false;
let gRequestIterator = null;
let gClientID = null;

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}

function truncateDateToDays(date) {
  return new Date(date.getFullYear(),
                  date.getMonth(),
                  date.getDate(),
                  0, 0, 0, 0);
}

function sendPing() {
  TelemetrySession.gatherStartup();
  if (gServerStarted) {
    TelemetryPing.setServer("http://localhost:" + gHttpServer.identity.primaryPort);
    return TelemetrySession.testPing();
  } else {
    TelemetryPing.setServer("http://doesnotexist");
    return TelemetrySession.testPing();
  }
}

function wrapWithExceptionHandler(f) {
  function wrapper(...args) {
    try {
      f(...args);
    } catch (ex if typeof(ex) == 'object') {
      dump("Caught exception: " + ex.message + "\n");
      dump(ex.stack);
      do_test_finished();
    }
  }
  return wrapper;
}

function fakeGenerateUUID(sessionFunc, subsessionFunc) {
  let session = Cu.import("resource://gre/modules/TelemetrySession.jsm");
  session.Policy.generateSessionUUID = sessionFunc;
  session.Policy.generateSubsessionUUID = subsessionFunc;
}

function fakeIdleNotification(topic) {
  let session = Cu.import("resource://gre/modules/TelemetrySession.jsm");
  return session.TelemetryScheduler.observe(null, topic, null);
}

function registerPingHandler(handler) {
  gHttpServer.registerPrefixHandler("/submit/telemetry/",
				   wrapWithExceptionHandler(handler));
}

function setupTestData() {
  Telemetry.newHistogram(IGNORE_HISTOGRAM, "never", Telemetry.HISTOGRAM_BOOLEAN);
  Telemetry.histogramFrom(IGNORE_CLONED_HISTOGRAM, IGNORE_HISTOGRAM_TO_CLONE);
  Services.startup.interrupted = true;
  Telemetry.registerAddonHistogram(ADDON_NAME, ADDON_HISTOGRAM,
                                   Telemetry.HISTOGRAM_LINEAR,
                                   1, 5, 6);
  let h1 = Telemetry.getAddonHistogram(ADDON_NAME, ADDON_HISTOGRAM);
  h1.add(1);
  let h2 = Telemetry.getHistogramById("TELEMETRY_TEST_COUNT");
  h2.add();

  let k1 = Telemetry.getKeyedHistogramById("TELEMETRY_TEST_KEYED_COUNT");
  k1.add("a");
  k1.add("a");
  k1.add("b");
}

function getSavedPingFile(basename) {
  let tmpDir = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let pingFile = tmpDir.clone();
  pingFile.append(basename);
  if (pingFile.exists()) {
    pingFile.remove(true);
  }
  do_register_cleanup(function () {
    try {
      pingFile.remove(true);
    } catch (e) {
    }
  });
  return pingFile;
}

function checkPingFormat(aPing, aType, aHasClientId, aHasEnvironment) {
  const MANDATORY_PING_FIELDS = [
    "type", "id", "creationDate", "version", "application", "payload"
  ];

  const APPLICATION_TEST_DATA = {
    buildId: "2007010101",
    name: APP_NAME,
    version: APP_VERSION,
    vendor: "Mozilla",
    platformVersion: PLATFORM_VERSION,
    xpcomAbi: "noarch-spidermonkey",
  };

  
  for (let f of MANDATORY_PING_FIELDS) {
    Assert.ok(f in aPing, f + "must be available.");
  }

  Assert.equal(aPing.type, aType, "The ping must have the correct type.");
  Assert.equal(aPing.version, PING_FORMAT_VERSION, "The ping must have the correct version.");

  
  for (let f in APPLICATION_TEST_DATA) {
    Assert.equal(aPing.application[f], APPLICATION_TEST_DATA[f],
                 f + " must have the correct value.");
  }

  
  
  Assert.ok("architecture" in aPing.application,
            "The application section must have an architecture field.");
  Assert.ok("channel" in aPing.application,
            "The application section must have a channel field.");

  
  Assert.equal("clientId" in aPing, aHasClientId);
  Assert.equal("environment" in aPing, aHasEnvironment);
}

function checkPayloadInfo(data) {
  const ALLOWED_REASONS = [
    "environment-change", "shutdown", "daily", "saved-session", "test-ping"
  ];
  const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
  let numberCheck = arg => { return (typeof arg == "number"); };
  let positiveNumberCheck = arg => { return numberCheck(arg) && (arg >= 0); };
  let stringCheck = arg => { return (typeof arg == "string") && (arg != ""); };
  let isoDateCheck = arg => { return stringCheck(arg) && !Number.isNaN(Date.parse(arg)); }
  let revisionCheck = arg => {
    return (Services.appinfo.isOfficial) ? stringCheck(arg) : (typeof arg == "string");
  };
  let uuidCheck = arg => uuidRegex.test(arg);

  const EXPECTED_INFO_FIELDS_TYPES = {
    reason: stringCheck,
    revision: revisionCheck,
    timezoneOffset: numberCheck,
    sessionId: uuidCheck,
    subsessionId: uuidCheck,
    
    previousSubsessionId: (arg) => { return (arg) ? uuidCheck(arg) : true; },
    subsessionCounter: positiveNumberCheck,
    profileSubsessionCounter: positiveNumberCheck,
    sessionStartDate: isoDateCheck,
    subsessionStartDate: isoDateCheck,
    subsessionLength: positiveNumberCheck,
  };

  for (let f in EXPECTED_INFO_FIELDS_TYPES) {
    Assert.ok(f in data, f + " must be available.");

    let checkFunc = EXPECTED_INFO_FIELDS_TYPES[f];
    Assert.ok(checkFunc(data[f]),
              f + " must have the correct type and valid data " + data[f]);
  }

  
  if (data.previousBuildId) {
    Assert.ok(stringCheck(data.previousBuildId));
  }

  Assert.ok(ALLOWED_REASONS.find(r => r == data.reason),
            "Payload must contain an allowed reason.");

  Assert.ok(Date.parse(data.subsessionStartDate) >= Date.parse(data.sessionStartDate));
  Assert.ok(data.profileSubsessionCounter >= data.subsessionCounter);
  Assert.ok(data.timezoneOffset >= -12*60, "The timezone must be in a valid range.");
  Assert.ok(data.timezoneOffset <= 12*60, "The timezone must be in a valid range.");
}

function checkPayload(payload, reason, successfulPings) {
  Assert.ok("info" in payload, "Payload must contain an info section.");
  checkPayloadInfo(payload.info);

  Assert.ok(payload.simpleMeasurements.totalTime >= 0);
  Assert.ok(payload.simpleMeasurements.uptime >= 0);
  Assert.equal(payload.simpleMeasurements.startupInterrupted, 1);
  Assert.equal(payload.simpleMeasurements.shutdownDuration, SHUTDOWN_TIME);
  Assert.equal(payload.simpleMeasurements.savedPings, 1);
  Assert.ok("maximalNumberOfConcurrentThreads" in payload.simpleMeasurements);
  Assert.ok(payload.simpleMeasurements.maximalNumberOfConcurrentThreads >= gNumberOfThreadsLaunched);

  let activeTicks = payload.simpleMeasurements.activeTicks;
  Assert.ok(activeTicks >= 0);

  Assert.equal(payload.simpleMeasurements.failedProfileLockCount,
              FAILED_PROFILE_LOCK_ATTEMPTS);
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let failedProfileLocksFile = profileDirectory.clone();
  failedProfileLocksFile.append("Telemetry.FailedProfileLocks.txt");
  Assert.ok(!failedProfileLocksFile.exists());


  let isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
  if (isWindows) {
    Assert.ok(payload.simpleMeasurements.startupSessionRestoreReadBytes > 0);
    Assert.ok(payload.simpleMeasurements.startupSessionRestoreWriteBytes > 0);
  }

  const TELEMETRY_PING = "TELEMETRY_PING";
  const TELEMETRY_SUCCESS = "TELEMETRY_SUCCESS";
  const TELEMETRY_TEST_FLAG = "TELEMETRY_TEST_FLAG";
  const TELEMETRY_TEST_COUNT = "TELEMETRY_TEST_COUNT";
  const TELEMETRY_TEST_KEYED_FLAG = "TELEMETRY_TEST_KEYED_FLAG";
  const TELEMETRY_TEST_KEYED_COUNT = "TELEMETRY_TEST_KEYED_COUNT";
  const READ_SAVED_PING_SUCCESS = "READ_SAVED_PING_SUCCESS";

  Assert.ok(TELEMETRY_PING in payload.histograms);
  Assert.ok(READ_SAVED_PING_SUCCESS in payload.histograms);
  Assert.ok(TELEMETRY_TEST_FLAG in payload.histograms);
  Assert.ok(TELEMETRY_TEST_COUNT in payload.histograms);

  let rh = Telemetry.registeredHistograms(Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN, []);
  for (let name of rh) {
    if (/SQLITE/.test(name) && name in payload.histograms) {
      let histogramName = ("STARTUP_" + name);
      Assert.ok(histogramName in payload.histograms, histogramName + " must be available.");
    }
  }
  Assert.ok(!(IGNORE_HISTOGRAM in payload.histograms));
  Assert.ok(!(IGNORE_CLONED_HISTOGRAM in payload.histograms));

  
  const expected_flag = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 3,
    values: {0:1, 1:0},
    sum: 0,
    sum_squares_lo: 0,
    sum_squares_hi: 0
  };
  let flag = payload.histograms[TELEMETRY_TEST_FLAG];
  Assert.equal(uneval(flag), uneval(expected_flag));

  
  const expected_count = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 4,
    values: {0:1, 1:0},
    sum: 1,
    sum_squares_lo: 1,
    sum_squares_hi: 0,
  };
  let count = payload.histograms[TELEMETRY_TEST_COUNT];
  Assert.equal(uneval(count), uneval(expected_count));

  
  const expected_tc = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 2,
    values: {0:2, 1:successfulPings, 2:0},
    sum: successfulPings,
    sum_squares_lo: successfulPings,
    sum_squares_hi: 0
  };
  let tc = payload.histograms[TELEMETRY_SUCCESS];
  Assert.equal(uneval(tc), uneval(expected_tc));

  let h = payload.histograms[READ_SAVED_PING_SUCCESS];
  Assert.equal(h.values[0], 1);

  
  
  
  
  
  
  
  
  

  Assert.ok('MEMORY_JS_GC_HEAP' in payload.histograms); 
  Assert.ok('MEMORY_JS_COMPARTMENTS_SYSTEM' in payload.histograms); 

  
  Assert.ok("addonHistograms" in payload);
  Assert.ok(ADDON_NAME in payload.addonHistograms);
  Assert.ok(ADDON_HISTOGRAM in payload.addonHistograms[ADDON_NAME]);

  Assert.ok(("mainThread" in payload.slowSQL) &&
                ("otherThreads" in payload.slowSQL));

  

  Assert.ok("keyedHistograms" in payload);
  let keyedHistograms = payload.keyedHistograms;
  Assert.ok(TELEMETRY_TEST_KEYED_FLAG in keyedHistograms);
  Assert.ok(TELEMETRY_TEST_KEYED_COUNT in keyedHistograms);

  Assert.deepEqual({}, keyedHistograms[TELEMETRY_TEST_KEYED_FLAG]);

  const expected_keyed_count = {
    "a": {
      range: [1, 2],
      bucket_count: 3,
      histogram_type: 4,
      values: {0:2, 1:0},
      sum: 2,
      sum_squares_lo: 2,
      sum_squares_hi: 0,
    },
    "b": {
      range: [1, 2],
      bucket_count: 3,
      histogram_type: 4,
      values: {0:1, 1:0},
      sum: 1,
      sum_squares_lo: 1,
      sum_squares_hi: 0,
    },
  };
  Assert.deepEqual(expected_keyed_count, keyedHistograms[TELEMETRY_TEST_KEYED_COUNT]);
}

function writeStringToFile(file, contents) {
  let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                .createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
	       RW_OWNER, ostream.DEFER_OPEN);
  ostream.write(contents, contents.length);
  ostream.QueryInterface(Ci.nsISafeOutputStream).finish();
  ostream.close();
}

function write_fake_shutdown_file() {
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let file = profileDirectory.clone();
  file.append("Telemetry.ShutdownTime.txt");
  let contents = "" + SHUTDOWN_TIME;
  writeStringToFile(file, contents);
}

function write_fake_failedprofilelocks_file() {
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let file = profileDirectory.clone();
  file.append("Telemetry.FailedProfileLocks.txt");
  let contents = "" + FAILED_PROFILE_LOCK_ATTEMPTS;
  writeStringToFile(file, contents);
}

function run_test() {
  do_test_pending();

  
  do_get_profile();
  loadAddonManager(APP_ID, APP_NAME, APP_VERSION, PLATFORM_VERSION);

  Services.prefs.setBoolPref(PREF_ENABLED, true);
  Services.prefs.setBoolPref(PREF_FHR_UPLOAD_ENABLED, true);

  
  write_fake_failedprofilelocks_file();

  
  write_fake_shutdown_file();

  let currentMaxNumberOfThreads = Telemetry.maximalNumberOfConcurrentThreads;
  do_check_true(currentMaxNumberOfThreads > 0);

  
  let threads = [];
  try {
    for (let i = 0; i < currentMaxNumberOfThreads + 10; ++i) {
      threads.push(Services.tm.newThread(0));
    }
  } catch (ex) {
    
  }
  gNumberOfThreadsLaunched = threads.length;

  do_check_true(Telemetry.maximalNumberOfConcurrentThreads >= gNumberOfThreadsLaunched);

  do_register_cleanup(function() {
    threads.forEach(function(thread) {
      thread.shutdown();
    });
  });

  Telemetry.asyncFetchTelemetryData(wrapWithExceptionHandler(run_next_test));
}

add_task(function* asyncSetup() {
  yield TelemetrySession.setup();
  yield TelemetryPing.setup();
  
  gClientID = yield ClientID.getClientID();
});


add_task(function* test_expiredHistogram() {
  let histogram_id = "FOOBAR";
  let dummy = Telemetry.newHistogram(histogram_id, "30", Telemetry.HISTOGRAM_EXPONENTIAL, 1, 2, 3);

  dummy.add(1);

  do_check_eq(TelemetrySession.getPayload()["histograms"][histogram_id], undefined);
  do_check_eq(TelemetrySession.getPayload()["histograms"]["TELEMETRY_TEST_EXPIRED"], undefined);
});


add_task(function* test_runInvalidJSON() {
  let pingFile = getSavedPingFile("invalid-histograms.dat");

  writeStringToFile(pingFile, "this.is.invalid.JSON");
  do_check_true(pingFile.exists());

  yield TelemetryFile.testLoadHistograms(pingFile);
  do_check_false(pingFile.exists());
});



add_task(function* test_noServerPing() {
  yield sendPing();
  
  
  yield sendPing();
});


add_task(function* test_simplePing() {
  gHttpServer.start(-1);
  gServerStarted = true;
  gRequestIterator = Iterator(new Request());
  Preferences.set(PREF_SERVER, "http://localhost:" + gHttpServer.identity.primaryPort);

  let now = new Date(2020, 1, 1, 12, 0, 0);
  let expectedDate = new Date(2020, 1, 1, 0, 0, 0);
  fakeNow(now);

  const expectedSessionUUID = "bd314d15-95bf-4356-b682-b6c4a8942202";
  const expectedSubsessionUUID = "3e2e5f6c-74ba-4e4d-a93f-a48af238a8c7";
  fakeGenerateUUID(() => expectedSessionUUID, () => expectedSubsessionUUID);
  yield TelemetrySession.reset();

  
  
  const SESSION_DURATION_IN_MINUTES = 15;
  fakeNow(new Date(2020, 1, 1, 12, SESSION_DURATION_IN_MINUTES, 0));

  yield sendPing();
  let request = yield gRequestIterator.next();
  let ping = decodeRequestPayload(request);

  checkPingFormat(ping, PING_TYPE_MAIN, true, true);

  
  let payload = ping.payload;
  Assert.equal(payload.info.sessionId, expectedSessionUUID);
  Assert.equal(payload.info.subsessionId, expectedSubsessionUUID);
  let sessionStartDate = new Date(payload.info.sessionStartDate);
  Assert.equal(sessionStartDate.toISOString(), expectedDate.toISOString());
  let subsessionStartDate = new Date(payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());
  Assert.equal(payload.info.subsessionLength, SESSION_DURATION_IN_MINUTES * 60);

  
  fakeGenerateUUID(generateUUID, generateUUID);
});




add_task(function* test_saveLoadPing() {
  let histogramsFile = getSavedPingFile("saved-histograms.dat");

  setupTestData();
  yield TelemetrySession.testSaveHistograms(histogramsFile);
  yield TelemetryFile.testLoadHistograms(histogramsFile);
  yield sendPing();

  
  let request1 = yield gRequestIterator.next();
  let request2 = yield gRequestIterator.next();

  Assert.equal(request1.getHeader("content-type"), "application/json; charset=UTF-8",
               "The request must have the correct content-type.");
  Assert.equal(request2.getHeader("content-type"), "application/json; charset=UTF-8",
               "The request must have the correct content-type.");

  
  let ping1 = decodeRequestPayload(request1);
  let ping2 = decodeRequestPayload(request2);

  
  
  let requestTypeComponent = request1.path.split("/")[4];
  if (requestTypeComponent === PING_TYPE_MAIN) {
    checkPingFormat(ping1, PING_TYPE_MAIN, true, true);
    checkPayload(ping1.payload, REASON_TEST_PING, 1);
    checkPingFormat(ping2, PING_TYPE_SAVED_SESSION, true, true);
    checkPayload(ping2.payload, REASON_SAVED_SESSION, 1);
  } else {
    checkPingFormat(ping1, PING_TYPE_SAVED_SESSION, true, true);
    checkPayload(ping1.payload, REASON_SAVED_SESSION, 1);
    checkPingFormat(ping2, PING_TYPE_MAIN, true, true);
    checkPayload(ping2.payload, REASON_TEST_PING, 1);
  }
});

add_task(function* test_checkSubsessionHistograms() {
  if (gIsAndroid) {
    
    return;
  }

  let now = new Date(2020, 1, 1, 12, 0, 0);
  let expectedDate = new Date(2020, 1, 1, 0, 0, 0);
  fakeNow(now);
  yield TelemetrySession.setup();

  const COUNT_ID = "TELEMETRY_TEST_COUNT";
  const KEYED_ID = "TELEMETRY_TEST_KEYED_COUNT";
  const count = Telemetry.getHistogramById(COUNT_ID);
  const keyed = Telemetry.getKeyedHistogramById(KEYED_ID);
  const registeredIds =
    new Set(Telemetry.registeredHistograms(Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN, []));

  const stableHistograms = new Set([
    "TELEMETRY_TEST_FLAG",
    "TELEMETRY_TEST_COUNT",
    "TELEMETRY_TEST_RELEASE_OPTOUT",
    "TELEMETRY_TEST_RELEASE_OPTIN",
    "STARTUP_CRASH_DETECTED",
  ]);

  const stableKeyedHistograms = new Set([
    "TELEMETRY_TEST_KEYED_FLAG",
    "TELEMETRY_TEST_KEYED_COUNT",
    "TELEMETRY_TEST_KEYED_RELEASE_OPTIN",
    "TELEMETRY_TEST_KEYED_RELEASE_OPTOUT",
  ]);

  
  
  
  
  
  checkHistograms = (classic, subsession) => {
    for (let id of Object.keys(classic)) {
      if (!registeredIds.has(id)) {
        continue;
      }

      Assert.ok(id in subsession);
      if (stableHistograms.has(id)) {
        Assert.deepEqual(classic[id],
                         subsession[id]);
      } else {
        Assert.equal(classic[id].histogram_type,
                     subsession[id].histogram_type);
      }
    }
  };

  
  checkKeyedHistograms = (classic, subsession) => {
    for (let id of Object.keys(classic)) {
      if (!registeredIds.has(id)) {
        continue;
      }

      Assert.ok(id in subsession);
      if (stableKeyedHistograms.has(id)) {
        Assert.deepEqual(classic[id],
                         subsession[id]);
      }
    }
  };

  
  
  count.clear();
  keyed.clear();
  let classic = TelemetrySession.getPayload();
  let subsession = TelemetrySession.getPayload("environment-change");

  Assert.equal(classic.info.reason, "gather-payload");
  Assert.equal(subsession.info.reason, "environment-change");
  Assert.ok(!(COUNT_ID in classic.histograms));
  Assert.ok(!(COUNT_ID in subsession.histograms));
  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.deepEqual(classic.keyedHistograms[KEYED_ID], {});
  Assert.deepEqual(subsession.keyedHistograms[KEYED_ID], {});

  checkHistograms(classic.histograms, subsession.histograms);
  checkKeyedHistograms(classic.keyedHistograms, subsession.keyedHistograms);

  
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");

  Assert.ok(COUNT_ID in classic.histograms);
  Assert.ok(COUNT_ID in subsession.histograms);
  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.equal(classic.histograms[COUNT_ID].sum, 1);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["b"].sum, 1);

  checkHistograms(classic.histograms, subsession.histograms);
  checkKeyedHistograms(classic.keyedHistograms, subsession.keyedHistograms);

  
  count.clear();
  keyed.clear();
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");

  Assert.ok(!(COUNT_ID in classic.histograms));
  Assert.ok(!(COUNT_ID in subsession.histograms));
  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.deepEqual(classic.keyedHistograms[KEYED_ID], {});

  checkHistograms(classic.histograms, subsession.histograms);
  checkKeyedHistograms(classic.keyedHistograms, subsession.keyedHistograms);

  
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");

  Assert.ok(COUNT_ID in classic.histograms);
  Assert.ok(COUNT_ID in subsession.histograms);
  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.equal(classic.histograms[COUNT_ID].sum, 1);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["b"].sum, 1);

  checkHistograms(classic.histograms, subsession.histograms);
  checkKeyedHistograms(classic.keyedHistograms, subsession.keyedHistograms);

  
  
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change", true);

  let subsessionStartDate = new Date(classic.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());
  subsessionStartDate = new Date(subsession.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());
  checkHistograms(classic.histograms, subsession.histograms);
  checkKeyedHistograms(classic.keyedHistograms, subsession.keyedHistograms);

  
  
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");

  Assert.ok(COUNT_ID in classic.histograms);
  Assert.ok(COUNT_ID in subsession.histograms);
  Assert.equal(classic.histograms[COUNT_ID].sum, 1);
  Assert.equal(subsession.histograms[COUNT_ID].sum, 0);

  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["b"].sum, 1);
  Assert.deepEqual(subsession.keyedHistograms[KEYED_ID], {});

  
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");

  Assert.ok(COUNT_ID in classic.histograms);
  Assert.ok(COUNT_ID in subsession.histograms);
  Assert.equal(classic.histograms[COUNT_ID].sum, 2);
  Assert.equal(subsession.histograms[COUNT_ID].sum, 1);

  Assert.ok(KEYED_ID in classic.keyedHistograms);
  Assert.ok(KEYED_ID in subsession.keyedHistograms);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["a"].sum, 2);
  Assert.equal(classic.keyedHistograms[KEYED_ID]["b"].sum, 2);
  Assert.equal(subsession.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(subsession.keyedHistograms[KEYED_ID]["b"].sum, 1);
});

add_task(function* test_checkSubsessionData() {
  if (gIsAndroid) {
    
    return;
  }

  
  let sessionRecorder = TelemetryPing.getSessionRecorder();
  let activeTicksAtSubsessionStart = sessionRecorder.activeTicks;
  let expectedActiveTicks = activeTicksAtSubsessionStart;

  incrementActiveTicks = () => {
    sessionRecorder.incrementActiveTicks();
    ++expectedActiveTicks;
  }

  yield TelemetrySession.reset();

  
  incrementActiveTicks();
  let classic = TelemetrySession.getPayload();
  let subsession = TelemetrySession.getPayload("environment-change");
  Assert.equal(classic.simpleMeasurements.activeTicks, expectedActiveTicks,
               "Classic pings must count active ticks since the beginning of the session.");
  Assert.equal(subsession.simpleMeasurements.activeTicks, expectedActiveTicks,
               "Subsessions must count active ticks as classic pings on the first subsession.");

  
  incrementActiveTicks();
  activeTicksAtSubsessionStart = sessionRecorder.activeTicks;
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change", true);
  Assert.equal(classic.simpleMeasurements.activeTicks, expectedActiveTicks,
               "Classic pings must count active ticks since the beginning of the session.");
  Assert.equal(subsession.simpleMeasurements.activeTicks, expectedActiveTicks,
               "Pings must not loose the tick count when starting a new subsession.");

  
  incrementActiveTicks();
  classic = TelemetrySession.getPayload();
  subsession = TelemetrySession.getPayload("environment-change");
  Assert.equal(classic.simpleMeasurements.activeTicks, expectedActiveTicks,
               "Classic pings must count active ticks since the beginning of the session.");
  Assert.equal(subsession.simpleMeasurements.activeTicks,
               expectedActiveTicks - activeTicksAtSubsessionStart,
               "Subsessions must count active ticks since the last new subsession.");
});

add_task(function* test_dailyCollection() {
  if (gIsAndroid) {
    
    return;
  }

  let now = new Date(2030, 1, 1, 12, 0, 0);
  let nowDay = new Date(2030, 1, 1, 0, 0, 0);
  let schedulerTickCallback = null;

  gRequestIterator = Iterator(new Request());

  fakeNow(now);

  
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});

  
  yield TelemetrySession.setup();
  TelemetryPing.setServer("http://localhost:" + gHttpServer.identity.primaryPort);

  
  const COUNT_ID = "TELEMETRY_TEST_COUNT";
  const KEYED_ID = "TELEMETRY_TEST_KEYED_COUNT";
  const count = Telemetry.getHistogramById(COUNT_ID);
  const keyed = Telemetry.getKeyedHistogramById(KEYED_ID);

  count.clear();
  keyed.clear();
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);
  keyed.add("b", 1);

  
  let expectedDate = nowDay;
  now = futureDate(nowDay, MS_IN_ONE_DAY);
  fakeNow(now);

  Assert.ok(!!schedulerTickCallback);
  
  yield schedulerTickCallback();

  
  let request = yield gRequestIterator.next();
  Assert.ok(!!request);
  let ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);
  let subsessionStartDate = new Date(ping.payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());

  Assert.equal(ping.payload.histograms[COUNT_ID].sum, 1);
  Assert.equal(ping.payload.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(ping.payload.keyedHistograms[KEYED_ID]["b"].sum, 2);

  
  expectedDate = futureDate(expectedDate, MS_IN_ONE_DAY);
  now = futureDate(now, MS_IN_ONE_DAY);
  fakeNow(now);

  
  yield schedulerTickCallback();

  request = yield gRequestIterator.next();
  Assert.ok(!!request);
  ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);
  subsessionStartDate = new Date(ping.payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());

  Assert.equal(ping.payload.histograms[COUNT_ID].sum, 0);
  Assert.deepEqual(ping.payload.keyedHistograms[KEYED_ID], {});

  
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);

  
  expectedDate = futureDate(expectedDate, MS_IN_ONE_DAY);
  now = futureDate(now, MS_IN_ONE_DAY);
  fakeNow(now);

  yield schedulerTickCallback();
  request = yield gRequestIterator.next();
  Assert.ok(!!request);
  ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);
  subsessionStartDate = new Date(ping.payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), expectedDate.toISOString());

  Assert.equal(ping.payload.histograms[COUNT_ID].sum, 1);
  Assert.equal(ping.payload.keyedHistograms[KEYED_ID]["a"].sum, 1);
  Assert.equal(ping.payload.keyedHistograms[KEYED_ID]["b"].sum, 1);

  
  yield TelemetrySession.shutdown();
});

add_task(function* test_dailyDuplication() {
  if (gIsAndroid) {
    
    return;
  }

  gRequestIterator = Iterator(new Request());

  let schedulerTickCallback = null;
  let now = new Date(2030, 1, 1, 0, 0, 0);
  fakeNow(now);
  
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.setup();

  
  
  
  let firstDailyDue = new Date(2030, 1, 2, 0, 0, 0);
  fakeNow(firstDailyDue);

  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();

  
  let request = yield gRequestIterator.next();
  Assert.ok(!!request);
  let ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);

  
  registerPingHandler((req, res) => {
    Assert.ok(false, "No more daily pings should be sent/received in this test.");
  });

  
  let secondDailyDue = new Date(firstDailyDue);
  secondDailyDue.setHours(0);
  secondDailyDue.setMinutes(15);
  fakeNow(secondDailyDue);

  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();

  
  yield TelemetrySession.shutdown();
});

add_task(function* test_dailyOverdue() {
  if (gIsAndroid) {
    
    return;
  }

  let schedulerTickCallback = null;
  let now = new Date(2030, 1, 1, 11, 0, 0);
  fakeNow(now);
  
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.setup();

  
  now.setHours(now.getHours() + 1);
  fakeNow(now);

  
  registerPingHandler((req, res) => {
    Assert.ok(false, "No daily ping should be received if not overdue!.");
  });

  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();

  
  gRequestIterator = Iterator(new Request());

  
  
  let dailyOverdue = new Date(2030, 1, 2, 13, 00, 0);
  fakeNow(dailyOverdue);

  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();

  
  let request = yield gRequestIterator.next();
  Assert.ok(!!request);
  let ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);

  
  yield TelemetrySession.shutdown();
});

add_task(function* test_environmentChange() {
  if (gIsAndroid) {
    
    return;
  }

  let now = new Date(2040, 1, 1, 12, 0, 0);
  let timerCallback = null;
  let timerDelay = null;

  gRequestIterator = Iterator(new Request());

  fakeNow(now);

  const PREF_TEST = "toolkit.telemetry.test.pref1";
  Preferences.reset(PREF_TEST);
  let prefsToWatch = {};
  prefsToWatch[PREF_TEST] = TelemetryEnvironment.RECORD_PREF_VALUE;

  
  yield TelemetrySession.setup();
  TelemetryPing.setServer("http://localhost:" + gHttpServer.identity.primaryPort);
  TelemetryEnvironment._watchPreferences(prefsToWatch);

  
  const COUNT_ID = "TELEMETRY_TEST_COUNT";
  const KEYED_ID = "TELEMETRY_TEST_KEYED_COUNT";
  const count = Telemetry.getHistogramById(COUNT_ID);
  const keyed = Telemetry.getKeyedHistogramById(KEYED_ID);

  count.clear();
  keyed.clear();
  count.add(1);
  keyed.add("a", 1);
  keyed.add("b", 1);

  
  let startDay = truncateDateToDays(now);
  now = futureDate(now, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(now);

  Preferences.set(PREF_TEST, 1);
  let request = yield gRequestIterator.next();
  Assert.ok(!!request);
  let ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.environment.settings.userPrefs[PREF_TEST], undefined);
  Assert.equal(ping.payload.info.reason, REASON_ENVIRONMENT_CHANGE);
  let subsessionStartDate = new Date(ping.payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), startDay.toISOString());

  Assert.equal(ping.payload.histograms[COUNT_ID].sum, 1);
  Assert.equal(ping.payload.keyedHistograms[KEYED_ID]["a"].sum, 1);

  
  startDay = truncateDateToDays(now);
  now = futureDate(now, 10 * MILLISECONDS_PER_MINUTE);
  fakeNow(now);

  Preferences.set(PREF_TEST, 2);
  request = yield gRequestIterator.next();
  Assert.ok(!!request);
  ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.environment.settings.userPrefs[PREF_TEST], 1);
  Assert.equal(ping.payload.info.reason, REASON_ENVIRONMENT_CHANGE);
  subsessionStartDate = new Date(ping.payload.info.subsessionStartDate);
  Assert.equal(subsessionStartDate.toISOString(), startDay.toISOString());

  Assert.equal(ping.payload.histograms[COUNT_ID].sum, 0);
  Assert.deepEqual(ping.payload.keyedHistograms[KEYED_ID], {});
});


add_task(function* test_runOldPingFile() {
  let histogramsFile = getSavedPingFile("old-histograms.dat");

  yield TelemetrySession.testSaveHistograms(histogramsFile);
  do_check_true(histogramsFile.exists());
  let mtime = histogramsFile.lastModifiedTime;
  histogramsFile.lastModifiedTime = mtime - (14 * 24 * 60 * 60 * 1000 + 60000); 

  yield TelemetryFile.testLoadHistograms(histogramsFile);
  do_check_false(histogramsFile.exists());
});

add_task(function* test_savedPingsOnShutdown() {
  
  
  const expectedPings = (gIsAndroid) ? 1 : 2;
  
  
  const dir = TelemetryFile.pingDirectoryPath;
  yield OS.File.removeDir(dir, {ignoreAbsent: true});
  yield OS.File.makeDir(dir);
  yield TelemetrySession.shutdown();

  yield TelemetryFile.loadSavedPings();
  Assert.equal(TelemetryFile.pingsLoaded, expectedPings);

  let pingsIterator = TelemetryFile.popPendingPings();
  for (let ping of pingsIterator) {
    Assert.ok("type" in ping);

    let expectedReason =
      (ping.type == PING_TYPE_SAVED_SESSION) ? REASON_SAVED_SESSION : REASON_SHUTDOWN;

    checkPingFormat(ping, ping.type, true, true);
    Assert.equal(ping.payload.info.reason, expectedReason);
    Assert.equal(ping.clientId, gClientID);
  }
});

add_task(function* test_savedSessionData() {
  
  
  yield OS.File.makeDir(DATAREPORTING_PATH);

  
  const dataFilePath = OS.Path.join(DATAREPORTING_PATH, "session-state.json");
  const sessionState = {
    previousSubsessionId: null,
    profileSubsessionCounter: 3785,
  };
  yield CommonUtils.writeJSON(sessionState, dataFilePath);

  const PREF_TEST = "toolkit.telemetry.test.pref1";
  Preferences.reset(PREF_TEST);
  let prefsToWatch = {};
  prefsToWatch[PREF_TEST] = TelemetryEnvironment.RECORD_PREF_VALUE;

  
  
  const expectedSubsessions = sessionState.profileSubsessionCounter + 2;
  const expectedUUID = "009fd1ad-b85e-4817-b3e5-000000003785";
  fakeGenerateUUID(generateUUID, () => expectedUUID);

  if (gIsAndroid) {
    
    return;
  }

  
  yield TelemetrySession.reset();
  

  
  fakeNow(new Date(2050, 1, 1, 12, 0, 0));
  TelemetryEnvironment._watchPreferences(prefsToWatch);
  let changePromise = new Promise(resolve =>
    TelemetryEnvironment.registerChangeListener("test_fake_change", resolve));
  Preferences.set(PREF_TEST, 1);
  yield changePromise;
  TelemetryEnvironment.unregisterChangeListener("test_fake_change");

  let payload = TelemetrySession.getPayload();
  Assert.equal(payload.info.profileSubsessionCounter, expectedSubsessions);
  yield TelemetrySession.shutdown();

  
  let data = yield CommonUtils.readJSON(dataFilePath);
  Assert.equal(data.profileSubsessionCounter, expectedSubsessions);
  Assert.equal(data.previousSubsessionId, expectedUUID);
});

add_task(function* test_invalidSessionData() {
  
  
  yield OS.File.makeDir(DATAREPORTING_PATH);

  
  const dataFilePath = OS.Path.join(DATAREPORTING_PATH, "session-state.json");
  const sessionState = {
    profileSubsessionCounter: "not-a-number?",
    someOtherField: 12,
  };
  yield CommonUtils.writeJSON(sessionState, dataFilePath);

  
  const expectedSubsessions = 1;
  const expectedUUID = "009fd1ad-b85e-4817-b3e5-000000003785";
  fakeGenerateUUID(() => expectedUUID, () => expectedUUID);
  
  yield TelemetrySession.reset();
  let payload = TelemetrySession.getPayload();
  Assert.equal(payload.info.profileSubsessionCounter, expectedSubsessions);
  yield TelemetrySession.shutdown();

  
  let data = yield CommonUtils.readJSON(dataFilePath);
  Assert.equal(data.profileSubsessionCounter, expectedSubsessions);
  Assert.equal(data.previousSubsessionId, null);
});

add_task(function* test_abortedSession() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  const ABORTED_FILE = OS.Path.join(DATAREPORTING_PATH, ABORTED_PING_FILE_NAME);

  
  yield OS.File.removeDir(DATAREPORTING_PATH, { ignoreAbsent: true });

  let schedulerTickCallback = null;
  let now = new Date(2040, 1, 1, 0, 0, 0);
  fakeNow(now);
  
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();

  Assert.ok((yield OS.File.exists(DATAREPORTING_PATH)),
            "Telemetry must create the aborted session directory when starting.");

  
  now = futureDate(now, ABORTED_SESSION_UPDATE_INTERVAL_MS);
  fakeNow(now);
  
  Assert.ok(!!schedulerTickCallback);
  
  yield schedulerTickCallback();
  
  Assert.ok((yield OS.File.exists(ABORTED_FILE)),
            "There must be an aborted session ping.");

  
  
  let pingContent = yield OS.File.read(ABORTED_FILE, { encoding: "utf-8" });
  let abortedSessionPing = JSON.parse(pingContent);

  
  checkPingFormat(abortedSessionPing, PING_TYPE_MAIN, true, true);
  Assert.equal(abortedSessionPing.payload.info.reason, REASON_ABORTED_SESSION);

  
  now = futureDate(now, ABORTED_SESSION_UPDATE_INTERVAL_MS);
  fakeNow(now);
  yield schedulerTickCallback();

  pingContent = yield OS.File.read(ABORTED_FILE, { encoding: "utf-8" });
  let updatedAbortedSessionPing = JSON.parse(pingContent);
  checkPingFormat(updatedAbortedSessionPing, PING_TYPE_MAIN, true, true);
  Assert.equal(updatedAbortedSessionPing.payload.info.reason, REASON_ABORTED_SESSION);
  Assert.notEqual(abortedSessionPing.id, updatedAbortedSessionPing.id);
  Assert.notEqual(abortedSessionPing.creationDate, updatedAbortedSessionPing.creationDate);

  yield TelemetrySession.shutdown();
  Assert.ok(!(yield OS.File.exists(ABORTED_FILE)),
            "No aborted session ping must be available after a shutdown.");

  
  
  yield TelemetryFile.savePingToFile(abortedSessionPing, ABORTED_FILE, false);

  gRequestIterator = Iterator(new Request());
  yield TelemetrySession.reset();

  Assert.ok(!(yield OS.File.exists(ABORTED_FILE)),
            "The aborted session ping must be removed from the aborted session ping directory.");

  
  
  const PENDING_PING_FILE =
    OS.Path.join(TelemetryFile.pingDirectoryPath, abortedSessionPing.id);
  Assert.ok((yield OS.File.exists(PENDING_PING_FILE)),
            "The aborted session ping must exist in the saved pings directory.");

  
  
  const OVERDUE_PING_FILE_AGE = TelemetryFile.OVERDUE_PING_FILE_AGE + 60 * 1000;
  yield OS.File.setDates(PENDING_PING_FILE, null, Date.now() - OVERDUE_PING_FILE_AGE);
  yield TelemetryPing.reset();

  
  let request = yield gRequestIterator.next();
  let receivedPing = decodeRequestPayload(request);
  Assert.equal(receivedPing.payload.info.reason, REASON_ABORTED_SESSION);
  Assert.equal(receivedPing.id, abortedSessionPing.id);

  yield TelemetrySession.shutdown();
});

add_task(function* test_abortedDailyCoalescing() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  const ABORTED_FILE = OS.Path.join(DATAREPORTING_PATH, ABORTED_PING_FILE_NAME);

  
  yield OS.File.removeDir(DATAREPORTING_PATH, { ignoreAbsent: true });

  let schedulerTickCallback = null;
  gRequestIterator = Iterator(new Request());

  let nowDate = new Date(2009, 10, 18, 00, 00, 0);
  fakeNow(nowDate);

  
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();

  Assert.ok((yield OS.File.exists(DATAREPORTING_PATH)),
            "Telemetry must create the aborted session directory when starting.");

  
  
  let dailyDueDate = futureDate(nowDate, MS_IN_ONE_DAY);
  fakeNow(dailyDueDate);
  
  Assert.ok(!!schedulerTickCallback);
  
  yield schedulerTickCallback();

  
  let request = yield gRequestIterator.next();
  let dailyPing = decodeRequestPayload(request);
  Assert.equal(dailyPing.payload.info.reason, REASON_DAILY);

  
  Assert.ok((yield OS.File.exists(ABORTED_FILE)),
            "There must be an aborted session ping.");

  
  
  let pingContent = yield OS.File.read(ABORTED_FILE, { encoding: "utf-8" });
  let abortedSessionPing = JSON.parse(pingContent);
  Assert.equal(abortedSessionPing.payload.info.sessionId, dailyPing.payload.info.sessionId);
  Assert.equal(abortedSessionPing.payload.info.subsessionId, dailyPing.payload.info.subsessionId);

  yield TelemetrySession.shutdown();
});

add_task(function* test_schedulerComputerSleep() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  const ABORTED_FILE = OS.Path.join(DATAREPORTING_PATH, ABORTED_PING_FILE_NAME);

  gRequestIterator = Iterator(new Request());

  
  yield OS.File.removeDir(DATAREPORTING_PATH, { ignoreAbsent: true });

  
  let nowDate = new Date(2009, 10, 18, 0, 00, 0);
  fakeNow(nowDate);
  let schedulerTickCallback = null;
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();

  
  let future = futureDate(nowDate, MS_IN_ONE_DAY * 3);
  fakeNow(future);
  Assert.ok(!!schedulerTickCallback);
  
  yield schedulerTickCallback();

  let request = yield gRequestIterator.next();
  let dailyPing = decodeRequestPayload(request);
  Assert.equal(dailyPing.payload.info.reason, REASON_DAILY);

  Assert.ok((yield OS.File.exists(ABORTED_FILE)),
            "There must be an aborted session ping.");

  yield TelemetrySession.shutdown();
});

add_task(function* test_schedulerEnvironmentReschedules() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  
  const PREF_TEST = "toolkit.telemetry.test.pref1";
  Preferences.reset(PREF_TEST);
  let prefsToWatch = {};
  prefsToWatch[PREF_TEST] = TelemetryEnvironment.RECORD_PREF_VALUE;

  gRequestIterator = Iterator(new Request());

  
  let nowDate = new Date(2060, 10, 18, 0, 00, 0);
  fakeNow(nowDate);
  let schedulerTickCallback = null;
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();
  TelemetryEnvironment._watchPreferences(prefsToWatch);

  
  let future = futureDate(nowDate, MS_IN_ONE_DAY);
  fakeNow(future);

  
  Preferences.set(PREF_TEST, 1);

  
  yield gRequestIterator.next();

  
  registerPingHandler((req, res) => {
    Assert.ok(false, "No ping should be sent/received in this test.");
  });

  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();

  yield TelemetrySession.shutdown();
});

add_task(function* test_schedulerNothingDue() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  const ABORTED_FILE = OS.Path.join(DATAREPORTING_PATH, ABORTED_PING_FILE_NAME);

  
  yield OS.File.removeDir(DATAREPORTING_PATH, { ignoreAbsent: true });

  
  registerPingHandler((req, res) => {
    Assert.ok(false, "No ping should be sent/received in this test.");
  });

  
  
  let nowDate = new Date(2009, 10, 18, 11, 0, 0);
  fakeNow(nowDate);
  let schedulerTickCallback = null;
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();

  
  let nothingDueDate = futureDate(nowDate, ABORTED_SESSION_UPDATE_INTERVAL_MS / 2);
  fakeNow(nothingDueDate);
  Assert.ok(!!schedulerTickCallback);
  
  yield schedulerTickCallback();

  
  Assert.ok(!(yield OS.File.exists(ABORTED_FILE)));

  yield TelemetrySession.shutdown();
});

add_task(function* test_pingExtendedStats() {
  const EXTENDED_PAYLOAD_FIELDS = [
    "chromeHangs", "threadHangStats", "log", "slowSQL", "fileIOReports", "lateWrites",
    "addonHistograms", "addonDetails", "UIMeasurements",
  ];

  
  Telemetry.canRecordExtended = false;

  gRequestIterator = Iterator(new Request());
  yield TelemetrySession.reset();
  yield sendPing();

  let request = yield gRequestIterator.next();
  let ping = decodeRequestPayload(request);
  checkPingFormat(ping, PING_TYPE_MAIN, true, true);

  
  for (let f in EXTENDED_PAYLOAD_FIELDS) {
    Assert.ok(!(EXTENDED_PAYLOAD_FIELDS[f] in ping.payload),
              EXTENDED_PAYLOAD_FIELDS[f] + " must not be in the payload if the extended set is off.");
  }

  
  
  Assert.ok(!("slowSQLStartup" in ping.payload),
            "slowSQLStartup must not be sent if the extended set is off");

  Assert.ok(!("addonManager" in ping.payload.simpleMeasurements),
            "addonManager must not be sent if the extended set is off.");
  Assert.ok(!("UITelemetry" in ping.payload.simpleMeasurements),
            "UITelemetry must not be sent if the extended set is off.");

  
  Telemetry.canRecordExtended = true;

  
  yield TelemetrySession.reset();
  yield sendPing();
  request = yield gRequestIterator.next();
  ping = decodeRequestPayload(request);
  checkPingFormat(ping, PING_TYPE_MAIN, true, true);

  
  for (let f in EXTENDED_PAYLOAD_FIELDS) {
    Assert.ok(EXTENDED_PAYLOAD_FIELDS[f] in ping.payload,
              EXTENDED_PAYLOAD_FIELDS[f] + " must be in the payload if the extended set is on.");
  }

  Assert.ok("addonManager" in ping.payload.simpleMeasurements,
            "addonManager must be sent if the extended set is on.");
  Assert.ok("UITelemetry" in ping.payload.simpleMeasurements,
            "UITelemetry must be sent if the extended set is on.");
});

add_task(function* test_schedulerUserIdle() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  const SCHEDULER_TICK_INTERVAL_MS = 5 * 60 * 1000;
  const SCHEDULER_TICK_IDLE_INTERVAL_MS = 60 * 60 * 1000;

  let now = new Date(2010, 1, 1, 11, 0, 0);
  fakeNow(now);

  let schedulerTimeout = 0;
  fakeSchedulerTimer((callback, timeout) => {
    schedulerTimeout = timeout;
  }, () => {});
  yield TelemetrySession.reset();
  gRequestIterator = Iterator(new Request());

  
  Assert.equal(schedulerTimeout, SCHEDULER_TICK_INTERVAL_MS);

  
  fakeIdleNotification("idle");

  
  Assert.equal(schedulerTimeout, SCHEDULER_TICK_IDLE_INTERVAL_MS);

  
  fakeIdleNotification("active");

  
  Assert.equal(schedulerTimeout, SCHEDULER_TICK_INTERVAL_MS);

  
  now.setHours(23);
  now.setMinutes(50);
  fakeIdleNotification("idle");
  Assert.equal(schedulerTimeout, 10 * 60 * 1000);

  yield TelemetrySession.shutdown();
});

add_task(function* test_sendDailyOnIdle() {
  if (gIsAndroid || gIsGonk) {
    
    return;
  }

  let now = new Date(2040, 1, 1, 11, 0, 0);
  fakeNow(now);

  let schedulerTickCallback = 0;
  fakeSchedulerTimer((callback, timeout) => {
    schedulerTickCallback = callback;
  }, () => {});
  yield TelemetrySession.reset();

  
  now = new Date(2040, 1, 1, 23, 55, 0);
  fakeNow(now);
  registerPingHandler((req, res) => {
    Assert.ok(false, "No daily ping should be received yet when the user is active.");
  });
  yield fakeIdleNotification("active");

  
  gRequestIterator = Iterator(new Request());

  
  now = new Date(2040, 1, 2, 0, 05, 0);
  fakeNow(now);
  yield schedulerTickCallback();

  let request = yield gRequestIterator.next();
  Assert.ok(!!request);
  let ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);

  
  now = new Date(2040, 1, 2, 23, 55, 0);
  fakeNow(now);
  yield fakeIdleNotification("idle");

  request = yield gRequestIterator.next();
  Assert.ok(!!request);
  ping = decodeRequestPayload(request);

  Assert.equal(ping.type, PING_TYPE_MAIN);
  Assert.equal(ping.payload.info.reason, REASON_DAILY);

  yield TelemetrySession.shutdown();
});

add_task(function* stopServer(){
  gHttpServer.stop(do_test_finished);
});


function Request() {
  let defers = [];
  let current = 0;

  function RequestIterator() {}

  
  RequestIterator.prototype.next = function() {
    let deferred = defers[current++];
    return deferred.promise;
  }

  this.__iterator__ = function(){
    return new RequestIterator();
  }

  registerPingHandler((request, response) => {
    let deferred = defers[defers.length - 1];
    defers.push(Promise.defer());
    deferred.resolve(request);
  });

  defers.push(Promise.defer());
}
