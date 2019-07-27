









Cu.import("resource://gre/modules/ClientID.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetryStorage.jsm", this);
Cu.import("resource://gre/modules/TelemetrySend.jsm", this);
Cu.import("resource://gre/modules/TelemetryArchive.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");

const PING_FORMAT_VERSION = 4;
const DELETION_PING_TYPE = "deletion";
const TEST_PING_TYPE = "test-ping-type";

const PLATFORM_VERSION = "1.9.2";
const APP_VERSION = "1";
const APP_NAME = "XPCShell";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_ARCHIVE_ENABLED = PREF_BRANCH + "archive.enabled";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_FHR_SERVICE_ENABLED = "datareporting.healthreport.service.enabled";
const PREF_UNIFIED = PREF_BRANCH + "unified";

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);

let gClientID = null;

function sendPing(aSendClientId, aSendEnvironment) {
  if (PingServer.started) {
    TelemetrySend.setServer("http://localhost:" + PingServer.port);
  } else {
    TelemetrySend.setServer("http://doesnotexist");
  }

  let options = {
    addClientId: aSendClientId,
    addEnvironment: aSendEnvironment,
  };
  return TelemetryController.submitExternalPing(TEST_PING_TYPE, {}, options);
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
    Assert.ok(f in aPing, f + " must be available.");
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

function run_test() {
  do_test_pending();

  
  do_get_profile();
  loadAddonManager("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  Services.prefs.setBoolPref(PREF_ENABLED, true);
  Services.prefs.setBoolPref(PREF_FHR_UPLOAD_ENABLED, true);
  Services.prefs.setBoolPref(PREF_FHR_SERVICE_ENABLED, true);

  Telemetry.asyncFetchTelemetryData(wrapWithExceptionHandler(run_next_test));
}

add_task(function* asyncSetup() {
  yield TelemetryController.setup();

  gClientID = yield ClientID.getClientID();

  
  
  let promisePingSetup = TelemetryController.reset();
  do_check_eq(TelemetryController.clientID, gClientID);
  yield promisePingSetup;
});


add_task(function* test_overwritePing() {
  let ping = {id: "foo"};
  yield TelemetryStorage.savePing(ping, true);
  yield TelemetryStorage.savePing(ping, false);
  yield TelemetryStorage.cleanupPingFile(ping);
});


add_task(function* test_simplePing() {
  PingServer.start();

  yield sendPing(false, false);
  let request = yield PingServer.promiseNextRequest();

  
  Assert.notEqual(request.queryString, "");

  
  let params = request.queryString.split("&");
  Assert.ok(params.find(p => p == ("v=" + PING_FORMAT_VERSION)));

  let ping = decodeRequestPayload(request);
  checkPingFormat(ping, TEST_PING_TYPE, false, false);
});

add_task(function* test_deletionPing() {
  const isUnified = Preferences.get(PREF_UNIFIED, false);
  if (!isUnified) {
    
    
    return;
  }

  
  Preferences.set(PREF_FHR_UPLOAD_ENABLED, false);

  let ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, DELETION_PING_TYPE, true, false);

  
  Preferences.set(PREF_FHR_UPLOAD_ENABLED, true);
});

add_task(function* test_pingHasClientId() {
  
  yield sendPing(true, false);

  let ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, TEST_PING_TYPE, true, false);

  if (HAS_DATAREPORTINGSERVICE &&
      Services.prefs.getBoolPref(PREF_FHR_UPLOAD_ENABLED)) {
    Assert.equal(ping.clientId, gClientID,
                 "The correct clientId must be reported.");
  }
});

add_task(function* test_pingHasEnvironment() {
  
  yield sendPing(false, true);
  let ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, TEST_PING_TYPE, false, true);

  
  Assert.equal(ping.application.buildId, ping.environment.build.buildId);
});

add_task(function* test_pingHasEnvironmentAndClientId() {
  
  yield sendPing(true, true);
  let ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, TEST_PING_TYPE, true, true);

  
  Assert.equal(ping.application.buildId, ping.environment.build.buildId);
  
  if (HAS_DATAREPORTINGSERVICE &&
      Services.prefs.getBoolPref(PREF_FHR_UPLOAD_ENABLED)) {
    Assert.equal(ping.clientId, gClientID,
                 "The correct clientId must be reported.");
  }
});

