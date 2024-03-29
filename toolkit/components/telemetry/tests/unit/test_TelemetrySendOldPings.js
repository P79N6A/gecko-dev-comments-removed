








"use strict"

Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/TelemetryStorage.jsm", this);
Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetrySend.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let {OS: {File, Path, Constants}} = Cu.import("resource://gre/modules/osfile.jsm", {});

XPCOMUtils.defineLazyGetter(this, "gDatareportingService",
  () => Cc["@mozilla.org/datareporting/service;1"]
          .getService(Ci.nsISupports)
          .wrappedJSObject);

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);




const ONE_MINUTE_MS = 60 * 1000;
const OVERDUE_PING_FILE_AGE = TelemetrySend.OVERDUE_PING_FILE_AGE + ONE_MINUTE_MS;

const PING_SAVE_FOLDER = "saved-telemetry-pings";
const PING_TIMEOUT_LENGTH = 5000;
const OVERDUE_PINGS = 6;
const OLD_FORMAT_PINGS = 4;
const RECENT_PINGS = 4;

const TOTAL_EXPECTED_PINGS = OVERDUE_PINGS + RECENT_PINGS + OLD_FORMAT_PINGS;

let gCreatedPings = 0;
let gSeenPings = 0;













let createSavedPings = Task.async(function* (aPingInfos) {
  let pingIds = [];
  let now = Date.now();

  for (let type in aPingInfos) {
    let num = aPingInfos[type].num;
    let age = now - aPingInfos[type].age;
    for (let i = 0; i < num; ++i) {
      let pingId = yield TelemetryController.addPendingPing("test-ping", {}, { overwrite: true });
      if (aPingInfos[type].age) {
        
        
        let filePath = getSavePathForPingId(pingId);
        yield File.setDates(filePath, null, age);
      }
      gCreatedPings++;
      pingIds.push(pingId);
    }
  }

  return pingIds;
});







let clearPings = Task.async(function* (aPingIds) {
  for (let pingId of aPingIds) {
    yield TelemetryStorage.removePendingPing(pingId);
  }
});





function fakePendingPingsQuota(aPendingQuota) {
  let storage = Cu.import("resource://gre/modules/TelemetryStorage.jsm");
  storage.Policy.getPendingPingsQuota = () => aPendingQuota;
}







function getSavePathForPingId(aPingId) {
  return Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, aPingId);
}







function assertReceivedPings(aExpectedNum) {
  do_check_eq(gSeenPings, aExpectedNum);
}







let assertNotSaved = Task.async(function* (aPingIds) {
  let saved = 0;
  for (let id of aPingIds) {
    let filePath = getSavePathForPingId(id);
    if (yield File.exists(filePath)) {
      saved++;
    }
  }
  if (saved > 0) {
    do_throw("Found " + saved + " unexpected saved pings.");
  }
});








function pingHandler(aRequest) {
  gSeenPings++;
}




let clearPendingPings = Task.async(function*() {
  const pending = yield TelemetryStorage.loadPendingPingList();
  for (let p of pending) {
    yield TelemetryStorage.removePendingPing(p.id);
  }
});

