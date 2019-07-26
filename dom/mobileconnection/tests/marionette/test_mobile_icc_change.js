


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


const ICCID = "89014103211118510720";

function setRadioEnabledAndWaitIccChange(aEnabled) {
  let promises = [];
  promises.push(waitForManagerEvent("iccchange"));
  promises.push(setRadioEnabled(aEnabled));

  return Promise.all(promises);
}


startTestCommon(function() {
  log("Test initial iccId");
  is(mobileConnection.iccId, ICCID);

  return setRadioEnabledAndWaitIccChange(false)
    .then(() => {
      is(mobileConnection.iccId, null);
    })

    
    .then(() => setRadioEnabledAndWaitIccChange(true))
    .then(() => {
      is(mobileConnection.iccId, ICCID);
    });
});
