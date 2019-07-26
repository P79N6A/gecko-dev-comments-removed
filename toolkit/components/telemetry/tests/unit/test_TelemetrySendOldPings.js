











Components.utils.import("resource://gre/modules/Services.jsm");




Services.scriptloader.loadSubScript("resource://gre/components/TelemetryPing.js");

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/TelemetryFile.jsm");




const EXPIRED_PING_FILE_AGE = TelemetryFile.MAX_PING_FILE_AGE + 1;
const OVERDUE_PING_FILE_AGE = TelemetryFile.OVERDUE_PING_FILE_AGE + 1;

const PING_SAVE_FOLDER = "saved-telemetry-pings";
const PING_TIMEOUT_LENGTH = 1500;
const EXPIRED_PINGS = 5;
const OVERDUE_PINGS = 6;
const RECENT_PINGS = 4;




const TOTAL_EXPECTED_PINGS = OVERDUE_PINGS + RECENT_PINGS + 1;

let gHttpServer = new HttpServer();
let gCreatedPings = 0;
let gSeenPings = 0;












function createSavedPings(aNum, aAge) {
  
  
  
  let pingService = new TelemetryPing();
  let pings = [];
  let age = Date.now() - aAge;
  for (let i = 0; i < aNum; ++i) {
    let payload = pingService.getPayload();
    let ping = { slug: "test-ping-" + gCreatedPings, reason: "test", payload: payload };
    TelemetryFile.savePing(ping);
    if (aAge) {
      
      
      let file = getSaveFileForPing(ping);
      file.lastModifiedTime = age;
    }
    gCreatedPings++;
    pings.push(ping);
  }
  return pings;
}







function clearPings(aPings) {
  for (let ping of aPings) {
    let file = getSaveFileForPing(ping);
    if (file.exists()) {
      file.remove(false);
    }
  }
}







function getSaveFileForPing(aPing) {
  let file = Services.dirsvc.get("ProfD", Ci.nsILocalFile).clone();
  file.append(PING_SAVE_FOLDER);
  file.append(aPing.slug);
  return file;
}







function assertReceivedNoPings() {
  let deferred = Promise.defer();

  do_timeout(PING_TIMEOUT_LENGTH, function() {
    if (gSeenPings > 0) {
      deferred.reject();
    } else {
      deferred.resolve();
    }
  });

  return deferred.promise;
}








function assertReceivedPings(aExpectedNum) {
  let deferred = Promise.defer();

  do_timeout(PING_TIMEOUT_LENGTH, function() {
    if (gSeenPings == aExpectedNum) {
      deferred.resolve();
    } else {
      deferred.reject("Saw " + gSeenPings + " TelemetryPings, " +
                      "but expected " + aExpectedNum);
    }
  })

  return deferred.promise;
}






function assertNotSaved(aPings) {
  let saved = 0;
  for (let ping of aPings) {
    let file = getSaveFileForPing(ping);
    if (file.exists()) {
      saved++;
    }
  }
  if (saved > 0) {
    do_throw("Found " + saved + " unexpected saved pings.");
  }
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





function resetTelemetry(aPingService) {
  aPingService.uninstall();
  
  
  let gen = TelemetryFile.popPendingPings();
  for (let item of gen) {};
}





function startTelemetry() {
  let service = new TelemetryPing();
  service.setup(true);
  return service;
}

function run_test() {
  gHttpServer.registerPrefixHandler("/submit/telemetry/", pingHandler);
  gHttpServer.start(-1);
  do_get_profile();
  Services.prefs.setBoolPref(PREF_ENABLED, true);
  Services.prefs.setCharPref(PREF_SERVER,
                             "http://localhost:" + gHttpServer.identity.primaryPort);
  run_next_test();
}





add_task(function test_expired_pings_are_deleted() {
  let expiredPings = createSavedPings(EXPIRED_PINGS, EXPIRED_PING_FILE_AGE);
  let pingService = startTelemetry();
  yield assertReceivedNoPings();
  assertNotSaved(expiredPings);
  resetTelemetry(pingService);
})




add_task(function test_recent_pings_not_sent() {
  let recentPings = createSavedPings(RECENT_PINGS);
  let pingService = startTelemetry();
  yield assertReceivedNoPings();
  resetTelemetry(pingService);
  clearPings(recentPings);
});






add_task(function test_overdue_pings_trigger_send() {
  let recentPings = createSavedPings(RECENT_PINGS);
  let expiredPings = createSavedPings(EXPIRED_PINGS, EXPIRED_PING_FILE_AGE);
  let overduePings = createSavedPings(OVERDUE_PINGS, OVERDUE_PING_FILE_AGE);

  let pingService = startTelemetry();
  yield assertReceivedPings(TOTAL_EXPECTED_PINGS);

  assertNotSaved(recentPings);
  assertNotSaved(expiredPings);
  assertNotSaved(overduePings);
  resetTelemetry(pingService);
})

add_task(function teardown() {
  yield stopHttpServer();
});