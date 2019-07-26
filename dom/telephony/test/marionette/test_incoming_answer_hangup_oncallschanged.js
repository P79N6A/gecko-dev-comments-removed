


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

SpecialPowers.addPermission("telephony", true, document);

let telephony = window.navigator.mozTelephony;
let number = "5555552368";
let incoming;

function getExistingCalls() {
  emulator.run("gsm list", function(result) {
    log("Initial call list: " + result);
    if (result[0] == "OK") {
      verifyInitialState(false);
    } else {
      cancelExistingCalls(result);
    }
  });
}

function cancelExistingCalls(callList) {
  if (callList.length && callList[0] != "OK") {
    
    nextCall = callList.shift().split(/\s+/)[2].trim();
    log("Cancelling existing call '" + nextCall +"'");
    emulator.run("gsm cancel " + nextCall, function(result) {
      if (result[0] == "OK") {
        cancelExistingCalls(callList);
      } else {
        log("Failed to cancel existing call");
        cleanUp();
      }
    });
  } else {
    
    waitFor(verifyInitialState, function() {
      return (telephony.calls.length === 0);
    });
  }
}

function verifyInitialState(confirmNoCalls = true) {
  log("Verifying initial state.");
  ok(telephony);
  is(telephony.active, null);
  ok(telephony.calls);
  is(telephony.calls.length, 0);
  if (confirmNoCalls) {
    emulator.run("gsm list", function(result) {
    log("Initial call list: " + result);
      is(result[0], "OK");
      if (result[0] == "OK") {
        simulateIncoming();
      } else {
        log("Call exists from a previous test, failing out.");
        cleanUp();
      }
    });
  } else {
    simulateIncoming();
  }
}

function simulateIncoming() {
  log("Simulating an incoming call.");

  telephony.oncallschanged = function oncallschanged(event) {
    log("Received 'callschanged' event.");

    if (!event.call) {
      log("Notifying calls array is loaded. No call information accompanies.");
      return;
    }

    telephony.oncallschanged = null;

    incoming = event.call;
    ok(incoming);
    is(incoming.number, number);
    is(incoming.state, "incoming");

    is(telephony.calls.length, 1);
    is(telephony.calls[0], incoming);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "inbound from " + number + " : incoming");
      is(result[1], "OK");
      answer();
    });
  };

  emulator.run("gsm call " + number);
}

function answer() {
  log("Answering the incoming call.");

  let gotConnecting = false;
  incoming.onconnecting = function onconnecting(event) {
    log("Received 'connecting' call event.");
    is(incoming, event.call);
    is(incoming.state, "connecting");

    
    isnot(incoming, telephony.active);
    gotConnecting = true;
  };

  incoming.onconnected = function onconnected(event) {
    log("Received 'connected' call event.");
    is(incoming, event.call);
    is(incoming.state, "connected");
    ok(gotConnecting);

    is(incoming, telephony.active);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "inbound from " + number + " : active");
      is(result[1], "OK");
      hangUp();
    });
  };
  incoming.answer();
}

function hangUp() {
  log("Hanging up the incoming call.");

  
  
  let gotDisconnecting = false;
  let gotCallschanged = false;

  incoming.ondisconnecting = function ondisconnecting(event) {
    log("Received 'disconnecting' call event.");
    is(incoming, event.call);
    is(incoming.state, "disconnecting");
    gotDisconnecting = true;
  };

  telephony.oncallschanged = function oncallschanged(event) {
    log("Received 'callschanged' event.");

    if (!event.call) {
      log("Notifying calls array is loaded. No call information accompanies.");
      return;
    }

    is(incoming, event.call);
    is(incoming.state, "disconnected");
    is(telephony.active, null);
    is(telephony.calls.length, 0);
    gotCallschanged = true;
  };

  incoming.ondisconnected = function ondisconnected(event) {
    log("Received 'disconnected' call event.");
    is(incoming, event.call);
    is(incoming.state, "disconnected");
    ok(gotDisconnecting);
    ok(gotCallschanged);

    is(telephony.active, null);
    is(telephony.calls.length, 0);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "OK");
      cleanUp();
    });
  };

  incoming.hangUp();
}

function cleanUp() {
  telephony.oncallschanged = null;
  SpecialPowers.removePermission("telephony", document);
  finish();
}

startTest(function() {
  getExistingCalls();
});