function run_test() {
  PingServer.start();
  PingServer.registerPingHandler(pingHandler);
  do_get_profile();
  loadAddonManager("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  
  gDatareportingService.observe(null, "app-startup", null);
  gDatareportingService.observe(null, "profile-after-change", null);

  Services.prefs.setBoolPref(TelemetryController.Constants.PREF_ENABLED, true);
  Services.prefs.setCharPref(TelemetryController.Constants.PREF_SERVER,
                             "http://localhost:" + PingServer.port);
  run_next_test();
}





add_task(function* setupEnvironment() {
  yield TelemetryController.setup();

  let directory = TelemetryStorage.pingDirectoryPath;
  yield File.makeDir(directory, { ignoreExisting: true, unixMode: OS.Constants.S_IRWXU });

  yield clearPendingPings();
});




add_task(function* test_recent_pings_sent() {
  let pingTypes = [{ num: RECENT_PINGS }];
  let recentPings = yield createSavedPings(pingTypes);

  yield TelemetryController.reset();
  yield TelemetrySend.testWaitOnOutgoingPings();
  assertReceivedPings(RECENT_PINGS);

  yield clearPendingPings();
});




add_task(function* test_overdue_old_format() {
  
  const PING_OLD_FORMAT = {
    slug: "1234567abcd",
    reason: "test-ping",
    payload: {
      info: {
        reason: "test-ping",
        OS: "XPCShell",
        appID: "SomeId",
        appVersion: "1.0",
        appName: "XPCShell",
        appBuildID: "123456789",
        appUpdateChannel: "Test",
        platformBuildID: "987654321",
      },
    },
  };

  
  const PING_NO_INFO = {
    slug: "1234-no-info-ping",
    reason: "test-ping",
    payload: {}
  };

  
  const PING_NO_PAYLOAD = {
    slug: "5678-no-payload",
    reason: "test-ping",
  };

  
  const PING_NO_SLUG = {
    reason: "test-ping",
    payload: {}
  };

  const PING_FILES_PATHS = [
    getSavePathForPingId(PING_OLD_FORMAT.slug),
    getSavePathForPingId(PING_NO_INFO.slug),
    getSavePathForPingId(PING_NO_PAYLOAD.slug),
    getSavePathForPingId("no-slug-file"),
  ];

  
  yield TelemetryStorage.savePing(PING_OLD_FORMAT, true);
  yield TelemetryStorage.savePing(PING_NO_INFO, true);
  yield TelemetryStorage.savePing(PING_NO_PAYLOAD, true);
  yield TelemetryStorage.savePingToFile(PING_NO_SLUG, PING_FILES_PATHS[3], true);

  for (let f in PING_FILES_PATHS) {
    yield File.setDates(PING_FILES_PATHS[f], null, Date.now() - OVERDUE_PING_FILE_AGE);
  }

  gSeenPings = 0;
  yield TelemetryController.reset();
  yield TelemetrySend.testWaitOnOutgoingPings();
  assertReceivedPings(OLD_FORMAT_PINGS);

  
  
  yield OS.File.remove(PING_FILES_PATHS[3]);

  yield clearPendingPings();
});




add_task(function* test_overdue_pings_trigger_send() {
  let pingTypes = [
    { num: RECENT_PINGS },
    { num: OVERDUE_PINGS, age: OVERDUE_PING_FILE_AGE },
  ];
  let pings = yield createSavedPings(pingTypes);
  let recentPings = pings.slice(0, RECENT_PINGS);
  let overduePings = pings.slice(-OVERDUE_PINGS);

  yield TelemetryController.reset();
  yield TelemetrySend.testWaitOnOutgoingPings();
  assertReceivedPings(TOTAL_EXPECTED_PINGS);

  yield assertNotSaved(recentPings);
  yield assertNotSaved(overduePings);

  Assert.equal(TelemetrySend.overduePingsCount, overduePings.length,
               "Should have tracked the correct amount of overdue pings");

  yield clearPendingPings();
});





add_task(function* test_overdue_old_format() {
  
  const PING_OLD_FORMAT = {
    slug: "1234567abcd",
    reason: "test-ping",
    payload: {
      info: {
        reason: "test-ping",
        OS: "XPCShell",
        appID: "SomeId",
        appVersion: "1.0",
        appName: "XPCShell",
        appBuildID: "123456789",
        appUpdateChannel: "Test",
        platformBuildID: "987654321",
      },
    },
  };

  const filePath =
    Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, PING_OLD_FORMAT.slug);

  
  yield TelemetryStorage.savePing(PING_OLD_FORMAT, true);
  yield File.setDates(filePath, null, Date.now() - OVERDUE_PING_FILE_AGE);

  let receivedPings = 0;
  
  PingServer.registerPingHandler(request => {
    
    Assert.notEqual(request.queryString, "");

    
    let params = request.queryString.split("&");
    Assert.ok(params.find(p => p == "v=1"));

    receivedPings++;
  });

  yield TelemetryController.reset();
  yield TelemetrySend.testWaitOnOutgoingPings();
  Assert.equal(receivedPings, 1, "We must receive a ping in the old format.");

  yield clearPendingPings();
  PingServer.resetPingHandler();
});

