


const NOTIFICATION = "sessionstore-browser-state-restored";

function test() {
  waitForExplicitFinish();

  function observe(subject, topic, data) {
    if (NOTIFICATION == topic) {
      finish();
      ok(true, "TOPIC received");
    }
  }

  Services.obs.addObserver(observe, NOTIFICATION, false);
  registerCleanupFunction(function () {
    Services.obs.removeObserver(observe, NOTIFICATION, false);
  });

  ss.setBrowserState(JSON.stringify({ windows: [] }));
}
