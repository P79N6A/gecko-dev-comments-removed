


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";





function testSetVoicePrivacyMode(aEnabled) {
  log("Test setting voice privacy mode to " + aEnabled);

  return setVoicePrivacyMode(aEnabled)
    .then(function resolve() {
      ok(false, "setVoicePrivacyMode should not success");
    }, function reject(aError) {
      is(aError.name, "RequestNotSupported", "failed to setVoicePrivacyMode");
    });
}





function testGetVoicePrivacyMode() {
  log("Test getting voice privacy mode");

  return getVoicePrivacyMode()
    .then(function resolve() {
      ok(false, "getVoicePrivacyMode should not success");
    }, function reject(aError) {
      is(aError.name, "RequestNotSupported", "failed to getVoicePrivacyMode");
    });
}


startTestCommon(function() {
  return Promise.resolve()
    .then(() => testSetVoicePrivacyMode(true))
    .then(() => testGetVoicePrivacyMode());
});
