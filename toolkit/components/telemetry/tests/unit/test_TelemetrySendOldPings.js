











"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://testing-common/httpd.js", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/TelemetryFile.jsm", this);
Cu.import("resource://gre/modules/TelemetryPing.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
let {OS: {File, Path, Constants}} = Cu.import("resource://gre/modules/osfile.jsm", {});




const ONE_MINUTE_MS = 60 * 1000;
const EXPIRED_PING_FILE_AGE = TelemetryFile.MAX_PING_FILE_AGE + ONE_MINUTE_MS;
const OVERDUE_PING_FILE_AGE = TelemetryFile.OVERDUE_PING_FILE_AGE + ONE_MINUTE_MS;

const PING_SAVE_FOLDER = "saved-telemetry-pings";
const PING_TIMEOUT_LENGTH = 5000;
const EXPIRED_PINGS = 5;
const OVERDUE_PINGS = 6;
const RECENT_PINGS = 4;
const LRU_PINGS = TelemetryFile.MAX_LRU_PINGS;

const TOTAL_EXPECTED_PINGS = OVERDUE_PINGS + RECENT_PINGS;

let gHttpServer = new HttpServer();
let gCreatedPings = 0;
let gSeenPings = 0;













function createSavedPings(aNum, aAge) {
  return Task.spawn(function*(){
    
    
    
    let pings = [];
    let age = Date.now() - aAge;

    for (let i = 0; i < aNum; ++i) {
      let payload = TelemetryPing.getPayload();
      let ping = { slug: "test-ping-" + gCreatedPings, reason: "test", payload: payload };

      yield TelemetryFile.savePing(ping);

      if (aAge) {
        
        
        let file = getSavePathForPing(ping);
        yield File.setDates(file, null, age);
      }
      gCreatedPings++;
      pings.push(ping);
    }
    return pings;
  });
}








function clearPings(aPings) {
  return Task.spawn(function*() {
    for (let ping of aPings) {
      let path = getSavePathForPing(ping);
      yield File.remove(path);
    }
  });
}







function getSavePathForPing(aPing) {
  return Path.join(Constants.Path.profileDir, PING_SAVE_FOLDER, aPing.slug);
}







function assertReceivedPings(aExpectedNum) {
  do_check_eq(gSeenPings, aExpectedNum);
}







function assertNotSaved(aPings) {
  return Task.spawn(function*() {
    let saved = 0;
    for (let ping of aPings) {
      let file = getSavePathForPing(ping);
      if (yield File.exists()) {
        saved++;
      }
    }
    if (saved > 0) {
      do_throw("Found " + saved + " unexpected saved pings.");
    }
  });
}








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
  TelemetryPing.uninstall();
  
  
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
  Services.prefs.setBoolPref(TelemetryPing.Constants.PREF_ENABLED, true);
  Services.prefs.setCharPref(TelemetryPing.Constants.PREF_SERVER,
                             "http://localhost:" + gHttpServer.identity.primaryPort);
  run_next_test();
}





add_task(function* test_expired_pings_are_deleted() {
  let expiredPings = yield createSavedPings(EXPIRED_PINGS, EXPIRED_PING_FILE_AGE);
  yield startTelemetry();
  assertReceivedPings(0);
  yield assertNotSaved(expiredPings);
  yield resetTelemetry();
});




add_task(function* test_recent_pings_not_sent() {
  let recentPings = yield createSavedPings(RECENT_PINGS);
  yield startTelemetry();
  assertReceivedPings(0);
  yield resetTelemetry();
  yield clearPings(recentPings);
});




add_task(function* test_most_recent_pings_kept() {
  let head = yield createSavedPings(LRU_PINGS);
  let tail = yield createSavedPings(3, ONE_MINUTE_MS);
  let pings = head.concat(tail);

  yield startTelemetry();
  let gen = TelemetryFile.popPendingPings();

  for (let item of gen) {
    for (let p of tail) {
      do_check_neq(p.slug, item.slug);
    }
  }

  assertNotSaved(tail);
  yield resetTelemetry();
  yield clearPings(pings);
});






add_task(function* test_overdue_pings_trigger_send() {
  let recentPings = yield createSavedPings(RECENT_PINGS);
  let expiredPings = yield createSavedPings(EXPIRED_PINGS, EXPIRED_PING_FILE_AGE);
  let overduePings = yield createSavedPings(OVERDUE_PINGS, OVERDUE_PING_FILE_AGE);

  yield startTelemetry();
  assertReceivedPings(TOTAL_EXPECTED_PINGS);

  yield assertNotSaved(recentPings);
  yield assertNotSaved(expiredPings);
  yield assertNotSaved(overduePings);
  yield resetTelemetry();
});

add_task(function* teardown() {
  yield stopHttpServer();
});
