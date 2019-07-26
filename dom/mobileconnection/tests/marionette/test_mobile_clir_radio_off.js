


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testSetClirOnRadioOff(aMode) {
  log("testSetClirOnRadioOff (set to mode: " + aMode + ")");
  return Promise.resolve()
    .then(() => setClir(aMode))
    .then(() => {
      ok(false, "shouldn't resolve");
    }, (aError) => {
      is(aError.name, "RadioNotAvailable");
    });
}

function testGetClirOnRadioOff() {
  log("testGetClirOnRadioOff");
  return Promise.resolve()
    .then(() => getClir())
    .then(() => {
      ok(false, "shouldn't resolve");
    }, (aError) => {
      is(aError.name, "RadioNotAvailable");
    });
}

startTestCommon(function() {
  return setRadioEnabledAndWait(false)
    .then(() => testSetClirOnRadioOff(0))
    .then(() => testGetClirOnRadioOff())
    
    .then(() => setRadioEnabledAndWait(true),
          () => setRadioEnabledAndWait(true));
});
