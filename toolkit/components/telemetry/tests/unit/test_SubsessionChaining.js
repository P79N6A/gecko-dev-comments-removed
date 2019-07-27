



Cu.import("resource://gre/modules/Preferences.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/TelemetryArchive.jsm", this);
Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

const MS_IN_ONE_HOUR  = 60 * 60 * 1000;
const MS_IN_ONE_DAY   = 24 * MS_IN_ONE_HOUR;

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_ARCHIVE_ENABLED = PREF_BRANCH + "archive.enabled";

const REASON_ABORTED_SESSION = "aborted-session";
const REASON_DAILY = "daily";
const REASON_ENVIRONMENT_CHANGE = "environment-change";
const REASON_SHUTDOWN = "shutdown";

XPCOMUtils.defineLazyGetter(this, "DATAREPORTING_PATH", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "datareporting");
});

let promiseValidateArchivedPings = Task.async(function*(aExpectedReasons) {
  
  
  const SESSION_END_PING_REASONS = new Set([ REASON_ABORTED_SESSION, REASON_SHUTDOWN ]);

  let list = yield TelemetryArchive.promiseArchivedPingList();

  
  list = list.filter(p => p.type == "main");

  Assert.equal(aExpectedReasons.length, list.length, "All the expected pings must be received.");

  let previousPing = yield TelemetryArchive.promiseArchivedPingById(list[0].id);
  Assert.equal(aExpectedReasons.shift(), previousPing.payload.info.reason,
               "Telemetry should only get pings with expected reasons.");
  Assert.equal(previousPing.payload.info.previousSessionId, null,
               "The first session must report a null previous session id.");
  Assert.equal(previousPing.payload.info.previousSubsessionId, null,
               "The first subsession must report a null previous subsession id.");
  Assert.equal(previousPing.payload.info.profileSubsessionCounter, 1,
               "profileSubsessionCounter must be 1 the first time.");
  Assert.equal(previousPing.payload.info.subsessionCounter, 1,
               "subsessionCounter must be 1 the first time.");

  let expectedSubsessionCounter = 1;
  let expectedPreviousSessionId = previousPing.payload.info.sessionId;

  for (let i = 1; i < list.length; i++) {
    let currentPing = yield TelemetryArchive.promiseArchivedPingById(list[i].id);
    let currentInfo = currentPing.payload.info;
    let previousInfo = previousPing.payload.info;
    do_print("Archive entry " + i + " - id: " + currentPing.id + ", reason: " + currentInfo.reason);

    Assert.equal(aExpectedReasons.shift(), currentInfo.reason,
                 "Telemetry should only get pings with expected reasons.");
    Assert.equal(currentInfo.previousSessionId, expectedPreviousSessionId,
                 "Telemetry must correctly chain session identifiers.");
    Assert.equal(currentInfo.previousSubsessionId, previousInfo.subsessionId,
                 "Telemetry must correctly chain subsession identifiers.");
    Assert.equal(currentInfo.profileSubsessionCounter, previousInfo.profileSubsessionCounter + 1,
                 "Telemetry must correctly track the profile subsessions count.");
    Assert.equal(currentInfo.subsessionCounter, expectedSubsessionCounter,
                 "The subsession counter should be monotonically increasing.");

    
    previousPing = currentPing;
    
    
    
    if (SESSION_END_PING_REASONS.has(currentInfo.reason)) {
      expectedSubsessionCounter = 1;
      expectedPreviousSessionId = currentInfo.sessionId;
    } else {
      expectedSubsessionCounter++;
    }
  }
});

function run_test() {
  do_test_pending();

  
  do_get_profile();
  loadAddonManager("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  Preferences.set(PREF_ENABLED, true);

  run_next_test();
}

add_task(function* test_subsessionsChaining() {
  if (gIsAndroid) {
    
    return;
  }

  const PREF_TEST = PREF_BRANCH + "test.pref1";
  const PREFS_TO_WATCH = new Map([
    [PREF_TEST, TelemetryEnvironment.RECORD_PREF_VALUE],
  ]);
  Preferences.reset(PREF_TEST);

  
  
  let now = fakeNow(2009, 9, 18, 0, 0, 0);

  let moveClockForward = (minutes) => {
    now = futureDate(now, minutes * MILLISECONDS_PER_MINUTE);
    fakeNow(now);
  }

  
  let expectedReasons = [];

  
  
  yield TelemetrySession.reset();
  yield TelemetrySession.shutdown();
  expectedReasons.push(REASON_SHUTDOWN);

  
  
  
  moveClockForward(30);
  TelemetrySession.reset();
  yield TelemetrySession.shutdown();
  expectedReasons.push(REASON_SHUTDOWN);

  
  
  
  let schedulerTickCallback = null;
  fakeSchedulerTimer(callback => schedulerTickCallback = callback, () => {});
  yield TelemetrySession.reset();
  moveClockForward(6);
  
  
  
  Assert.ok(!!schedulerTickCallback);
  yield schedulerTickCallback();
  expectedReasons.push(REASON_ABORTED_SESSION);

  
  
  
  moveClockForward(30);
  yield TelemetryController.reset();
  yield TelemetrySession.reset();
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  moveClockForward(30);
  Preferences.set(PREF_TEST, 1);
  expectedReasons.push(REASON_ENVIRONMENT_CHANGE);

  
  
  moveClockForward(30);
  yield TelemetrySession.shutdown();
  expectedReasons.push(REASON_SHUTDOWN);

  
  
  
  moveClockForward(30);
  yield TelemetrySession.reset();

  
  now = fakeNow(futureDate(now, MS_IN_ONE_DAY));
  
  yield schedulerTickCallback();
  expectedReasons.push(REASON_DAILY);

  
  
  
  moveClockForward(30);
  Preferences.set(PREF_TEST, 0);
  expectedReasons.push(REASON_ENVIRONMENT_CHANGE);

  
  moveClockForward(30);
  yield TelemetrySession.shutdown();
  expectedReasons.push(REASON_SHUTDOWN);

  
  yield TelemetrySession.reset();
  TelemetryEnvironment._watchPreferences(PREFS_TO_WATCH);
  moveClockForward(30);
  Preferences.set(PREF_TEST, 1);
  expectedReasons.push(REASON_ENVIRONMENT_CHANGE);

  
  moveClockForward(6);
  
  yield schedulerTickCallback();
  expectedReasons.push(REASON_ABORTED_SESSION);

  
  moveClockForward(30);
  yield TelemetryController.reset();
  yield TelemetrySession.reset();
  
  now = futureDate(now, MS_IN_ONE_DAY);
  fakeNow(now);
  
  yield schedulerTickCallback();
  expectedReasons.push(REASON_DAILY);

  
  moveClockForward(30);
  Preferences.set(PREF_TEST, 0);
  expectedReasons.push(REASON_ENVIRONMENT_CHANGE);

  
  moveClockForward(6);
  
  yield schedulerTickCallback();
  expectedReasons.push(REASON_ABORTED_SESSION);

  
  yield TelemetryController.reset();
  yield TelemetrySession.reset();

  yield promiseValidateArchivedPings(expectedReasons);
});

add_task(function* () {
  yield TelemetrySend.shutdown();
  do_test_finished();
});
