



Promise.defer = function() { return new Deferred(); };
function Deferred()  {
  this.promise = new Promise(function(resolve, reject) {
    this.resolve = resolve;
    this.reject = reject;
  }.bind(this));
  Object.freeze(this);
}

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
    throw "Use emulator.runShellCmd(cmd) instead of runEmulatorShell";
  };

  


  function runCmd(cmd) {
    return new Promise(function(resolve, reject) {
      pendingCmdCount++;
      originalRunEmulatorCmd(cmd, function(result) {
        pendingCmdCount--;
        if (result[result.length - 1] === "OK") {
          resolve(result);
        } else {
          is(result[result.length - 1], "OK", "emulator command result.");
          reject();
        }
      });
    });
  }

  


  function runCmdWithCallback(cmd, callback) {
    return runCmd(cmd).then(result => {
      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  }

  


  function runShellCmd(aCommands) {
    return new Promise(function(resolve, reject) {
      ++pendingShellCount;
      originalRunEmulatorShell(aCommands, function(aResult) {
        --pendingShellCount;
        resolve(aResult);
      });
    });
  }

  


  function waitFinish() {
    return new Promise(function(resolve, reject) {
      waitFor(function() {
        resolve();
      }, function() {
        return pendingCmdCount === 0 && pendingShellCount === 0;
      });
    });
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
    return new Promise(function(resolve, reject) {
      let startTime = Date.now();
      waitFor(function() {
        resolve();
      },function() {
        let duration = Date.now() - startTime;
        return (duration >= ms);
      });
    });
  }

  















  function waitForSystemMessage(aMessageName, aMatchFun = null) {
    
    
    let systemMessenger = SpecialPowers.Cc["@mozilla.org/system-message-internal;1"]
                                       .getService(SpecialPowers.Ci.nsISystemMessagesInternal);

    
    systemMessenger.registerPage(aMessageName,
                                 SpecialPowers.Services.io.newURI("app://system.gaiamobile.org/index.html", null, null),
                                 SpecialPowers.Services.io.newURI("app://system.gaiamobile.org/manifest.webapp", null, null));

    return new Promise(function(aResolve, aReject) {
      window.navigator.mozSetMessageHandler(aMessageName, function(aMessage) {
        if (!aMatchFun || aMatchFun(aMessage)) {
          log("System message '" + aMessageName + "' got.");
          window.navigator.mozSetMessageHandler(aMessageName, null);
          aResolve(aMessage);
        }
      });
    });
  }

  











  function waitForEvent(aTarget, aEventName, aPredicate) {
    return new Promise(function(resolve, reject) {
      aTarget.addEventListener(aEventName, function onevent(aEvent) {
        if (aPredicate === undefined || aPredicate(aEvent)) {
          aTarget.removeEventListener(aEventName, onevent);

          let label = "X";
          if (aTarget instanceof TelephonyCall) {
            label = "Call (" + aTarget.id.number + ")";
          } else if (aTarget instanceof TelephonyCallGroup) {
            label = "Conference";
          } else if (aTarget instanceof Telephony) {
            label = "Telephony";
          }

          log(label + " received event '" + aEventName + "'");
          resolve(aEvent);
        }
      });
    });
  }

  








  function waitForCallsChangedEvent(aTarget, aExpectedCall) {
    if (aExpectedCall === undefined) {
      return waitForEvent(aTarget, "callschanged").then(event => event.call);
    } else {
      return waitForEvent(aTarget, "callschanged",
                          event => event.call == aExpectedCall)
               .then(event => event.call);
    }
  }

  








  function waitForNamedStateEvent(aTarget, aState) {
    return waitForEvent(aTarget, aState)
      .then(event => {
        if (aTarget instanceof TelephonyCall) {
          is(aTarget, event.call, "event.call");
        }
        is(aTarget.state, aState, "check state");
        return aTarget;
      });
  }

  








  function waitForGroupChangeEvent(aCall, aGroup) {
    return waitForEvent(aCall, "groupchange")
      .then(() => {
        is(aCall.group, aGroup, "call group");
        return aCall;
      });
  }

  








  function waitForStateChangeEvent(aTarget, aState) {
    return waitForEvent(aTarget, "statechange")
      .then(() => {
        is(aTarget.state, aState);
        return aTarget;
      });
  }

  


  function waitForNoCall() {
    return new Promise(function(resolve, reject) {
      waitFor(function() {
        resolve();
      }, function() {
        return telephony.calls.length === 0;
      });
    });
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
    let states = ["ringing", "incoming", "waiting", "active", "held"];
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
    ok(conference.calls, 'conference.calls');
    checkState(null, [], "", []);
  }

  


  function checkCalls(actualCalls, expectedCalls) {
    if (actualCalls.length != expectedCalls.length) {
      ok(false, "check calls.length");
      return;
    }

    let expectedSet = new Set(expectedCalls);
    for (let i = 0; i < actualCalls.length; ++i) {
      ok(expectedSet.has(actualCalls[i]), "should contain the call");
    }
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
    is(telephony.active, active, "telephony.active");
    checkCalls(telephony.calls, calls);
    is(conference.state, conferenceState, "conference.state");
    checkCalls(conference.calls, conferenceCalls);
  }

  

















  function checkAll(active, calls, conferenceState, conferenceCalls, callList) {
    checkState(active, calls, conferenceState, conferenceCalls);
    return checkEmulatorCallList(callList);
  }

  



  








  function dial(number, serviceId) {
    serviceId = typeof serviceId !== "undefined" ? serviceId : 0;
    log("Make an outgoing call: " + number + ", serviceId: " + serviceId);

    let outCall;

    return telephony.dial(number, serviceId)
      .then(call => outCall = call)
      .then(() => {
        ok(outCall instanceof TelephonyCall, "check instance");
        is(outCall.id.number, number);
        is(outCall.state, "dialing");
        is(outCall.serviceId, serviceId);
      })
      .then(() => waitForNamedStateEvent(outCall, "alerting"));
  }

  






  function dialEmergency(number) {
    log("Make an outgoing emergency call: " + number);

    let outCall;

    return telephony.dialEmergency(number)
      .then(call => outCall = call)
      .then(() => {
        ok(outCall instanceof TelephonyCall, "check instance");
        ok(outCall);
        is(outCall.id.number, number);
        is(outCall.state, "dialing");
      })
      .then(() => waitForNamedStateEvent(outCall, "alerting"))
      .then(() => {
        is(outCall.emergency, true, "check emergency");
        return outCall;
      });
  }

  






  function dialSTK(number) {
    log("STK makes an outgoing call: " + number);

    let p1 = waitForCallsChangedEvent(telephony);
    let p2 = emulator.runCmd("stk setupcall " + number);

    return Promise.all([p1, p2])
      .then(result => {
        let call = result[0];

        ok(call instanceof TelephonyCall, "check instance");
        is(call.id.number, number, "check number");
        is(call.state, "dialing", "check call state");

        return waitForNamedStateEvent(call, "alerting");
      });
  }

  









  function answer(call, conferenceStateChangeCallback) {
    log("Answering the incoming call.");

    let promises = [];

    
    
    
    if (conference.state === "connected") {
      let promise = waitForStateChangeEvent(conference, "held")
        .then(() => {
          if (typeof conferenceStateChangeCallback === "function") {
            conferenceStateChangeCallback();
          }
        });

      promises.push(promise);
    }

    promises.push(waitForNamedStateEvent(call, "connected"));
    promises.push(call.answer());

    return Promise.all(promises).then(() => call);
  }

  






  function hold(call) {
    log("Putting the call on hold.");

    let promises = [];

    promises.push(waitForNamedStateEvent(call, "held"));
    promises.push(call.hold());

    return Promise.all(promises).then(() => call);
  }

  






  function resume(call) {
    log("Resuming the held call.");

    let promises = [];

    promises.push(waitForNamedStateEvent(call, "connected"));
    promises.push(call.resume());

    return Promise.all(promises).then(() => call);
  }

  






  function hangUp(call) {
    log("Local hanging up the call: " + call.id.number);

    let promises = [];

    promises.push(waitForNamedStateEvent(call, "disconnected"));
    promises.push(call.hangUp());

    return Promise.all(promises).then(() => call);
  }

  












  function remoteDial(number, numberPresentation, name, namePresentation) {
    log("Simulating an incoming call.");

    numberPresentation = numberPresentation || "";
    name = name || "";
    namePresentation = namePresentation || "";
    emulator.runCmd("gsm call " + number + "," + numberPresentation + "," + name +
                 "," + namePresentation);

    return waitForEvent(telephony, "incoming")
      .then(event => {
        let call = event.call;

        ok(call);
        is(call.state, "incoming");
        checkCallId(number, numberPresentation, name, namePresentation,
                    call.id.number, call.id.name);

        return call;
      });
  }

  






  function remoteAnswer(call) {
    log("Remote answering the call: " + call.id.number);

    emulator.runCmd("gsm accept " + call.id.number);

    return waitForNamedStateEvent(call, "connected");
  }

  






  function remoteHangUp(call) {
    log("Remote hanging up the call: " + call.id.number);

    emulator.runCmd("gsm cancel " + call.id.number);

    return waitForNamedStateEvent(call, "disconnected");
  }

  






  function remoteHangUpCalls(calls) {
    let promises = calls.map(remoteHangUp);
    return Promise.all(promises);
  }

  










  function addCallsToConference(callsToAdd, connectedCallback) {
    log("Add " + callsToAdd.length + " calls into conference.");

    let promises = [];

    for (let call of callsToAdd) {
      promises.push(waitForCallsChangedEvent(conference, call));
      promises.push(waitForGroupChangeEvent(call, conference));
      promises.push(waitForNamedStateEvent(call, "connected"));
      promises.push(waitForStateChangeEvent(call, "connected"));
    }

    let promise = waitForNamedStateEvent(conference, "connected")
      .then(() => {
        if (typeof connectedCallback === "function") {
          connectedCallback();
        }
      });
    promises.push(promise);

    
    if (callsToAdd.length == 2) {
      promise = conference.add(callsToAdd[0], callsToAdd[1]);
      promises.push(promise);
    } else {
      promise = conference.add(callsToAdd[0]);
      promises.push(promise);
    }

    return Promise.all(promises).then(() => conference.calls);
  }

  









  function holdConference(callsInConference, heldCallback) {
    log("Holding the conference call.");

    let promises = [];

    for (let call of callsInConference) {
      promises.push(waitForNamedStateEvent(call, "held"));
    }

    let promise = waitForNamedStateEvent(conference, "holding")
      .then(() => waitForNamedStateEvent(conference, "held"))
      .then(() => {
        if (typeof heldCallback === "function") {
          heldCallback();
        }
      });
    promises.push(promise);

    promises.push(conference.hold());

    return Promise.all(promises).then(() => conference.calls);
  }

  









  function resumeConference(callsInConference, connectedCallback) {
    log("Resuming the held conference call.");

    let promises = [];

    for (let call of callsInConference) {
      promises.push(waitForNamedStateEvent(call, "connected"));
    }

    let promise = waitForNamedStateEvent(conference, "resuming")
      .then(() => waitForNamedStateEvent(conference, "connected"))
      .then(() => {
        if (typeof connectedCallback === "function") {
          connectedCallback();
        }
      });
    promises.push(promise);

    promises.push(conference.resume());

    return Promise.all(promises).then(() => conference.calls);
  }

  













  function removeCallInConference(callToRemove, autoRemovedCalls, remainedCalls,
                                  statechangeCallback) {
    log("Removing a participant from the conference call.");

    is(conference.state, 'connected');

    let promises = [];

    
    promises.push(waitForCallsChangedEvent(telephony, callToRemove));
    promises.push(waitForCallsChangedEvent(conference, callToRemove));
    promises.push(waitForGroupChangeEvent(callToRemove, null).then(() => {
      is(callToRemove.state, 'connected');
    }));

    
    
    for (let call of autoRemovedCalls) {
      promises.push(waitForCallsChangedEvent(telephony, call));
      promises.push(waitForCallsChangedEvent(conference, call));
      promises.push(waitForGroupChangeEvent(call, null));
      promises.push(waitForStateChangeEvent(call, "held"));
    }

    
    for (let call of remainedCalls) {
      promises.push(waitForStateChangeEvent(call, "held"));
    }

    let finalConferenceState = remainedCalls.length ? "held" : "";
    let promise = waitForStateChangeEvent(conference, finalConferenceState)
      .then(() => {
        if (typeof statechangeCallback === 'function') {
          statechangeCallback();
        }
      });
    promises.push(promise);

    promises.push(conference.remove(callToRemove));

    return Promise.all(promises)
      .then(() => checkCalls(conference.calls, remainedCalls))
      .then(() => conference.calls);
  }

  













  function hangUpCallInConference(callToHangUp, autoRemovedCalls, remainedCalls,
                                  statechangeCallback) {
    log("Release one call in conference.");

    let promises = [];

    
    promises.push(waitForCallsChangedEvent(conference, callToHangUp));

    
    
    for (let call of autoRemovedCalls) {
      promises.push(waitForCallsChangedEvent(telephony, call));
      promises.push(waitForCallsChangedEvent(conference, call));
      promises.push(waitForGroupChangeEvent(call, null));
    }

    if (remainedCalls.length === 0) {
      let promise = waitForStateChangeEvent(conference, "")
        .then(() => {
          if (typeof statechangeCallback === 'function') {
            statechangeCallback();
          }
        });
      promises.push(promise);
    }

    promises.push(remoteHangUp(callToHangUp));

    return Promise.all(promises)
      .then(() => checkCalls(conference.calls, remainedCalls))
      .then(() => conference.calls);
  }

  




  function hangUpConference() {
    log("Hangup conference.");

    let promises = [];

    promises.push(waitForStateChangeEvent(conference, ""));

    for (let call of conference.calls) {
      promises.push(waitForNamedStateEvent(call, "disconnected"));
    }

    return conference.hangUp().then(() => {
      return Promise.all(promises);
    });
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
                           [outInfo.active, inInfo.waiting]))
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

    for (let state of ["ringing", "incoming", "waiting", "active", "held"]) {
      addInfoState(allInfo, state);
    }

    let newCall;
    let newInfo = inCallStrPool(inNumber);

    return remoteDial(inNumber)
      .then(call => { newCall = call; })
      .then(() => checkAll(conference, [newCall], 'connected', conferenceCalls,
                           allInfo.active.concat(newInfo.waiting)))
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

  






  function sendMMI(mmi) {
    return telephony.dial(mmi).then(mmiCall => {
      ok(mmiCall instanceof MMICall, "mmiCall is instance of MMICall");
      ok(mmiCall.result instanceof Promise, "result is Promise");
      return mmiCall.result;
    });
  }

  function sendTone(tone, pause, serviceId) {
    log("Send DTMF " + tone + " serviceId " + serviceId);
    return telephony.sendTones(tone, pause, null, serviceId);
  }

  








  function setRadioEnabled(connection, enabled) {
    let desiredRadioState = enabled ? 'enabled' : 'disabled';
    log("Set radio: " + desiredRadioState);

    if (connection.radioState === desiredRadioState) {
      return Promise.resolve();
    }

    let promises = [];

    let promise = gWaitForEvent(connection, "radiostatechange", event => {
      let state = connection.radioState;
      log("current radioState: " + state);
      return state == desiredRadioState;
    });
    promises.push(promise);

    promises.push(connection.setRadioEnabled(enabled));

    return Promise.all(promises);
  }

  function setRadioEnabledAll(enabled) {
    let promises = [];
    let numOfSim = navigator.mozMobileConnections.length;

    for (let i = 0; i < numOfSim; i++) {
      let connection = navigator.mozMobileConnections[i];
      ok(connection instanceof MozMobileConnection,
         "connection[" + i + "] is instanceof " + connection.constructor);

         promises.push(setRadioEnabled(connection, enabled));
    }

    return Promise.all(promises);
  }

  



  this.gDelay = delay;
  this.gWaitForSystemMessage = waitForSystemMessage;
  this.gWaitForEvent = waitForEvent;
  this.gWaitForCallsChangedEvent = waitForCallsChangedEvent;
  this.gWaitForNamedStateEvent = waitForNamedStateEvent;
  this.gWaitForStateChangeEvent = waitForStateChangeEvent;
  this.gCheckInitialState = checkInitialState;
  this.gClearCalls = clearCalls;
  this.gOutCallStrPool = outCallStrPool;
  this.gInCallStrPool = inCallStrPool;
  this.gCheckState = checkState;
  this.gCheckAll = checkAll;
  this.gSendMMI = sendMMI;
  this.gDial = dial;
  this.gDialEmergency = dialEmergency;
  this.gDialSTK = dialSTK;
  this.gAnswer = answer;
  this.gHangUp = hangUp;
  this.gHold = hold;
  this.gResume = resume;
  this.gRemoteDial = remoteDial;
  this.gRemoteAnswer = remoteAnswer;
  this.gRemoteHangUp = remoteHangUp;
  this.gRemoteHangUpCalls = remoteHangUpCalls;
  this.gAddCallsToConference = addCallsToConference;
  this.gHoldConference = holdConference;
  this.gResumeConference = resumeConference;
  this.gRemoveCallInConference = removeCallInConference;
  this.gHangUpCallInConference = hangUpCallInConference;
  this.gHangUpConference = hangUpConference;
  this.gSendTone = sendTone;
  this.gSetupConference = setupConference;
  this.gSetRadioEnabled = setRadioEnabled;
  this.gSetRadioEnabledAll = setRadioEnabledAll;
}());

