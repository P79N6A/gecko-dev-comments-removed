


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const number = "0912345678";

function setRadioEnabled(enabled) {
  log("Set radio enabled: " + enabled + ".");

  let desiredRadioState = enabled ? 'enabled' : 'disabled';
  let deferred = Promise.defer();
  let connection = navigator.mozMobileConnections[0];
  ok(connection instanceof MozMobileConnection,
     "connection is instanceof " + connection.constructor);

  connection.onradiostatechange = function() {
    let state = connection.radioState;
    log("Received 'radiostatechange' event, radioState: " + state);

    
    if (state === desiredRadioState) {
      connection.onradiostatechange = null;
      deferred.resolve();
    }
  };
  connection.setRadioEnabled(enabled);

  return deferred.promise;
}

startTestWithPermissions(['mobileconnection'], function() {
  let outCall;
  setRadioEnabled(false)
    .then(() => gDial(number))
    .then(null, cause => { is(cause, "RadioNotAvailable"); })
    .then(() => gDialEmergency(number))
    .then(null, cause => { is(cause, "RadioNotAvailable"); })
    .then(() => setRadioEnabled(true))
    .then(() => gDialEmergency(number))
    .then(null, cause => { is(cause, "BadNumberError"); })
    .then(finish);
});
