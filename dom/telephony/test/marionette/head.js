


let Promise = SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;
let telephony;
let conference;




let emulator = (function() {
  let pendingCmdCount = 0;
  let originalRunEmulatorCmd = runEmulatorCmd;

  
  runEmulatorCmd = function() {
    throw "Use emulator.run(cmd, callback) instead of runEmulatorCmd";
  };

  function run(cmd, callback) {
    pendingCmdCount++;
    originalRunEmulatorCmd(cmd, function(result) {
      pendingCmdCount--;
      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  }

  


  function waitFinish() {
    let deferred = Promise.defer();

    waitFor(function() {
      deferred.resolve();
    }, function() {
      return pendingCmdCount === 0;
    });

    return deferred.promise;
  }

  return {
    run: run,
    waitFinish: waitFinish
  };
}());




(function() {
  


  function clearCalls() {
    let deferred = Promise.defer();

    log("Clear existing calls.");
    emulator.run("gsm clear", function(result) {
      if (result[0] == "OK") {
        waitFor(function() {
          deferred.resolve();
        }, function() {
          return telephony.calls.length === 0;
        });
      } else {
        log("Failed to clear existing calls.");
        deferred.reject();
      }
    });

    return deferred.promise;
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

  






  function checkEmulatorCallList(expectedCallList) {
    let deferred = Promise.defer();

    emulator.run("gsm list", function(result) {
        log("Call list is now: " + result);
        for (let i = 0; i < expectedCallList.length; ++i) {
          is(result[i], expectedCallList[i], "emulator calllist");
        }
        is(result[expectedCallList.length], "OK", "emulator calllist");
        deferred.resolve();
        });

    return deferred.promise;
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
      is(call.number, number);
      is(call.state, "dialing");
      is(call.serviceId, serviceId);

      call.onalerting = function onalerting(event) {
        call.onalerting = null;
        log("Received 'onalerting' call event.");
        checkEventCallState(event, call, "alerting");
        deferred.resolve(call);
      };
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

  






  function remoteDial(number) {
    log("Simulating an incoming call.");

    let deferred = Promise.defer();

    telephony.onincoming = function onincoming(event) {
      log("Received 'incoming' call event.");
      telephony.onincoming = null;

      let call = event.call;

      ok(call);
      is(call.number, number);
      is(call.state, "incoming");

      deferred.resolve(call);
    };
    emulator.run("gsm call " + number);

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
    emulator.run("gsm accept " + call.number);

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
    emulator.run("gsm cancel " + call.number);

    return deferred.promise;
  }

  






  function remoteHangUpCalls(calls) {
    let promise = Promise.resolve();

    for (let call of calls) {
      promise = promise.then(remoteHangUp.bind(null, call));
    }

    return promise;
  }

  










  function addCallsToConference(callsToAdd, connectedCallback) {
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
      let callName = "callToAdd (" + call.number + ')';

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

    
    if (callsToAdd.length == 2) {
      conference.add(callsToAdd[0], callsToAdd[1]);
    } else {
      conference.add(callsToAdd[0]);
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
      let callName = "call (" + call.number + ')';

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
      let callName = "call (" + call.number + ')';

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
      let callName = "remainedCall (" + call.number + ')';

      let onstatechange = callName + ".onstatechange";
      pending.push(onstatechange);
      check_onstatechange(call, callName, 'held',
                          receive.bind(null, onstatechange));
    }

    
    
    for (let call of autoRemovedCalls) {
      let callName = "autoRemovedCall (" + call.number + ')';

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
      let callName = "autoRemovedCall (" + call.number + ')';

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

  








  function setupConferenceTwoCalls(outNumber, inNumber) {
    log('Create conference with two calls.');

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

  










  function setupConferenceThreeCalls(outNumber, inNumber, inNumber2) {
    log('Create conference with three calls.');

    let outCall;
    let inCall;
    let inCall2;
    let outInfo = outCallStrPool(outNumber);
    let inInfo = inCallStrPool(inNumber);
    let inInfo2 = inCallStrPool(inNumber2);

    return Promise.resolve()
      .then(() => setupConferenceTwoCalls(outNumber, inNumber))
      .then(calls => {
          outCall = calls[0];
          inCall = calls[1];
      })
      .then(() => remoteDial(inNumber2))
      .then(call => { inCall2 = call; })
      .then(() => checkAll(conference, [inCall2], 'connected', [outCall, inCall],
                           [outInfo.active, inInfo.active, inInfo2.incoming]))
      .then(() => answer(inCall2, function() {
        checkState(inCall2, [inCall2], 'held', [outCall, inCall]);
      }))
      .then(() => checkAll(inCall2, [inCall2], 'held', [outCall, inCall],
                           [outInfo.held, inInfo.held, inInfo2.active]))
      .then(() => addCallsToConference([inCall2], function() {
        checkState(conference, [], 'connected', [outCall, inCall, inCall2]);
      }))
      .then(() => checkAll(conference, [],
                           'connected', [outCall, inCall, inCall2],
                           [outInfo.active, inInfo.active, inInfo2.active]))
      .then(() => {
        return [outCall, inCall, inCall2];
      });
  }

  














  function setupConferenceFiveCalls(outNumber, inNumber, inNumber2, inNumber3,
                                    inNumber4) {
    log('Create conference with five calls.');

    let outCall;
    let inCall;
    let inCall2;
    let inCall3;
    let inCall4;
    let outInfo = outCallStrPool(outNumber);
    let inInfo = inCallStrPool(inNumber);
    let inInfo2 = inCallStrPool(inNumber2);
    let inInfo3 = inCallStrPool(inNumber3);
    let inInfo4 = inCallStrPool(inNumber4);

    return Promise.resolve()
      .then(() => setupConferenceThreeCalls(outNumber, inNumber, inNumber2))
      .then(calls => {
        [outCall, inCall, inCall2] = calls;
      })
      .then(() => remoteDial(inNumber3))
      .then(call => {inCall3 = call;})
      .then(() => checkAll(conference, [inCall3], 'connected',
                           [outCall, inCall, inCall2],
                           [outInfo.active, inInfo.active, inInfo2.active,
                           inInfo3.incoming]))
      .then(() => answer(inCall3, function() {
        checkState(inCall3, [inCall3], 'held', [outCall, inCall, inCall2]);
      }))
      .then(() => checkAll(inCall3, [inCall3], 'held',
                           [outCall, inCall, inCall2],
                           [outInfo.held, inInfo.held, inInfo2.held,
                            inInfo3.active]))
      .then(() => addCallsToConference([inCall3], function() {
        checkState(conference, [], 'connected', [outCall, inCall, inCall2, inCall3]);
      }))
      .then(() => checkAll(conference, [], 'connected',
                           [outCall, inCall, inCall2, inCall3],
                           [outInfo.active, inInfo.active, inInfo2.active,
                            inInfo3.active]))
      .then(() => remoteDial(inNumber4))
      .then(call => {inCall4 = call;})
      .then(() => checkAll(conference, [inCall4], 'connected',
                           [outCall, inCall, inCall2, inCall3],
                           [outInfo.active, inInfo.active, inInfo2.active,
                            inInfo3.active, inInfo4.incoming]))
      .then(() => answer(inCall4, function() {
        checkState(inCall4, [inCall4], 'held', [outCall, inCall, inCall2, inCall3]);
      }))
      .then(() => checkAll(inCall4, [inCall4], 'held',
                           [outCall, inCall, inCall2, inCall3],
                           [outInfo.held, inInfo.held, inInfo2.held,
                            inInfo3.held, inInfo4.active]))
      .then(() => addCallsToConference([inCall4], function() {
        checkState(conference, [], 'connected', [outCall, inCall, inCall2,
                                                 inCall3, inCall4]);
      }))
      .then(() => checkAll(conference, [], 'connected',
                           [outCall, inCall, inCall2, inCall3, inCall4],
                           [outInfo.active, inInfo.active, inInfo2.active,
                            inInfo3.active, inInfo4.active]))
      .then(() => {
        return [outCall, inCall, inCall2, inCall3, inCall4];
      });
  }

  



  this.gCheckInitialState = checkInitialState;
  this.gClearCalls = clearCalls;
  this.gOutCallStrPool = outCallStrPool;
  this.gInCallStrPool = inCallStrPool;
  this.gCheckState = checkState;
  this.gCheckAll = checkAll;
  this.gDial = dial;
  this.gAnswer = answer;
  this.gRemoteDial = remoteDial;
  this.gRemoteAnswer = remoteAnswer;
  this.gRemoteHangUp = remoteHangUp;
  this.gRemoteHangUpCalls = remoteHangUpCalls;
  this.gAddCallsToConference = addCallsToConference;
  this.gHoldConference = holdConference;
  this.gResumeConference = resumeConference;
  this.gRemoveCallInConference = removeCallInConference;
  this.gHangUpCallInConference = hangUpCallInConference;
  this.gSetupConferenceTwoCalls = setupConferenceTwoCalls;
  this.gSetupConferenceThreeCalls = setupConferenceThreeCalls;
  this.gSetupConferenceFiveCalls = setupConferenceFiveCalls;
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

  function setUp() {
    log("== Test SetUp ==");
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
        .then(permissionTearDown)
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