function _startTest(permissions, test) {
  function typesToPermissions(types) {
    return types.map(type => {
      return {
        "type": type,
        "allow": 1,
        "context": document
      };
    });
  }

  function ensureRadio() {
    log("== Ensure Radio ==");
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPermissions(typesToPermissions(["mobileconnection"]), () => {
        gSetRadioEnabledAll(true).then(() => {
          SpecialPowers.popPermissions(() => {
            resolve();
          });
        });
      });
    });
  }

  function permissionSetUp() {
    log("== Permission SetUp ==");
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPermissions(typesToPermissions(permissions), resolve);
    });
  }

  let debugPref;

  function setUp() {
    log("== Test SetUp ==");

    
    debugPref = SpecialPowers.getBoolPref(kPrefRilDebuggingEnabled);
    SpecialPowers.setBoolPref(kPrefRilDebuggingEnabled, true);
    log("Set debugging pref: " + debugPref + " => true");

    return Promise.resolve()
      .then(ensureRadio)
      .then(permissionSetUp)
      .then(() => {
        
        telephony = window.navigator.mozTelephony;
        ok(telephony);
        conference = telephony.conferenceGroup;
        ok(conference);
      })
      .then(gClearCalls)
      .then(gCheckInitialState);
  }

  
  finish = (function() {
    let originalFinish = finish;

    function tearDown() {
      log("== Test TearDown ==");
      emulator.waitFinish()
        .then(() => {
          
          SpecialPowers.setBoolPref(kPrefRilDebuggingEnabled, debugPref);
          log("Set debugging pref: true => " + debugPref);
        })
        .then(function() {
          originalFinish.apply(this, arguments);
        });
    }

    return tearDown.bind(this);
  }());

  setUp().then(() => {
    log("== Test Start ==");
    test();
  })
  .catch(error => ok(false, error));
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
