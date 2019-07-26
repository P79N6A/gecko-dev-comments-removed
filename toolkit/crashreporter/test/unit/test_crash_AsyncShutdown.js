


function setup_crash() {
  Components.utils.import("resource://gre/modules/AsyncShutdown.jsm", this);
  Components.utils.import("resource://gre/modules/Services.jsm", this);
  Components.utils.import("resource://gre/modules/Promise.jsm", this);

  Services.prefs.setBoolPref("toolkit.asyncshutdown.testing", true);
  Services.prefs.setIntPref("toolkit.asyncshutdown.crash_timeout", 10);

  let TOPIC = "testing-async-shutdown-crash";
  let phase = AsyncShutdown._getPhase(TOPIC);
  phase.addBlocker("A blocker that is never satisfied", function() {
    dump("Installing blocker\n");
    let deferred = Promise.defer();
    return deferred.promise;
  });

  Services.obs.notifyObservers(null, TOPIC, null);
  dump("Waiting for crash\n");
}

function after_crash(mdump, extra) {
  do_print("after crash: " + extra.AsyncShutdownTimeout);
  let info = JSON.parse(extra.AsyncShutdownTimeout);
  do_check_eq(info.phase, "testing-async-shutdown-crash");
  do_print("Condition: " + JSON.stringify(info.conditions));
  do_check_true(JSON.stringify(info.conditions).indexOf("A blocker that is never satisfied") != -1);
}

function run_test() {
  do_crash(setup_crash, after_crash);
}
