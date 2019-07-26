


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

SpecialPowers.addPermission("telephony", true, document);

let telephony = window.navigator.mozTelephony;
let conference = telephony.conferenceGroup;
let outNumber = "5555551111";
let inNumber = "5555552222";
let inNumber2 = "5555553333";
let outgoingCall;
let incomingCall;
let incomingCall2;
let gotOriginalConnected = false;


function receivedPending(received, pending, nextTest) {
  let index = pending.indexOf(received);
  if (index != -1) {
    pending.splice(index, 1);
  }
  if (pending.length === 0) {
    nextTest();
  }
}

function checkState(telephonyActive, telephonyCalls, conferenceState,
                    conferenceCalls) {
  is(telephony.active, telephonyActive);

  is(telephony.calls.length, telephonyCalls.length);
  for (let i = 0; i < telephonyCalls.length; i++) {
    is(telephony.calls[i], telephonyCalls[i]);
  }

  is(conference.state, conferenceState);
  is(conference.calls.length, conferenceCalls.length);
  for (let i = 0; i < conferenceCalls.length; i++) {
    is(conference.calls[i], conferenceCalls[i]);
  }
}

function verifyInitialState() {
  log("Verifying initial state.");
  ok(telephony);
  ok(conference);

  emulator.run("gsm clear", function(result) {
    log("Clear up calls from a previous test if any.");
    is(result[0], "OK");

    
    waitFor(function next() {
      checkState(null, [], '', []);
      dial();
    }, function isDone() {
      return (telephony.calls.length === 0);
    });
  });
}

function dial() {
  log("Making an outgoing call.");
  outgoingCall = telephony.dial(outNumber);
  ok(outgoingCall);
  is(outgoingCall.number, outNumber);
  is(outgoingCall.state, "dialing");

  checkState(outgoingCall, [outgoingCall], '', []);

  outgoingCall.onalerting = function(event) {
    log("Received 'onalerting' call event.");

    outgoingCall.onalerting = null;

    is(outgoingCall, event.call);
    is(outgoingCall.state, "alerting");

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : ringing");
      is(result[1], "OK");

      answer();
    });
  };
}

function answer() {
  log("Answering the outgoing call.");

  
  outgoingCall.onconnected = function(event) {
    log("Received 'connected' call event for the original outgoing call.");

    outgoingCall.onconnected = null;

    is(outgoingCall, event.call);
    is(outgoingCall.state, "connected");
    is(outgoingCall, telephony.active);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "OK");

      if(!gotOriginalConnected){
        gotOriginalConnected = true;
        simulateIncoming();
      } else {
        
        ok(false,
           "Received 'connected' event for original call multiple times");
      }
    });
  };
  emulator.run("gsm accept " + outNumber);
}


function simulateIncoming() {
  log("Simulating an incoming call (with one call already connected).");

  telephony.onincoming = function(event) {
    log("Received 'incoming' call event.");

    telephony.onincoming = null;

    incomingCall = event.call;
    ok(incomingCall);
    is(incomingCall.number, inNumber);
    is(incomingCall.state, "incoming");

    
    checkState(outgoingCall, [outgoingCall, incomingCall], '', []);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
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
  incomingCall.onconnecting = function(event) {
    log("Received 'connecting' call event for incoming/2nd call.");

    incomingCall.onconnecting = null;

    is(incomingCall, event.call);
    is(incomingCall.state, "connecting");
    gotConnecting = true;
  };

  incomingCall.onconnected = function(event) {
    log("Received 'connected' call event for incoming/2nd call.");

    incomingCall.onconnected = null;

    is(incomingCall, event.call);
    is(incomingCall.state, "connected");
    ok(gotConnecting);

    is(outgoingCall.state, "held");
    checkState(incomingCall, [outgoingCall, incomingCall], '', []);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "OK");

      conferenceAddTwoCalls();
    });
  };
  incomingCall.answer();
}


