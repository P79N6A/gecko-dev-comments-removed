









Cu.import("resource://testing-common/httpd.js", this);
Cu.import("resource://gre/modules/ClientID.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetryStorage.jsm", this);
Cu.import("resource://gre/modules/TelemetryArchive.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");

const PING_FORMAT_VERSION = 4;
const TEST_PING_TYPE = "test-ping-type";
const TEST_PING_RETENTION = 180;

const PLATFORM_VERSION = "1.9.2";
const APP_VERSION = "1";
const APP_NAME = "XPCShell";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_ARCHIVE_ENABLED = PREF_BRANCH + "archive.enabled";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_FHR_SERVICE_ENABLED = "datareporting.healthreport.service.enabled";

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);

let gHttpServer = new HttpServer();
let gServerStarted = false;
let gRequestIterator = null;
let gClientID = null;

function sendPing(aSendClientId, aSendEnvironment) {
  if (gServerStarted) {
    TelemetryController.setServer("http://localhost:" + gHttpServer.identity.primaryPort);
  } else {
    TelemetryController.setServer("http://doesnotexist");
  }

  let options = {
    addClientId: aSendClientId,
    addEnvironment: aSendEnvironment,
    retentionDays: TEST_PING_RETENTION,
  };
  return TelemetryController.submitExternalPing(TEST_PING_TYPE, {}, options);
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

function registerPingHandler(handler) {
  gHttpServer.registerPrefixHandler("/submit/telemetry/",
				   wrapWithExceptionHandler(handler));
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




function startWebserver() {
  gHttpServer.start(-1);
  gServerStarted = true;
  gRequestIterator = Iterator(new Request());
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
  let ping = {id: "foo"}
  yield TelemetryStorage.savePing(ping, true);
  yield TelemetryStorage.savePing(ping, false);
  yield TelemetryStorage.cleanupPingFile(ping);
});


add_task(function* test_noServerPing() {
  yield sendPing(false, false);
});


add_task(function* test_simplePing() {
  startWebserver();

  yield sendPing(false, false);
  let request = yield gRequestIterator.next();

  
  Assert.notEqual(request.queryString, "");

  
  let params = request.queryString.split("&");
  Assert.ok(params.find(p => p == ("v=" + PING_FORMAT_VERSION)));

  let ping = decodeRequestPayload(request);
  checkPingFormat(ping, TEST_PING_TYPE, false, false);
});

add_task(function* test_pingHasClientId() {
  
  yield sendPing(true, false);

  let request = yield gRequestIterator.next();
  let ping = decodeRequestPayload(request);
  checkPingFormat(ping, TEST_PING_TYPE, true, false);

  if (HAS_DATAREPORTINGSERVICE &&
      Services.prefs.getBoolPref(PREF_FHR_UPLOAD_ENABLED)) {
    Assert.equal(ping.clientId, gClientID,
                 "The correct clientId must be reported.");
  }
});

add_task(function* test_pingHasEnvironment() {
  
  yield sendPing(false, true);
  let request = yield gRequestIterator.next();
  let ping = decodeRequestPayload(request);
  checkPingFormat(ping, TEST_PING_TYPE, false, true);

  
  Assert.equal(ping.application.buildId, ping.environment.build.buildId);
});

add_task(function* test_pingHasEnvironmentAndClientId() {
  
  yield sendPing(true, true);
  let request = yield gRequestIterator.next();
  let ping = decodeRequestPayload(request);
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

  
  Preferences.set(PREF_FHR_UPLOAD_ENABLED, false);

  
  registerPingHandler(() => Assert.ok(false, "Telemetry must not send pings if not allowed to."));
  let pingId = yield sendPing(true, true);

  
  let ping = yield TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.equal(ping.id, pingId, "TelemetryController must archive pings if FHR is enabled.");

  
  now = new Date(2010, 10, 18, 12, 0, 0);
  fakeNow(now);
  Preferences.set(PREF_ARCHIVE_ENABLED, false);
  pingId = yield sendPing(true, true);
  let promise = TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.ok((yield promiseRejects(promise)),
            "TelemetryController must not archive pings if the archive pref is disabled.");

  
  Preferences.set(PREF_FHR_UPLOAD_ENABLED, true);
  Preferences.set(PREF_ARCHIVE_ENABLED, true);

  now = new Date(2014, 06, 18, 22, 0, 0);
  fakeNow(now);
  
  gRequestIterator = Iterator(new Request());
  pingId = yield sendPing(true, true);

  
  yield gRequestIterator.next();
  ping = yield TelemetryArchive.promiseArchivedPingById(pingId);
  Assert.equal(ping.id, pingId, "TelemetryController must archive pings if FHR is enabled.");
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
