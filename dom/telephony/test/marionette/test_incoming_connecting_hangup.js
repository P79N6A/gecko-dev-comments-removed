


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

let inNumber = "5555551111";
let incomingCall;

function simulateIncoming() {
  log("Simulating an incoming call.");

  telephony.onincoming = function onincoming(event) {
    log("Received 'incoming' call event.");
    incomingCall = event.call;
    ok(incomingCall);
    is(incomingCall.number, inNumber);
    is(incomingCall.state, "incoming");

    is(telephony.calls.length, 1);
    is(telephony.calls[0], incomingCall);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "inbound from " + inNumber + " : incoming");
      is(result[1], "OK");
      answerIncoming();
    });
  };
  emulator.run("gsm call " + inNumber);
}

function answerIncoming() {
  log("Answering the incoming call.");

  incomingCall.onconnecting = function onconnecting(event) {
    log("Received 'connecting' call event.");
    is(incomingCall, event.call);
    is(incomingCall.state, "connecting");
    

    
    
    
    
    
    
    

    
    log("==> Waiting one second, remove wait once bug 784429 is fixed <==");
    setTimeout(hangUp, 1000);
  };

  incomingCall.onconnected = function onconnected(event) {
    log("Received 'connected' call event.");
  };
  incomingCall.answer();
}

function hangUp() {
  log("Hanging up the incoming call before fully connected.");

  let gotDisconnecting = false;
  incomingCall.ondisconnecting = function ondisconnecting(event) {
    log("Received 'disconnecting' call event.");
    is(incomingCall, event.call);
    is(incomingCall.state, "disconnecting");
    gotDisconnecting = true;
  };

  incomingCall.ondisconnected = function ondisconnected(event) {
    log("Received 'disconnected' call event.");
    is(incomingCall, event.call);
    is(incomingCall.state, "disconnected");
    ok(gotDisconnecting);

    is(telephony.active, null);
    is(telephony.calls.length, 0);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "OK");
      cleanUp();
    });
  };
  incomingCall.hangUp();
}

function cleanUp() {
  telephony.onincoming = null;
  finish();
}

startTest(function() {
  simulateIncoming();
});