function conferenceAddTwoCalls() {
  log("Creating a conference call.");

  let pending = ["conference.oncallschanged", "conference.onconnected",
                 "outgoingCall.onstatechange", "outgoingCall.ongroupchange",
                 "incomingCall.onstatechange", "incomingCall.ongroupchange"];
  let nextTest = conferenceHold;

  
  
  let expected = [outgoingCall, incomingCall];
  conference.oncallschanged = function(event) {
    log("Received 'callschanged' event for the conference call.");

    let index = expected.indexOf(event.call);
    ok(index != -1);
    expected.splice(index, 1);
    is(conference.calls[conference.calls.length - 1].number, event.call.number);

    if (expected.length === 0) {
      conference.oncallschanged = null;
      receivedPending("conference.oncallschanged", pending, nextTest);
    }
  };

  conference.onconnected = function(event) {
    log("Received 'connected' event for the conference call.");
    conference.onconnected = null;

    ok(!conference.oncallschanged);

    checkState(conference, [], 'connected', [outgoingCall, incomingCall]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "OK");

      receivedPending("conference.onconnected", pending, nextTest);
    });
  };

  outgoingCall.ongroupchange = function(event) {
    log("Received 'groupchange' event for the outgoing call.");
    outgoingCall.ongroupchange = null;

    ok(outgoingCall.group);
    is(outgoingCall.group, conference);

    receivedPending("outgoingCall.ongroupchange", pending, nextTest);
  };

  outgoingCall.onstatechange = function(event) {
    log("Received 'statechange' event for the outgoing call.");
    outgoingCall.onstatechange = null;

    ok(!outgoingCall.ongroupchange);
    is(outgoingCall.state, conference.state);

    receivedPending("outgoingCall.onstatechange", pending, nextTest);
  };

  incomingCall.ongroupchange = function(event) {
    log("Received 'groupchange' event for the incoming call.");
    incomingCall.ongroupchange = null;

    ok(incomingCall.group);
    is(incomingCall.group, conference);

    receivedPending("incomingCall.ongroupchange", pending, nextTest);
  };

  incomingCall.onstatechange = function(event) {
    log("Received 'statechange' event for the incoming call.");
    incomingCall.onstatechange = null;

    ok(!incomingCall.ongroupchange);
    is(incomingCall.state, conference.state);

    receivedPending("incomingCall.onstatechange", pending, nextTest);
  };

  conference.add(outgoingCall, incomingCall);
}

function conferenceHold() {
  log("Holding the conference call.");

  let pending = ["conference.onholding", "conference.onheld",
                 "outgoingCall.onholding", "outgoingCall.onheld",
                 "incomingCall.onholding", "incomingCall.onheld"];
  let nextTest = conferenceResume;

  conference.onholding = function(event) {
    log("Received 'holding' event for the conference call.");
    conference.onholding = null;

    is(conference.state, 'holding');

    receivedPending("conference.onholding", pending, nextTest);
  };

  conference.onheld = function(event) {
    log("Received 'held' event for the conference call.");
    conference.onheld = null;

    ok(!conference.onholding);
    checkState(null, [], 'held', [outgoingCall, incomingCall]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "inbound from " + inNumber + " : held");
      is(result[2], "OK");

      receivedPending("conference.onheld", pending, nextTest);
    });
  };

  outgoingCall.onholding = function(event) {
    log("Received 'holding' event for the outgoing call in conference.");
    outgoingCall.onholding = null;

    is(outgoingCall.state, 'holding');

    receivedPending("outgoingCall.onholding", pending, nextTest);
  };

  outgoingCall.onheld = function(event) {
    log("Received 'held' event for the outgoing call in conference.");
    outgoingCall.onheld = null;

    ok(!outgoingCall.onholding);
    is(outgoingCall.state, 'held');

    receivedPending("outgoingCall.onheld", pending, nextTest);
  };

  incomingCall.onholding = function(event) {
    log("Received 'holding' event for the incoming call in conference.");
    incomingCall.onholding = null;

    is(incomingCall.state, 'holding');

    receivedPending("incomingCall.onholding", pending, nextTest);
  };

  incomingCall.onheld = function(event) {
    log("Received 'held' event for the incoming call in conference.");
    incomingCall.onheld = null;

    ok(!incomingCall.onholding);
    is(incomingCall.state, 'held');

    receivedPending("incomingCall.onheld", pending, nextTest);
  };

  conference.hold();
}

