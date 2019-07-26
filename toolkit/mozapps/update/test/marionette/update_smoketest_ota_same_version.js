


function testSameVersion() {
  let mozSettings = window.navigator.mozSettings;
  let forceSent = false;

  mozSettings.addObserver("gecko.updateStatus", function statusObserver(setting) {
    if (!forceSent) {
      return;
    }

    mozSettings.removeObserver("gecko.updateStatus", statusObserver);
    is(setting.settingValue, "already-latest-version");
    cleanUp();
  });

  sendContentEvent("force-update-check");
  forceSent = true;
}


function preUpdate() {
  testSameVersion();
}
