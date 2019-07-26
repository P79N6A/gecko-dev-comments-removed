


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

let outNumber = "5555551111";
let inNumber = "5555552222";
let outgoingCall;
let incomingCall;
let gotOriginalConnected = false;
let gotHeld = false;
let gotConnected = false;

function dial() {
  log("Make an outgoing call.");
  telephony.dial(outNumber).then(call => {
    outgoingCall = call;
    ok(outgoingCall);
    is(outgoingCall.number, outNumber);
    is(outgoingCall.state, "dialing");

    outgoingCall.onalerting = function onalerting(event) {
      log("Received 'alerting' call event.");

      is(outgoingCall, event.call);
      is(outgoingCall.state, "alerting");
      is(outgoingCall, telephony.active);
      is(telephony.calls.length, 1);
      is(telephony.calls[0], outgoingCall);

      emulator.run("gsm list", function(result) {
        log("Call list is now: " + result);
        is(result[0], "outbound to  " + outNumber + " : ringing");
        is(result[1], "OK");
        answer();
      });
    };
  });
}

function answer() {
  log("Answering the outgoing call.");

  
  outgoingCall.onconnected = function onconnectedOut(event) {
    log("Received 'connected' call event for the original outgoing call.");

    is(outgoingCall, event.call);
    is(outgoingCall.state, "connected");
    is(outgoingCall, telephony.active);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "OK");

      if(!gotOriginalConnected){
        gotOriginalConnected = true;
        holdCall();
      } else {
        
        ok(false,
           "Received 'connected' event for original call multiple times.");
      }
    });
  };
  emulator.run("gsm accept " + outNumber);
}

function holdCall() {
  log("Putting the original (outgoing) call on hold.");

  let gotHolding = false;
  outgoingCall.onholding = function onholding(event) {
    log("Received 'holding' call event");
    is(outgoingCall, event.call);
    is(outgoingCall.state, "holding");
    gotHolding = true;
  };

  outgoingCall.onheld = function onheld(event) {
    log("Received 'held' call event");
    is(outgoingCall, event.call);
    is(outgoingCall.state, "held");
    ok(gotHolding);

    is(telephony.active, null);
    is(telephony.calls.length, 1);
    is(telephony.calls[0], outgoingCall);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "OK");
      simulateIncoming();
    });
  };
  outgoingCall.hold();
}


function simulateIncoming() {
  log("Simulating an incoming call (with one call already held).");

  telephony.onincoming = function onincoming(event) {
    log("Received 'incoming' call event.");
    incomingCall = event.call;
    ok(incomingCall);
    is(incomingCall.number, inNumber);
    is(incomingCall.state, "incoming");

    
    is(telephony.calls.length, 2);
    is(telephony.calls[0], outgoingCall);
    is(telephony.calls[1], incomingCall);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "inbound from " + inNumber + " : incoming");
      is(result[2], "OK");
      answerIncoming();
    });
  };
  emulator.run("gsm call " + inNumber);
}


function answerIncoming() {
  log("Answering the incoming call.");

  let gotConnecting = false;
  incomingCall.onconnecting = function onconnectingIn(event) {
    log("Received 'connecting' call event for incoming/2nd call.");
    is(incomingCall, event.call);
    is(incomingCall.state, "connecting");
    gotConnecting = true;
  };

  incomingCall.onconnected = function onconnectedIn(event) {
    log("Received 'connected' call event for incoming/2nd call.");
    is(incomingCall, event.call);
    ok(gotConnecting);

    is(incomingCall, telephony.active);

    
    is(outgoingCall.state, "held");
    is(incomingCall.state, "connected");

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "OK");
      swapCalls();
    });
  };
  incomingCall.answer();
}


function swapCalls() {
  log("Swapping the held and active calls.");

  
  

  incomingCall.onheld = function onheldIn(event) {
    log("Received 'held' call event for incoming call.");
    gotHeld = true;

    if (gotHeld && gotConnected) { verifySwap(); }
  };

  let gotResuming = false;
  outgoingCall.onresuming = function onresuming(event) {
    log("Received 'resuming' call event for outbound call.");
    is(outgoingCall, event.call);
    is(outgoingCall.state, "resuming");
    gotResuming = true;
  };

  outgoingCall.onconnected = function onconnected(event) {
    log("Received 'connected' call event for outbound call.");
    is(outgoingCall, event.call);
    ok(gotResuming);

    is(outgoingCall, telephony.active);
    is(telephony.calls.length, 2);
    is(telephony.calls[0], outgoingCall);
    is(telephony.calls[1], incomingCall);
    gotConnected = true;

    if (gotHeld && gotConnected) { verifySwap(); }
  };
  outgoingCall.resume();
}

function verifySwap() {
  
  is(outgoingCall.state, "connected");
  is(incomingCall.state, "held");

  emulator.run("gsm list", function(result) {
    log("Call list is now: " + result);
    is(result[0], "outbound to  " + outNumber + " : active");
    is(result[1], "inbound from " + inNumber + " : held");

    
    hangUpOutgoing();
  });
}


function hangUpOutgoing() {
  log("Hanging up the original outgoing (now active) call.");

  let gotDisconnecting = false;
  outgoingCall.ondisconnecting = function ondisconnectingOut(event) {
    log("Received 'disconnecting' call event for original outgoing call.");
    is(outgoingCall, event.call);
    is(outgoingCall.state, "disconnecting");
    gotDisconnecting = true;
  };

  outgoingCall.ondisconnected = function ondisconnectedOut(event) {
    log("Received 'disconnected' call event for original outgoing call.");
    is(outgoingCall, event.call);
    is(outgoingCall.state, "disconnected");
    ok(gotDisconnecting);

    
    is(telephony.calls.length, 1);
    is(incomingCall.state, "held");

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "inbound from " + inNumber + " : held");
      is(result[1], "OK");
      hangUpIncoming();
    });
  };
  outgoingCall.hangUp();
}


function hangUpIncoming() {
  log("Hanging up the remaining (now held) call.");

  let gotDisconnecting = false;
  incomingCall.ondisconnecting = function ondisconnectingIn(event) {
    log("Received 'disconnecting' call event for remaining (held) call.");
    is(incomingCall, event.call);
    is(incomingCall.state, "disconnecting");
    gotDisconnecting = true;
  };

  incomingCall.ondisconnected = function ondisconnectedIn(event) {
    log("Received 'disconnected' call event for remaining (incoming) call.");
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
  dial();
});
