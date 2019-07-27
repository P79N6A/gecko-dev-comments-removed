


let Promise = SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;
let telephony;
let conference;

const kPrefRilDebuggingEnabled = "ril.debugging.enabled";




let emulator = (function() {
  let pendingCmdCount = 0;
  let originalRunEmulatorCmd = runEmulatorCmd;

  let pendingShellCount = 0;
  let originalRunEmulatorShell = runEmulatorShell;

  
  runEmulatorCmd = function() {
    throw "Use emulator.runCmdWithCallback(cmd, callback) instead of runEmulatorCmd";
  };

  
  runEmulatorShell = function() {
    throw "Use emulator.runShellCmd(cmd, callback) instead of runEmulatorShell";
  };

  function runCmd(cmd) {
    let deferred = Promise.defer();

    pendingCmdCount++;
    originalRunEmulatorCmd(cmd, function(result) {
      pendingCmdCount--;
      if (result[result.length - 1] === "OK") {
        deferred.resolve(result);
      } else {
        is(result[result.length - 1], "OK", "emulator command result.");
        deferred.reject();
      }
    });

    return deferred.promise;
  }

  function runCmdWithCallback(cmd, callback) {
    runCmd(cmd).then(result => {
      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  }

  


  function runShellCmd(aCommands) {
    let deferred = Promise.defer();

    ++pendingShellCount;
    originalRunEmulatorShell(aCommands, function(aResult) {
      --pendingShellCount;
      deferred.resolve(aResult);
    });

    return deferred.promise;
  }

  


  function waitFinish() {
    let deferred = Promise.defer();

    waitFor(function() {
      deferred.resolve();
    }, function() {
      return pendingCmdCount === 0 && pendingShellCount === 0;
    });

    return deferred.promise;
  }

  return {
    runCmd: runCmd,
    runCmdWithCallback: runCmdWithCallback,
    runShellCmd: runShellCmd,
    waitFinish: waitFinish
  };
}());




(function() {
  


  function delay(ms) {
    let deferred = Promise.defer();

    let startTime = Date.now();
    waitFor(function() {
      deferred.resolve();
    },function() {
      let duration = Date.now() - startTime;
      return (duration >= ms);
    });

    return deferred.promise;
  }

  


  function waitForNoCall() {
    let deferred = Promise.defer();

    waitFor(function() {
      deferred.resolve();
    }, function() {
      return telephony.calls.length === 0;
    });

    return deferred.promise;
  }

  


  function clearCalls() {
    log("Clear existing calls.");

    
    let hangUpPromises = [];

    for (let call of telephony.calls) {
      log(".. hangUp " + call.id.number);
      hangUpPromises.push(hangUp(call));
    }

    for (let call of conference.calls) {
      log(".. hangUp " + call.id.number);
      hangUpPromises.push(hangUp(call));
    }

    return Promise.all(hangUpPromises)
      .then(() => {
        return emulator.runCmd("gsm clear").then(waitForNoCall);
      })
      .then(waitForNoCall);
  }

  








  function callStrPool(prefix, number) {
    let padding = "           : ";
    let numberInfo = prefix + number + padding.substr(number.length);

    let info = {};
    let states = ['ringing', 'incoming', 'active', 'held'];
    for (let state of states) {
      info[state] = numberInfo + state;
    }

    return info;
  }

  







  function outCallStrPool(number) {
    return callStrPool("outbound to  ", number);
  }

  







  function inCallStrPool(number) {
    return callStrPool("inbound from ", number);
  }

  



  function checkInitialState() {
    log("Verify initial state.");
    ok(telephony.calls, 'telephony.call');
    checkTelephonyActiveAndCalls(null, []);
    ok(conference.calls, 'conference.calls');
    checkConferenceStateAndCalls('', []);
  }

  


  function checkEventCallState(event, call, state) {
    is(call, event.call, "event.call");
    is(call.state, state, "call state");
  }

  


  function checkTelephonyActiveAndCalls(active, calls) {
    is(telephony.active, active, "telephony.active");
    is(telephony.calls.length, calls.length, "telephony.calls");
    for (let i = 0; i < calls.length; ++i) {
      is(telephony.calls[i], calls[i]);
    }
  }

  



  function checkConferenceStateAndCalls(state, calls) {
    is(conference.state, state, "conference.state");
    is(conference.calls.length, calls.length, "conference.calls");
    for (let i = 0; i < calls.length; i++) {
      is(conference.calls[i], calls[i]);
    }
  }

  












  function check_oncallschanged(container, containerName, expectedCalls,
                                callback) {
    container.oncallschanged = function(event) {
      log("Received 'callschanged' event for the " + containerName);
      if (event.call) {
        let index = expectedCalls.indexOf(event.call);
        ok(index != -1);
        expectedCalls.splice(index, 1);

        if (expectedCalls.length === 0) {
          container.oncallschanged = null;
          callback();
        }
      }
    };
  }

  












  function check_ongroupchange(call, callName, group, callback) {
    call.ongroupchange = function(event) {
      log("Received 'groupchange' event for the " + callName);
      call.ongroupchange = null;

      is(call.group, group);
      callback();
    };
  }

  












  function check_onstatechange(container, containerName, state, callback) {
    container.onstatechange = function(event) {
      log("Received 'statechange' event for the " + containerName);
      container.onstatechange = null;

      is(container.state, state);
      callback();
    };
  }

  







  function StateEventChecker(state, previousEvent) {
    let event = 'on' + state;

    return function(call, callName, callback) {
      call[event] = function() {
        log("Received '" + state + "' event for the " + callName);
        call[event] = null;

        if (previousEvent) {
          
          
          
          ok(!call[previousEvent]);
        }
        is(call.state, state);
        callback();
      };
    };
  }

  















  function checkCallId(number, numberPresentation, name, namePresentation,
                       receivedNumber, receivedName) {
    let expectedNum = !numberPresentation ? number : "";
    is(receivedNumber, expectedNum, "check number per numberPresentation");

    let expectedName;
    if (numberPresentation) {
      expectedName = "";
    } else if (!namePresentation) {
      expectedName = name ? name : "";
    } else {
      expectedName = "";
    }
    is(receivedName, expectedName, "check name per number/namePresentation");
  }

  






  function checkEmulatorCallList(expectedCallList) {
    return emulator.runCmd("gsm list").then(result => {
      log("Call list is now: " + result);
      for (let i = 0; i < expectedCallList.length; ++i) {
        is(result[i], expectedCallList[i], "emulator calllist");
      }
    });
  }

  














  function checkState(active, calls, conferenceState, conferenceCalls) {
    checkTelephonyActiveAndCalls(active, calls);
    checkConferenceStateAndCalls(conferenceState, conferenceCalls);
  }

  

















  function checkAll(active, calls, conferenceState, conferenceCalls, callList) {
    checkState(active, calls, conferenceState, conferenceCalls);
    return checkEmulatorCallList(callList);
  }

  



  









  function receivedPending(received, pending, nextAction) {
    let index = pending.indexOf(received);
    if (index != -1) {
      pending.splice(index, 1);
    }
    if (pending.length === 0) {
      nextAction();
    }
  }

  








  function dial(number, serviceId) {
    serviceId = typeof serviceId !== "undefined" ? serviceId : 0;
    log("Make an outgoing call: " + number + ", serviceId: " + serviceId);

    let deferred = Promise.defer();

    telephony.dial(number, serviceId).then(call => {
      ok(call);
      is(call.id.number, number);
      is(call.state, "dialing");
      is(call.serviceId, serviceId);

      call.onalerting = function onalerting(event) {
        call.onalerting = null;
        log("Received 'onalerting' call event.");
        checkEventCallState(event, call, "alerting");
        deferred.resolve(call);
      };
    }, cause => {
      deferred.reject(cause);
    });

    return deferred.promise;
  }

  






  function dialEmergency(number) {
    log("Make an outgoing emergency call: " + number);

    let deferred = Promise.defer();

    telephony.dialEmergency(number).then(call => {
      ok(call);
      is(call.id.number, number);
      is(call.state, "dialing");

      call.onalerting = function onalerting(event) {
        call.onalerting = null;
        log("Received 'onalerting' call event.");
        checkEventCallState(event, call, "alerting");
        deferred.resolve(call);
      };
    }, cause => {
      deferred.reject(cause);
    });

    return deferred.promise;
  }

  









  function answer(call, conferenceStateChangeCallback) {
    log("Answering the incoming call.");

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve(call);
    };

    let pending = ["call.onconnected"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    
    
    
    if (conference.state === "connected") {
      pending.push("conference.onstatechange");
      check_onstatechange(conference, "conference", "held", function() {
        if (typeof conferenceStateChangeCallback === "function") {
          conferenceStateChangeCallback();
        }
        receive("conference.onstatechange");
      });
    }

    call.onconnecting = function onconnectingIn(event) {
      log("Received 'connecting' call event for incoming call.");
      call.onconnecting = null;
      checkEventCallState(event, call, "connecting");
    };

    call.onconnected = function onconnectedIn(event) {
      log("Received 'connected' call event for incoming call.");
      call.onconnected = null;
      checkEventCallState(event, call, "connected");
      ok(!call.onconnecting);
      receive("call.onconnected");
    };
    call.answer();

    return deferred.promise;
  }

  






  function hold(call) {
    log("Putting the call on hold.");

    let deferred = Promise.defer();

    let gotHolding = false;
    call.onholding = function onholding(event) {
      log("Received 'holding' call event");
      call.onholding = null;
      checkEventCallState(event, call, "holding");
      gotHolding = true;
    };

    call.onheld = function onheld(event) {
      log("Received 'held' call event");
      call.onheld = null;
      checkEventCallState(event, call, "held");
      ok(gotHolding);
      deferred.resolve(call);
    };
    call.hold();

    return deferred.promise;
  }

  






  function hangUp(call) {
    let deferred = Promise.defer();

    call.ondisconnected = function(event) {
      log("Received 'disconnected' call event");
      call.ondisconnected = null;
      checkEventCallState(event, call, "disconnected");
      deferred.resolve(call);
    };
    call.hangUp();

    return deferred.promise;
  }

  












  function remoteDial(number, numberPresentation, name, namePresentation) {
    log("Simulating an incoming call.");

    let deferred = Promise.defer();

    telephony.onincoming = function onincoming(event) {
      log("Received 'incoming' call event.");
      telephony.onincoming = null;

      let call = event.call;

      ok(call);
      is(call.state, "incoming");
      checkCallId(number, numberPresentation, name, namePresentation,
                  call.id.number, call.id.name);
      deferred.resolve(call);
    };

    numberPresentation = numberPresentation || "";
    name = name || "";
    namePresentation = namePresentation || "";
    emulator.runCmd("gsm call " + number + "," + numberPresentation + "," + name +
                 "," + namePresentation);
    return deferred.promise;
  }

  






  function remoteAnswer(call) {
    log("Remote answering the call.");

    let deferred = Promise.defer();

    call.onconnected = function onconnected(event) {
      log("Received 'connected' call event.");
      call.onconnected = null;
      checkEventCallState(event, call, "connected");
      deferred.resolve(call);
    };
    emulator.runCmd("gsm accept " + call.id.number);

    return deferred.promise;
  }

  






  function remoteHangUp(call) {
    log("Remote hanging up the call.");

    let deferred = Promise.defer();

    call.ondisconnected = function ondisconnected(event) {
      log("Received 'disconnected' call event.");
      call.ondisconnected = null;
      checkEventCallState(event, call, "disconnected");
      deferred.resolve(call);
    };
    emulator.runCmd("gsm cancel " + call.id.number);

    return deferred.promise;
  }

  






  function remoteHangUpCalls(calls) {
    let promise = Promise.resolve();

    for (let call of calls) {
      promise = promise.then(remoteHangUp.bind(null, call));
    }

    return promise;
  }

  












  function addCallsToConference(callsToAdd, connectedCallback, twice) {
    log("Add " + callsToAdd.length + " calls into conference.");

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve();
    };

    let pending = ["conference.oncallschanged", "conference.onconnected"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    let check_onconnected  = StateEventChecker('connected', 'onresuming');

    for (let call of callsToAdd) {
      let callName = "callToAdd (" + call.id.number + ')';

      let ongroupchange = callName + ".ongroupchange";
      pending.push(ongroupchange);
      check_ongroupchange(call, callName, conference,
                          receive.bind(null, ongroupchange));

      let onstatechange = callName + ".onstatechange";
      pending.push(onstatechange);
      check_onstatechange(call, callName, 'connected',
                          receive.bind(null, onstatechange));
    }

    check_oncallschanged(conference, 'conference', callsToAdd,
                         receive.bind(null, "conference.oncallschanged"));

    check_onconnected(conference, "conference", function() {
      ok(!conference.oncallschanged);
      if (typeof connectedCallback === 'function') {
        connectedCallback();
      }
      receive("conference.onconnected");
    });

    
    let requestCount = twice ? 2 : 1;
    for (let i = 0; i < requestCount; ++i) {
      if (callsToAdd.length == 2) {
        conference.add(callsToAdd[0], callsToAdd[1]);
      } else {
        conference.add(callsToAdd[0]);
      }
    }

    return deferred.promise;
  }

  









  function holdConference(calls, heldCallback) {
    log("Holding the conference call.");

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve();
    };

    let pending = ["conference.onholding", "conference.onheld"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    let check_onholding = StateEventChecker('holding', null);
    let check_onheld = StateEventChecker('held', 'onholding');

    for (let call of calls) {
      let callName = "call (" + call.id.number + ')';

      let onholding = callName + ".onholding";
      pending.push(onholding);
      check_onholding(call, callName, receive.bind(null, onholding));

      let onheld = callName + ".onheld";
      pending.push(onheld);
      check_onheld(call, callName, receive.bind(null, onheld));
    }

    check_onholding(conference, "conference",
                    receive.bind(null, "conference.onholding"));

    check_onheld(conference, "conference", function() {
      if (typeof heldCallback === 'function') {
        heldCallback();
      }
      receive("conference.onheld");
    });

    conference.hold();

    return deferred.promise;
  }

  









  function resumeConference(calls, connectedCallback) {
    log("Resuming the held conference call.");

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve();
    };

    let pending = ["conference.onresuming", "conference.onconnected"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    let check_onresuming   = StateEventChecker('resuming', null);
    let check_onconnected  = StateEventChecker('connected', 'onresuming');

    for (let call of calls) {
      let callName = "call (" + call.id.number + ')';

      let onresuming = callName + ".onresuming";
      pending.push(onresuming);
      check_onresuming(call, callName, receive.bind(null, onresuming));

      let onconnected = callName + ".onconnected";
      pending.push(onconnected);
      check_onconnected(call, callName, receive.bind(null, onconnected));
    }

    check_onresuming(conference, "conference",
                     receive.bind(null, "conference.onresuming"));

    check_onconnected(conference, "conference", function() {
      if (typeof connectedCallback === 'function') {
        connectedCallback();
      }
      receive("conference.onconnected");
    });

    conference.resume();

    return deferred.promise;
  }

  













  function removeCallInConference(callToRemove, autoRemovedCalls, remainedCalls,
                                  statechangeCallback) {
    log("Removing a participant from the conference call.");

    is(conference.state, 'connected');

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve();
    };

    let pending = ["callToRemove.ongroupchange", "telephony.oncallschanged",
                   "conference.oncallschanged", "conference.onstatechange"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    
    for (let call of remainedCalls) {
      let callName = "remainedCall (" + call.id.number + ')';

      let onstatechange = callName + ".onstatechange";
      pending.push(onstatechange);
      check_onstatechange(call, callName, 'held',
                          receive.bind(null, onstatechange));
    }

    
    
    for (let call of autoRemovedCalls) {
      let callName = "autoRemovedCall (" + call.id.number + ')';

      let ongroupchange = callName + ".ongroupchange";
      pending.push(ongroupchange);
      check_ongroupchange(call, callName, null,
                          receive.bind(null, ongroupchange));

      let onstatechange = callName + ".onstatechange";
      pending.push(onstatechange);
      check_onstatechange(call, callName, 'held',
                          receive.bind(null, onstatechange));
    }

    check_ongroupchange(callToRemove, "callToRemove", null, function() {
      is(callToRemove.state, 'connected');
      receive("callToRemove.ongroupchange");
    });

    check_oncallschanged(telephony, 'telephony',
                         autoRemovedCalls.concat(callToRemove),
                         receive.bind(null, "telephony.oncallschanged"));

    check_oncallschanged(conference, 'conference',
                         autoRemovedCalls.concat(callToRemove), function() {
      is(conference.calls.length, remainedCalls.length);
      receive("conference.oncallschanged");
    });

    check_onstatechange(conference, 'conference',
                        (remainedCalls.length ? 'held' : ''), function() {
      ok(!conference.oncallschanged);
      if (typeof statechangeCallback === 'function') {
        statechangeCallback();
      }
      receive("conference.onstatechange");
    });

    conference.remove(callToRemove);

    return deferred.promise;
  }

  













  function hangUpCallInConference(callToHangUp, autoRemovedCalls, remainedCalls,
                                  statechangeCallback) {
    log("Release one call in conference.");

    let deferred = Promise.defer();
    let done = function() {
      deferred.resolve();
    };

    let pending = ["conference.oncallschanged", "remoteHangUp"];
    let receive = function(name) {
      receivedPending(name, pending, done);
    };

    
    
    for (let call of autoRemovedCalls) {
      let callName = "autoRemovedCall (" + call.id.number + ')';

      let ongroupchange = callName + ".ongroupchange";
      pending.push(ongroupchange);
      check_ongroupchange(call, callName, null,
                          receive.bind(null, ongroupchange));
    }

    if (autoRemovedCalls.length) {
      pending.push("telephony.oncallschanged");
      check_oncallschanged(telephony, 'telephony',
                           autoRemovedCalls,
                           receive.bind(null, "telephony.oncallschanged"));
    }

    check_oncallschanged(conference, 'conference',
                         autoRemovedCalls.concat(callToHangUp), function() {
      is(conference.calls.length, remainedCalls.length);
      receive("conference.oncallschanged");
    });

    if (remainedCalls.length === 0) {
      pending.push("conference.onstatechange");
      check_onstatechange(conference, 'conference', '', function() {
        ok(!conference.oncallschanged);
        if (typeof statechangeCallback === 'function') {
          statechangeCallback();
        }
        receive("conference.onstatechange");
      });
    }

    remoteHangUp(callToHangUp)
      .then(receive.bind(null, "remoteHangUp"));

    return deferred.promise;
  }

  








  function createConferenceWithTwoCalls(outNumber, inNumber) {
    let outCall;
    let inCall;
    let outInfo = outCallStrPool(outNumber);
    let inInfo = inCallStrPool(inNumber);

    return Promise.resolve()
      .then(checkInitialState)
      .then(() => dial(outNumber))
      .then(call => { outCall = call; })
      .then(() => checkAll(outCall, [outCall], '', [], [outInfo.ringing]))
      .then(() => remoteAnswer(outCall))
      .then(() => checkAll(outCall, [outCall], '', [], [outInfo.active]))
      .then(() => remoteDial(inNumber))
      .then(call => { inCall = call; })
      .then(() => checkAll(outCall, [outCall, inCall], '', [],
                           [outInfo.active, inInfo.incoming]))
      .then(() => answer(inCall))
      .then(() => checkAll(inCall, [outCall, inCall], '', [],
                           [outInfo.held, inInfo.active]))
      .then(() => addCallsToConference([outCall, inCall], function() {
        checkState(conference, [], 'connected', [outCall, inCall]);
      }))
      .then(() => checkAll(conference, [], 'connected', [outCall, inCall],
                           [outInfo.active, inInfo.active]))
      .then(() => {
        return [outCall, inCall];
      });
  }

  








  function createCallAndAddToConference(inNumber, conferenceCalls) {
    
    let allInfo = conferenceCalls.map(function(call, i) {
      return (i === 0) ? outCallStrPool(call.id.number)
                       : inCallStrPool(call.id.number);
    });

    
    
    function addInfoState(allInfo, state) {
      Object.defineProperty(allInfo, state, {
        get: function() {
          return allInfo.map(function(info) { return info[state]; });
        }
      });
    }

    for (let state of ['ringing', 'incoming', 'active', 'held']) {
      addInfoState(allInfo, state);
    }

    let newCall;
    let newInfo = inCallStrPool(inNumber);

    return remoteDial(inNumber)
      .then(call => { newCall = call; })
      .then(() => checkAll(conference, [newCall], 'connected', conferenceCalls,
                           allInfo.active.concat(newInfo.incoming)))
      .then(() => answer(newCall, function() {
        checkState(newCall, [newCall], 'held', conferenceCalls);
      }))
      .then(() => checkAll(newCall, [newCall], 'held', conferenceCalls,
                           allInfo.held.concat(newInfo.active)))
      .then(() => {
        
        conferenceCalls.push(newCall);
        allInfo.push(newInfo);
      })
      .then(() => addCallsToConference([newCall], function() {
        checkState(conference, [], 'connected', conferenceCalls);
      }))
      .then(() => checkAll(conference, [], 'connected', conferenceCalls,
                           allInfo.active))
      .then(() => {
        return conferenceCalls;
      });
  }

  







  function setupConference(callNumbers) {
    log("Create a conference with " + callNumbers.length + " calls.");

    let promise = createConferenceWithTwoCalls(callNumbers[0], callNumbers[1]);

    callNumbers.shift();
    callNumbers.shift();
    for (let number of callNumbers) {
      promise = promise.then(createCallAndAddToConference.bind(null, number));
    }

    return promise;
  }

  



  this.gDelay = delay;
  this.gCheckInitialState = checkInitialState;
  this.gClearCalls = clearCalls;
  this.gOutCallStrPool = outCallStrPool;
  this.gInCallStrPool = inCallStrPool;
  this.gCheckState = checkState;
  this.gCheckAll = checkAll;
  this.gDial = dial;
  this.gDialEmergency = dialEmergency;
  this.gAnswer = answer;
  this.gHangUp = hangUp;
  this.gHold = hold;
  this.gRemoteDial = remoteDial;
  this.gRemoteAnswer = remoteAnswer;
  this.gRemoteHangUp = remoteHangUp;
  this.gRemoteHangUpCalls = remoteHangUpCalls;
  this.gAddCallsToConference = addCallsToConference;
  this.gHoldConference = holdConference;
  this.gResumeConference = resumeConference;
  this.gRemoveCallInConference = removeCallInConference;
  this.gHangUpCallInConference = hangUpCallInConference;
  this.gSetupConference = setupConference;
  this.gReceivedPending = receivedPending;
}());

function _startTest(permissions, test) {
  function permissionSetUp() {
    SpecialPowers.setBoolPref("dom.mozSettings.enabled", true);
    for (let per of permissions) {
      SpecialPowers.addPermission(per, true, document);
    }
  }

  function permissionTearDown() {
    SpecialPowers.clearUserPref("dom.mozSettings.enabled");
    for (let per of permissions) {
      SpecialPowers.removePermission(per, document);
    }
  }

  let debugPref;

  function setUp() {
    log("== Test SetUp ==");

    
    debugPref = SpecialPowers.getBoolPref(kPrefRilDebuggingEnabled);
    SpecialPowers.setBoolPref(kPrefRilDebuggingEnabled, true);
    log("Set debugging pref: " + debugPref + " => true");

    permissionSetUp();

    
    telephony = window.navigator.mozTelephony;
    ok(telephony);
    conference = telephony.conferenceGroup;
    ok(conference);
    return gClearCalls().then(gCheckInitialState);
  }

  
  finish = (function() {
    let originalFinish = finish;

    function tearDown() {
      log("== Test TearDown ==");
      emulator.waitFinish()
        .then(() => {
          permissionTearDown();

          
          SpecialPowers.setBoolPref(kPrefRilDebuggingEnabled, debugPref);
          log("Set debugging pref: true => " + debugPref);
        })
        .then(function() {
          originalFinish.apply(this, arguments);
        });
    }

    return tearDown.bind(this);
  }());

  function mainTest() {
    setUp()
      .then(function onSuccess() {
        log("== Test Start ==");
        test();
      }, function onError(error) {
        SpecialPowers.Cu.reportError(error);
        ok(false, "SetUp error");
      });
  }

  mainTest();
}

function startTest(test) {
  _startTest(["telephony"], test);
}

function startTestWithPermissions(permissions, test) {
  _startTest(permissions.concat("telephony"), test);
}

function startDSDSTest(test) {
  let numRIL;
  try {
    numRIL = SpecialPowers.getIntPref("ril.numRadioInterfaces");
  } catch (ex) {
    numRIL = 1;  
  }

  if (numRIL > 1) {
    startTest(test);
  } else {
    log("Not a DSDS environment. Test is skipped.");
    ok(true);  
    finish();
  }
}

function sendMMI(aMmi) {
  let deferred = Promise.defer();

  telephony.dial(aMmi)
    .then(request => {
      ok(request instanceof DOMRequest,
         "request is instanceof " + request.constructor);

      request.addEventListener("success", function(event) {
        deferred.resolve(request.result);
      });

      request.addEventListener("error", function(event) {
        deferred.reject(request.error);
      });
    }, cause => {
      deferred.reject(cause);
    });

  return deferred.promise;
}