function conferenceResume() {
  log("Resuming the held conference call.");

  let pending = ["conference.onresuming", "conference.onconnected",
                 "outgoingCall.onresuming", "outgoingCall.onconnected",
                 "incomingCall.onresuming", "incomingCall.onconnected"];
  let nextTest = simulate2ndIncoming;

  conference.onresuming = function(event) {
    log("Received 'resuming' event for the conference call.");
    conference.onresuming = null;

    is(conference.state, 'resuming');

    receivedPending("conference.onresuming", pending, nextTest);
  };

  conference.onconnected = function(event) {
    log("Received 'connected' event for the conference call.");
    conference.onconnected = null;

    ok(!conference.onresuming);
    checkState(conference, [], 'connected', [outgoingCall, incomingCall]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "OK");

      receivedPending("conference.onconnected", pending, nextTest);
    });
  };

  outgoingCall.onresuming = function(event) {
    log("Received 'resuming' event for the outgoing call in conference.");
    outgoingCall.onresuming = null;

    is(outgoingCall.state, 'resuming');

    receivedPending("outgoingCall.onresuming", pending, nextTest);
  };

  outgoingCall.onconnected = function(event) {
    log("Received 'connected' event for the outgoing call in conference.");
    outgoingCall.onconnected = null;

    ok(!outgoingCall.onresuming);
    is(outgoingCall.state, 'connected');

    receivedPending("outgoingCall.onconnected", pending, nextTest);
  };

  incomingCall.onresuming = function(event) {
    log("Received 'resuming' event for the incoming call in conference.");
    incomingCall.onresuming = null;

    is(incomingCall.state, 'resuming');

    receivedPending("incomingCall.onresuming", pending, nextTest);
  };

  incomingCall.onconnected = function(event) {
    log("Received 'connected' event for the incoming call in conference.");
    incomingCall.onconnected = null;

    ok(!incomingCall.onresuming);
    is(incomingCall.state, 'connected');

    receivedPending("incomingCall.onconnected", pending, nextTest);
  };

  conference.resume();
}

function simulate2ndIncoming() {
  log("Simulating 2nd incoming call (with one conference call already).");

  telephony.onincoming = function(event) {
    log("Received 'incoming' call event.");

    telephony.onincoming = null;

    incomingCall2 = event.call;
    ok(incomingCall2);
    is(incomingCall2.number, inNumber2);
    is(incomingCall2.state, "incoming");

    checkState(conference, [incomingCall2], 'connected', [outgoingCall, incomingCall]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "inbound from " + inNumber2 + " : incoming");
      is(result[3], "OK");

      answer2ndIncoming();
    });
  };
  emulator.run("gsm call " + inNumber2);
}

function answer2ndIncoming() {
  log("Answering the 2nd incoming call when there's a connected conference call.");

  let gotConnecting = false;
  incomingCall2.onconnecting = function(event) {
    log("Received 'connecting' event for the 2nd incoming call.");
    incomingCall2.onconnecting = null;

    is(incomingCall2, event.call);
    is(incomingCall2.state, "connecting");
    gotConnecting = true;
  };

  incomingCall2.onconnected = function(event) {
    incomingCall2.onconnected = null;

    is(incomingCall2, event.call);
    is(incomingCall2.state, "connected");
    ok(gotConnecting);

    is(incomingCall2, telephony.active);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : held");
      is(result[1], "inbound from " + inNumber + " : held");
      is(result[2], "inbound from " + inNumber2 + " : active");
      is(result[3], "OK");

      conferenceAddOneCall();
    });
  };
  incomingCall2.answer();
}

function conferenceAddOneCall() {
  log("Adding one more call to the conference call.");

  let callToAdd = incomingCall2;
  let pending = ["conference.oncallschanged", "conference.onconnected",
                 "callToAdd.ongroupchange", "callToAdd.onconnected"];
  let nextTest = conferenceRemove;

  ok(!callToAdd.group);

  conference.oncallschanged = function(event) {
    log("Received 'callschanged' event for the conference call.");
    conference.oncallschanged = null;

    ok(event.call, callToAdd);
    is(conference.calls.length, 3);
    is(conference.calls[2].number, event.call.number);

    receivedPending("conference.oncallschanged", pending, nextTest);
  };

  conference.onconnected = function(event) {
    log("Received 'connected' event for the conference call.");
    conference.oncallschanged = null;

    ok(!conference.oncallschanged);

    checkState(conference, [], 'connected',
               [outgoingCall, incomingCall, incomingCall2]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "inbound from " + inNumber + " : active");
      is(result[2], "inbound from " + inNumber2 + " : active");
      is(result[3], "OK");

      receivedPending("conference.onconnected", pending, nextTest);
    });
  };

  callToAdd.ongroupchange = function(event) {
    log("Received 'groupchange' event for the call to add.");
    callToAdd.ongroupchange = null;

    is(callToAdd.group, conference);
    receivedPending("callToAdd.ongroupchange", pending, nextTest);
  };

  callToAdd.onconnected = function(event) {
    log("Received 'connected' event for the call to add.");
    callToAdd.onconnected = null;

    ok(!callToAdd.ongroupchange);
    is(callToAdd.state, 'connected');

    receivedPending("callToAdd.onconnected", pending, nextTest);
  };
  conference.add(callToAdd);
}



