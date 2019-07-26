


MARIONETTE_TIMEOUT = 60000;

const KEY = "ril.radio.preferredNetworkType";

let gSettingsEnabled = SpecialPowers.getBoolPref("dom.mozSettings.enabled");
if (!gSettingsEnabled) {
  SpecialPowers.setBoolPref("dom.mozSettings.enabled", true);
}
SpecialPowers.addPermission("mobileconnection", true, document);
SpecialPowers.addPermission("settings", true, document);

let settings = window.navigator.mozSettings;

function test_revert_previous_setting_on_invalid_value() {
  log("Testing reverting to previous setting on invalid value received");

  let getLock = settings.createLock();
  let getReq = getLock.get(KEY);
  getReq.addEventListener("success", function onGetSuccess() {
    let originalValue = getReq.result[KEY] || "wcdma/gsm";

    let setDone = false;
    settings.addObserver(KEY, function observer(setting) {
      
      if (setting.settingValue == obj[KEY]) {
        setDone = true;
        return;
      }

      
      if (!setDone) {
        originalValue = setting.settingValue;
        return;
      }

      settings.removeObserver(KEY, observer);
      is(setting.settingValue, originalValue, "Settings reverted");
      window.setTimeout(cleanUp, 0);
    });

    let obj = {};
    obj[KEY] = "AnInvalidValue";
    let setLock = settings.createLock();
    setLock.set(obj);
    setLock.addEventListener("error", function onSetError() {
      ok(false, "cannot set '" + KEY + "'");
    });
  });
  getReq.addEventListener("error", function onGetError() {
    ok(false, "cannot get default value of '" + KEY + "'");
  });
}

function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);
  SpecialPowers.removePermission("settings", document);
  SpecialPowers.clearUserPref("dom.mozSettings.enabled");

  finish();
}

waitFor(test_revert_previous_setting_on_invalid_value, function () {
  return navigator.mozMobileConnection.voice.connected;
});