add_task(function* test_pendingPingsQuota() {
  const PING_TYPE = "foo";
  const PREF_FHR_UPLOAD = "datareporting.healthreport.uploadEnabled";

  
  Services.prefs.setBoolPref(PREF_FHR_UPLOAD, false);

  
  
  yield clearPendingPings();
  yield TelemetryController.reset();
  yield TelemetrySend.testWaitOnOutgoingPings();
  yield TelemetryStorage.testPendingQuotaTaskPromise();

  
  yield clearPendingPings();

  let expectedPrunedPings = [];
  let expectedNotPrunedPings = [];

  let checkPendingPings = Task.async(function*() {
    
    for (let prunedPingId of expectedPrunedPings) {
      yield Assert.rejects(TelemetryStorage.loadPendingPing(prunedPingId),
                           "Ping " + prunedPingId + " should have been pruned.");
      const pingPath = getSavePathForPingId(prunedPingId);
      Assert.ok(!(yield OS.File.exists(pingPath)), "The ping should not be on the disk anymore.");
    }

    
    for (let expectedPingId of expectedNotPrunedPings) {
      Assert.ok((yield TelemetryStorage.loadPendingPing(expectedPingId)),
                "Ping" + expectedPingId + " should be among the pending pings.");
    }
  });

  let pendingPingsInfo = [];
  let pingsSizeInBytes = 0;

  
  for (let days = 1; days < 11; days++) {
    const date = fakeNow(2010, 1, days, 1, 1, 0);
    const pingId = yield TelemetryController.addPendingPing(PING_TYPE, {}, {});

    
    const pingFilePath = getSavePathForPingId(pingId);
    const pingSize = (yield OS.File.stat(pingFilePath)).size;
    
    pendingPingsInfo.unshift({id: pingId, size: pingSize, timestamp: date.getTime() });

    
    yield OS.File.setDates(pingFilePath, null, date.getTime());

    
    pingsSizeInBytes += pingSize;
  }

  
  
  Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_SIZE_MB").clear();
  Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_EVICTED_OVER_QUOTA").clear();
  Telemetry.getHistogramById("TELEMETRY_PENDING_EVICTING_OVER_QUOTA_MS").clear();

  yield TelemetryController.reset();
  yield TelemetryStorage.testPendingQuotaTaskPromise();

  
  let h = Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_SIZE_MB").snapshot();
  Assert.equal(h.sum, Math.round(pingsSizeInBytes / 1024 / 1024),
               "Telemetry must report the correct pending pings directory size.");
  h = Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_EVICTED_OVER_QUOTA").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report 0 evictions if quota is not hit.");
  h = Telemetry.getHistogramById("TELEMETRY_PENDING_EVICTING_OVER_QUOTA_MS").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report a null elapsed time if quota is not hit.");

  
  const testQuotaInBytes = pingsSizeInBytes * 0.8;
  fakePendingPingsQuota(testQuotaInBytes);

  
  
  const safeQuotaSize = Math.round(testQuotaInBytes * 0.9);
  let sizeInBytes = 0;
  let pingsWithinQuota = [];
  let pingsOutsideQuota = [];

  for (let pingInfo of pendingPingsInfo) {
    sizeInBytes += pingInfo.size;
    if (sizeInBytes >= safeQuotaSize) {
      pingsOutsideQuota.push(pingInfo.id);
      continue;
    }
    pingsWithinQuota.push(pingInfo.id);
  }

  expectedNotPrunedPings = pingsWithinQuota;
  expectedPrunedPings = pingsOutsideQuota;

  
  yield TelemetryController.reset();
  yield TelemetryStorage.testPendingQuotaTaskPromise();
  yield checkPendingPings();

  h = Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_EVICTED_OVER_QUOTA").snapshot();
  Assert.equal(h.sum, pingsOutsideQuota.length,
               "Telemetry must correctly report the over quota pings evicted from the pending pings directory.");
  h = Telemetry.getHistogramById("TELEMETRY_PENDING_PINGS_SIZE_MB").snapshot();
  Assert.equal(h.sum, 17, "Pending pings quota was hit, a special size must be reported.");

  
  yield TelemetryController.reset();
  yield TelemetryStorage.testPendingQuotaTaskPromise();
  yield checkPendingPings();

  Services.prefs.setBoolPref(PREF_FHR_UPLOAD, true);
});

add_task(function* teardown() {
  yield PingServer.stop();
});