function conferenceRemove() {
  log("Removing a participant from the conference call.");

  is(conference.state, 'connected');

  let callToRemove = conference.calls[0];
  let pending = ["callToRemove.ongroupchange", "telephony.oncallschanged",
                 "conference.oncallschanged", "conference.onstatechange"];
  let nextTest = emptyConference;

  callToRemove.ongroupchange = function(event) {
    log("Received 'groupchange' event for the call to remove.");
    callToRemove.ongroupchange = null;

    ok(!callToRemove.group);
    is(callToRemove.state, 'connected');

    receivedPending("callToRemove.ongroupchange", pending, nextTest);
  };

  telephony.oncallschanged = function(event) {
    log("Received 'callschanged' event for telephony.");
    if (event.call) {
      
      
      telephony.oncallschanged = null;

      is(event.call.number, callToRemove.number);
      is(telephony.calls.length, 1);
      is(telephony.calls[0].number, event.call.number);

      receivedPending("telephony.oncallschanged", pending, nextTest);
    }
  };

  conference.oncallschanged = function(event) {
    log("Received 'callschanged' event for the conference.");
    conference.oncallschanged = null;

    is(event.call.number, callToRemove.number);
    is(conference.calls.length, 2);

    receivedPending("conference.oncallschanged", pending, nextTest);
  };

  conference.onstatechange = function(event) {
    log("Received 'statechange' event for the conference.");
    conference.onstatechange = null;

    ok(!conference.oncallschanged);

    checkState(callToRemove, [callToRemove], 'held',
               [incomingCall, incomingCall2]);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "outbound to  " + outNumber + " : active");
      is(result[1], "inbound from " + inNumber + " : held");
      is(result[2], "inbound from " + inNumber2 + " : held");
      is(result[3], "OK");

      receivedPending("conference.onstatechange", pending, nextTest);
    });
  };

  conference.remove(callToRemove);
}




function emptyConference() {
  log("Release one call in conference.");

  outgoingCall.ondisconnected = function(event) {
    log("Received 'disconnected' event for the outgoing call.");
    outgoingCall.ondisconnected = null;

    checkState(null, [], 'held',
               [incomingCall, incomingCall2]);

    
    
    emulator.run("gsm cancel " + inNumber);
  };

  let pending = ["conference.oncallschanged", "conference.onstatechange",
                 "incomingCall2.ongroupchange"];
  let nextTest = hangUpLastCall;

  incomingCall2.ongroupchange = function(event) {
    log("Received 'groupchange' event for the outgoing call.");
    incomingCall2.ongroupchange = null;

    ok(!incomingCall2.group);
    is(incomingCall2.state, 'held');

    receivedPending("incomingCall2.ongroupchange", pending, nextTest);
  };

  
  
  let expected = [incomingCall, incomingCall2];
  conference.oncallschanged = function(event) {
    log("Received 'callschanged' event for the conference call.");

    let index = expected.indexOf(event.call);
    ok(index != -1);
    expected.splice(index, 1);

    if (expected.length === 0) {
      conference.oncallschanged = null;
      is(conference.calls.length, 0);
      receivedPending("conference.oncallschanged", pending, nextTest);
    }
  };

  conference.onstatechange = function(event) {
    log("Received 'statechange' event for the conference call.");
    conference.onstatechange = null;

    ok(!conference.oncallschanged);
    checkState(null, [incomingCall2], '', []);

    receivedPending("conference.onstatechange", pending, nextTest);
  };

  emulator.run("gsm cancel " + outNumber);
}

function hangUpLastCall() {
  log("Going to leave the test. Hanging up the last call.");

  incomingCall2.ondisconnected = function(event) {
    incomingCall2.ondisconnected = null;

    checkState(null, [], '', []);

    emulator.run("gsm list", function(result) {
      log("Call list is now: " + result);
      is(result[0], "OK");

      cleanUp();
    });
  };

  emulator.run("gsm cancel " + inNumber2);
}

function cleanUp() {
  SpecialPowers.removePermission("telephony", document);
  finish();
}


startTest(function() {
  verifyInitialState();
});
