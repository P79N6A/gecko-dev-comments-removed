


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;

let _pendingEmulatorCmdCount = 0;















function runEmulatorCmdSafe(aCommand) {
  let deferred = Promise.defer();

  ++_pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --_pendingEmulatorCmdCount;

    ok(true, "Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) &&
        aResult[aResult.length - 1] === "OK") {
      deferred.resolve(aResult);
    } else {
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}














function getEmulatorSensorValues(aSensorName) {
  return runEmulatorCmdSafe("sensor get " + aSensorName)
    .then(function(aResult) {
      
      return aResult[0].split(" ")[2].split(":").map(function(aElement) {
        return parseInt(aElement, 10);
      });
    });
}




function getEmulatorOrientationValues() {
  return getEmulatorSensorValues("orientation");
}













function setEmulatorOrientationValues(aAzimuth, aPitch, aRoll) {
  let cmd = "sensor set orientation " + aAzimuth + ":" + aPitch + ":" + aRoll;
  return runEmulatorCmdSafe(cmd);
}













function waitForWindowEvent(aEventName) {
  let deferred = Promise.defer();

  window.addEventListener(aEventName, function onevent(aEvent) {
    window.removeEventListener(aEventName, onevent);

    ok(true, "Window event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return _pendingEmulatorCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
    .then(aTestCaseMain)
    .then(cleanUp, function() {
      ok(false, 'promise rejects during test.');
      cleanUp();
    });
}
