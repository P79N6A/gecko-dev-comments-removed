


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function setRadioEnabled(enabled) {
  log("Set radio enabled: " + enabled + ".");

  let desiredRadioState = enabled ? 'enabled' : 'disabled';
  let deferred = Promise.defer();
  let connection = navigator.mozMobileConnections[0];

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

function dial(number) {
  log("Make an outgoing call.");

  let deferred = Promise.defer();
  telephony.dial(number).then(call => {
    ok(call);
    is(call.id.number, number);
    is(call.state, "dialing");

    call.onalerting = function(event) {
      log("Received 'onalerting' call event.");
      call.onalerting = null;
      is(call, event.call);
      is(call.state, "alerting");
      deferred.resolve(call);
    };
  });

  return deferred.promise;
}

function remoteAnswer(call) {
  log("Remote answering the call.");

  let deferred = Promise.defer();

  call.onconnected = function(event) {
    log("Received 'connected' call event.");
    call.onconnected = null;
    is(call, event.call);
    is(call.state, "connected");
    deferred.resolve(call);
  };
  emulator.runCmdWithCallback("gsm accept " + call.id.number);

  return deferred.promise;
}

function disableRadioAndWaitForCallEvent(call) {
  log("Disable radio and wait for call event.");

  let deferred = Promise.defer();

  call.ondisconnected = function(event) {
    log("Received 'disconnected' call event.");
    call.ondisconnected = null;
    is(call, event.call);
    is(call.state, "disconnected");
    deferred.resolve();
  };
  navigator.mozMobileConnections[0].setRadioEnabled(false);

  return deferred.promise;
}




function testOutgoingCallRadioOff() {
  log("= testOutgoingCallRadioOff =");
  let number = "0912345000";

  return Promise.resolve()
    .then(() => dial(number))
    .then(call => remoteAnswer(call))
    .then(call => disableRadioAndWaitForCallEvent(call))
    .then(() => setRadioEnabled(true));
}


startTestWithPermissions(['mobileconnection'], function() {
  testOutgoingCallRadioOff()
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