add_task(function* test_archivePings() {
  const ARCHIVE_PATH =
    OS.Path.join(OS.Constants.Path.profileDir, "datareporting", "archived");

  let now = new Date(2009, 10, 18, 12, 0, 0);
  fakeNow(now);

  
  
  
  const isUnified = Preferences.get(PREF_UNIFIED, false);
  const uploadPref = isUnified ? PREF_FHR_UPLOAD_ENABLED : PREF_ENABLED;
  Preferences.set(uploadPref, false);

  
  
  if (isUnified) {
    let ping = yield PingServer.promiseNextPing();
    checkPingFormat(ping, DELETION_PING_TYPE, true, false);
  }

  
  PingServer.registerPingHandler(() => Assert.ok(false, "Telemetry must not send pings if not allowed to."));
  let pingId = yield sendPing(true, true);

  
  let ping = yield TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.equal(ping.id, pingId, "TelemetryController should still archive pings.");

  
  now = new Date(2010, 10, 18, 12, 0, 0);
  fakeNow(now);
  Preferences.set(PREF_ARCHIVE_ENABLED, false);
  pingId = yield sendPing(true, true);
  let promise = TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.ok((yield promiseRejects(promise)),
    "TelemetryController should not archive pings if the archive pref is disabled.");

  
  Preferences.set(uploadPref, true);
  Preferences.set(PREF_ARCHIVE_ENABLED, true);

  now = new Date(2014, 06, 18, 22, 0, 0);
  fakeNow(now);
  
  PingServer.resetPingHandler();
  pingId = yield sendPing(true, true);

  
  yield PingServer.promiseNextPing();
  ping = yield TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.equal(ping.id, pingId,
    "TelemetryController should still archive pings if ping upload is enabled.");
});



add_task(function* test_midnightPingSendFuzzing() {
  const fuzzingDelay = 60 * 60 * 1000;
  fakeMidnightPingFuzzingDelay(fuzzingDelay);
  let now = new Date(2030, 5, 1, 11, 00, 0);
  fakeNow(now);

  let waitForTimer = () => new Promise(resolve => {
    fakePingSendTimer((callback, timeout) => {
      resolve([callback, timeout]);
    }, () => {});
  });

  PingServer.clearRequests();
  yield TelemetryController.reset();

  
  now = new Date(2030, 5, 2, 0, 40, 0);
  fakeNow(now);
  PingServer.registerPingHandler((req, res) => {
    Assert.ok(false, "No ping should be received yet.");
  });
  let timerPromise = waitForTimer();
  yield sendPing(true, true);
  let [timerCallback, timerTimeout] = yield timerPromise;
  Assert.ok(!!timerCallback);
  Assert.deepEqual(futureDate(now, timerTimeout), new Date(2030, 5, 2, 1, 0, 0));

  
  now = new Date(2030, 5, 2, 0, 59, 59);
  fakeNow(now);
  timerPromise = waitForTimer();
  yield sendPing(true, true);
  [timerCallback, timerTimeout] = yield timerPromise;
  Assert.deepEqual(timerTimeout, 1 * 1000);

  
  PingServer.resetPingHandler();

  
  
  now = futureDate(now, timerTimeout);
  fakeNow(now);
  yield timerCallback();
  const pings = yield PingServer.promiseNextPings(2);
  for (let ping of pings) {
    checkPingFormat(ping, TEST_PING_TYPE, true, true);
  }
  yield TelemetrySend.testWaitOnOutgoingPings();

  
  now = futureDate(now, 5 * 60 * 1000);
  yield sendPing(true, true);
  let ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, TEST_PING_TYPE, true, true);
  yield TelemetrySend.testWaitOnOutgoingPings();

  
  now = fakeNow(2030, 5, 3, 23, 59, 0);
  yield sendPing(true, true);
  ping = yield PingServer.promiseNextPing();
  checkPingFormat(ping, TEST_PING_TYPE, true, true);
  yield TelemetrySend.testWaitOnOutgoingPings();

  
  fakeMidnightPingFuzzingDelay(0);
  fakePingSendTimer(() => {}, () => {});
});

add_task(function* test_changePingAfterSubmission() {
  
  let payload = { canary: "test" };
  let pingPromise = TelemetryController.submitExternalPing(TEST_PING_TYPE, payload, options);

  
  payload.canary = "changed";

  
  const pingId = yield pingPromise;

  
  let archivedCopy = yield TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.equal(archivedCopy.payload.canary, "test",
               "The payload must not be changed after being submitted.");
});

add_task(function* stopServer(){
  yield PingServer.stop();
  do_test_finished();
});
