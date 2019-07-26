


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;












let cbManager;
function ensureCellBroadcast() {
  let deferred = Promise.defer();

  let permissions = [{
    "type": "cellbroadcast",
    "allow": 1,
    "context": document,
  }];
  SpecialPowers.pushPermissions(permissions, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    cbManager = window.navigator.mozCellBroadcast;
    if (cbManager) {
      log("navigator.mozCellBroadcast is instance of " + cbManager.constructor);
    } else {
      log("navigator.mozCellBroadcast is undefined.");
    }

    if (cbManager instanceof window.MozCellBroadcast) {
      deferred.resolve(cbManager);
    } else {
      deferred.reject();
    }
  });

  return deferred.promise;
}
















let pendingEmulatorCmdCount = 0;
function runEmulatorCmdSafe(aCommand) {
  let deferred = Promise.defer();

  ++pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --pendingEmulatorCmdCount;

    ok(true, "Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) && aResult[aResult.length - 1] === "OK") {
      deferred.resolve(aResult);
    } else {
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}















function sendRawCbsToEmulator(aPdu) {
  let command = "cbs pdu " + aPdu;
  return runEmulatorCmdSafe(command);
}













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  cbManager.addEventListener(aEventName, function onevent(aEvent) {
    cbManager.removeEventListener(aEventName, onevent);

    ok(true, "Cellbroadcast event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}


















function sendMultipleRawCbsToEmulatorAndWait(aPdus) {
  let promises = [];

  promises.push(waitForManagerEvent("received"));
  for (let pdu of aPdus) {
    promises.push(sendRawCbsToEmulator(pdu));
  }

  return Promise.all(promises);
}




function cleanUp() {
  waitFor(function() {
    SpecialPowers.flushPermissions(function() {
      
      ok(true, "permissions flushed");

      finish();
    });
  }, function() {
    return pendingEmulatorCmdCount === 0;
  });
}















function selectModem(aServiceId) {
  let command = "mux modem " + aServiceId;
  return runEmulatorCmdSafe(command);
}








function runIfMultiSIM(aTest) {
  let numRIL;
  try {
    numRIL = SpecialPowers.getIntPref("ril.numRadioInterfaces");
  } catch (ex) {
    numRIL = 1;  
  }

  if (numRIL > 1) {
    return aTest();
  } else {
    log("Not a Multi-SIM environment. Test is skipped.");
    return Promise.resolve();
  }
}










function startTestCommon(aTestCaseMain) {
  Promise.resolve()
         .then(ensureCellBroadcast)
         .then(aTestCaseMain)
         .then(cleanUp, function() {
           ok(false, 'promise rejects during test.');
           cleanUp();
         });
}
