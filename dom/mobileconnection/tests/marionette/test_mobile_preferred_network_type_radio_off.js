


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testSetPreferredNetworkTypeOnRadioOff(aType) {
  log("Test setPreferredNetworkType when radio is off");
  return setPreferredNetworkType(aType)
    .then(function resolve() {
      ok(false, "setPreferredNetworkType shouldn't success");
    }, function reject(aError) {
      is(aError.name, "RadioNotAvailable", "failed to setPreferredNetworkType");
    });
}

function testGetPreferredNetworkTypeOnRadioOff() {
  log("Test getPreferredNetworkType when radio is off");
  return getPreferredNetworkType(aType)
    .then(function resolve() {
      ok(false, "getPreferredNetworkType shouldn't success");
    }, function reject(aError) {
      is(aError.name, "RadioNotAvailable", "failed to getPreferredNetworkType");
    });
}


startTestCommon(function() {
  return setRadioEnabledAndWait(false)
    .then(() => testSetPreferredNetworkTypeOnRadioOff("gsm"))
    .then(() => testGetPreferredNetworkTypeOnRadioOff())
    
    .then(() => setRadioEnabledAndWait(true),
          () => setRadioEnabledAndWait(true));
});
