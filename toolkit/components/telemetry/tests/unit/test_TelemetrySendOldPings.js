











"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://testing-common/httpd.js", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/TelemetryFile.jsm", this);
Cu.import("resource://gre/modules/TelemetryPing.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let {OS: {File, Path, Constants}} = Cu.import("resource://gre/modules/osfile.jsm", {});

XPCOMUtils.defineLazyGetter(this, "gDatareportingService",
  () => Cc["@mozilla.org/datareporting/service;1"]
          .getService(Ci.nsISupports)
          .wrappedJSObject);




const ONE_MINUTE_MS = 60 * 1000;
const EXPIRED_PING_FILE_AGE = TelemetryFile.MAX_PING_FILE_AGE + ONE_MINUTE_MS;
const OVERDUE_PING_FILE_AGE = TelemetryFile.OVERDUE_PING_FILE_AGE + ONE_MINUTE_MS;

const PING_SAVE_FOLDER = "saved-telemetry-pings";
const PING_TIMEOUT_LENGTH = 5000;
const EXPIRED_PINGS = 5;
const OVERDUE_PINGS = 6;
const OLD_FORMAT_PINGS = 4;
const RECENT_PINGS = 4;
const LRU_PINGS = TelemetryFile.MAX_LRU_PINGS;

const TOTAL_EXPECTED_PINGS = OVERDUE_PINGS + RECENT_PINGS + OLD_FORMAT_PINGS;

let gHttpServer = new HttpServer();
let gCreatedPings = 0;
let gSeenPings = 0;













let createSavedPings = Task.async(function* (aPingInfos) {
  let pingIds = [];
  let now = Date.now();

  for (let type in aPingInfos) {
    let num = aPingInfos[type].num;
    let age = now - aPingInfos[type].age;
    for (let i = 0; i < num; ++i) {
      let pingId = yield TelemetryPing.savePing("test-ping", {}, { overwrite: true });
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
    let filePath = getSavePathForPingId(pingId);
    yield File.remove(filePath);
  }
});







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







function stopHttpServer() {
  let deferred = Promise.defer();
  gHttpServer.stop(function() {
    deferred.resolve();
  })
  return deferred.promise;
}




function resetTelemetry() {
  
  
  let gen = TelemetryFile.popPendingPings();
  for (let item of gen) {};
}





function startTelemetry() {
  return TelemetryPing.setup();
}

function run_test() {
  gHttpServer.registerPrefixHandler("/submit/telemetry/", pingHandler);
  gHttpServer.start(-1);
  do_get_profile();
  loadAddonManager("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  
  gDatareportingService.observe(null, "app-startup", null);
  gDatareportingService.observe(null, "profile-after-change", null);

  Services.prefs.setBoolPref(TelemetryPing.Constants.PREF_ENABLED, true);
  Services.prefs.setCharPref(TelemetryPing.Constants.PREF_SERVER,
                             "http://localhost:" + gHttpServer.identity.primaryPort);
  run_next_test();
}





add_task(function* setupEnvironment() {
  yield TelemetryPing.setup();

  let directory = TelemetryFile.pingDirectoryPath;
  yield File.makeDir(directory, { ignoreExisting: true, unixMode: OS.Constants.S_IRWXU });

  yield resetTelemetry();
});





add_task(function* test_expired_pings_are_deleted() {
  let pingTypes = [{ num: EXPIRED_PINGS, age: EXPIRED_PING_FILE_AGE }];
  let expiredPings = yield createSavedPings(pingTypes);
  yield startTelemetry();
  assertReceivedPings(0);
  yield assertNotSaved(expiredPings);
  yield resetTelemetry();
});




add_task(function* test_recent_pings_not_sent() {
  let pingTypes = [{ num: RECENT_PINGS }];
  let recentPings = yield createSavedPings(pingTypes);
  yield startTelemetry();
  assertReceivedPings(0);
  yield resetTelemetry();
  yield clearPings(recentPings);
});




add_task(function* test_most_recent_pings_kept() {
  let pingTypes = [
    { num: LRU_PINGS },
    { num: 3, age: ONE_MINUTE_MS },
  ];
  let pings = yield createSavedPings(pingTypes);
  let head = pings.slice(0, LRU_PINGS);
  let tail = pings.slice(-3);

  yield startTelemetry();
  let gen = TelemetryFile.popPendingPings();

  for (let item of gen) {
    for (let id of tail) {
      do_check_neq(id, item.id);
    }
  }

  assertNotSaved(tail);
  yield resetTelemetry();
  yield clearPings(pings);
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
    Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, PING_OLD_FORMAT.slug),
    Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, PING_NO_INFO.slug),
    Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, PING_NO_PAYLOAD.slug),
    Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, "no-slug-file"),
  ];

  
  yield TelemetryFile.savePing(PING_OLD_FORMAT, true);
  yield TelemetryFile.savePing(PING_NO_INFO, true);
  yield TelemetryFile.savePing(PING_NO_PAYLOAD, true);
  yield TelemetryFile.savePingToFile(PING_NO_SLUG, PING_FILES_PATHS[3], true);

  for (let f in PING_FILES_PATHS) {
    yield File.setDates(PING_FILES_PATHS[f], null, Date.now() - OVERDUE_PING_FILE_AGE);
  }

  yield startTelemetry();
  assertReceivedPings(OLD_FORMAT_PINGS);

  
  
  yield OS.File.remove(PING_FILES_PATHS[3]);

  yield resetTelemetry();
});






add_task(function* test_overdue_pings_trigger_send() {
  let pingTypes = [
    { num: RECENT_PINGS },
    { num: EXPIRED_PINGS, age: EXPIRED_PING_FILE_AGE },
    { num: OVERDUE_PINGS, age: OVERDUE_PING_FILE_AGE },
  ];
  let pings = yield createSavedPings(pingTypes);
  let recentPings = pings.slice(0, RECENT_PINGS);
  let expiredPings = pings.slice(RECENT_PINGS, RECENT_PINGS + EXPIRED_PINGS);
  let overduePings = pings.slice(-OVERDUE_PINGS);

  yield startTelemetry();
  assertReceivedPings(TOTAL_EXPECTED_PINGS);

  yield assertNotSaved(recentPings);
  yield assertNotSaved(expiredPings);
  yield assertNotSaved(overduePings);
  yield resetTelemetry();
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

  
  yield TelemetryFile.savePing(PING_OLD_FORMAT, true);
  yield File.setDates(filePath, null, Date.now() - OVERDUE_PING_FILE_AGE);

  let receivedPings = 0;
  
  gHttpServer.registerPrefixHandler("/submit/telemetry/", request => {
    
    Assert.notEqual(request.queryString, "");

    
    let params = request.queryString.split("&");
    Assert.ok(params.find(p => p == "v=1"));

    receivedPings++;
  });

  yield startTelemetry();
  Assert.equal(receivedPings, 1, "We must receive a ping in the old format.");

  yield resetTelemetry();
});

add_task(function* teardown() {
  yield stopHttpServer();
});
