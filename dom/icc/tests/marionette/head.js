


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

const PREF_KEY_RIL_DEBUGGING_ENABLED = "ril.debugging.enabled";


const DEFAULT_PIN = "0000";


Promise.defer = function() { return new Deferred(); }
function Deferred() {
  this.promise = new Promise(function(resolve, reject) {
    this.resolve = resolve;
    this.reject = reject;
  }.bind(this));
  Object.freeze(this);
}

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

let workingFrame;
let iccManager;

















function ensureIccManager(aAdditionalPermissions) {
  let deferred = Promise.defer();

  aAdditionalPermissions = aAdditionalPermissions || [];

  if (aAdditionalPermissions.indexOf("mobileconnection") < 0) {
    aAdditionalPermissions.push("mobileconnection");
  }
  let permissions = [];
  for (let perm of aAdditionalPermissions) {
    permissions.push({ "type": perm, "allow": 1, "context": document });
  }

  SpecialPowers.pushPermissions(permissions, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    
    
    workingFrame = document.createElement("iframe");
    workingFrame.addEventListener("load", function load() {
      workingFrame.removeEventListener("load", load);

      iccManager = workingFrame.contentWindow.navigator.mozIccManager;

      if (iccManager) {
        ok(true, "navigator.mozIccManager is instance of " + iccManager.constructor);
      } else {
        ok(true, "navigator.mozIccManager is undefined");
      }

      if (iccManager instanceof MozIccManager) {
        deferred.resolve(iccManager);
      } else {
        deferred.reject();
      }
    });

    document.body.appendChild(workingFrame);
  });

  return deferred.promise;
}









function getMozIcc(aIccId) {
  aIccId = aIccId || iccManager.iccIds[0];

  if (!aIccId) {
    ok(true, "iccManager.iccIds[0] is undefined");
    return null;
  }

  return iccManager.getIccById(aIccId);
}









function getMozMobileConnectionByServiceId(aServiceId) {
  aServiceId = aServiceId || 0;
  return workingFrame.contentWindow.navigator.mozMobileConnections[aServiceId];
}















function setRadioEnabled(aEnabled, aServiceId) {
  return getMozMobileConnectionByServiceId(aServiceId).setRadioEnabled(aEnabled)
    .then(() => {
      ok(true, "setRadioEnabled " + aEnabled + " on " + aServiceId + " success.");
    }, (aError) => {
      ok(false, "setRadioEnabled " + aEnabled + " on " + aServiceId + " " +
         aError.name);
    });
}

















function waitForTargetEvent(aEventTarget, aEventName, aMatchFun) {
  let deferred = Promise.defer();

  aEventTarget.addEventListener(aEventName, function onevent(aEvent) {
    if (!aMatchFun || aMatchFun(aEvent)) {
      aEventTarget.removeEventListener(aEventName, onevent);
      ok(true, "Event '" + aEventName + "' got.");
      deferred.resolve(aEvent);
    }
  });

  return deferred.promise;
}
















function setRadioEnabledAndWait(aEnabled, aServiceId) {
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  let promises = [];

  promises.push(waitForTargetEvent(mobileConn, "radiostatechange", function() {
    
    
    return mobileConn.radioState === (aEnabled ? "enabled" : "disabled");
  }));
  promises.push(setRadioEnabled(aEnabled, aServiceId));

  return Promise.all(promises);
}













function restartRadioAndWait(aCardState) {
  return setRadioEnabledAndWait(false).then(() => {
    let promises = [];

    promises.push(waitForTargetEvent(iccManager, "iccdetected")
      .then((aEvent) => {
        let icc = getMozIcc(aEvent.iccId);
        if (icc.cardState !== aCardState) {
          return waitForTargetEvent(icc, "cardstatechange", function() {
            return icc.cardState === aCardState;
          });
        }
      }));
    promises.push(setRadioEnabledAndWait(true));

    return Promise.all(promises);
  });
}
















function setPinLockEnabled(aIcc, aEnabled) {
  let options = {
    lockType: "pin",
    enabled: aEnabled,
    pin: DEFAULT_PIN
  };

  return aIcc.setCardLock(options);
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return _pendingEmulatorCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  
  let debugPref = SpecialPowers.getBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED);
  SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, true);

  Promise.resolve()
    .then(aTestCaseMain)
    .catch((aError) => {
      ok(false, "promise rejects during test: " + aError);
    })
    .then(() => {
      
      SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, debugPref);
      cleanUp();
    });
}













function startTestCommon(aTestCaseMain, aAdditionalPermissions) {
  startTestBase(function() {
    return ensureIccManager(aAdditionalPermissions)
      .then(aTestCaseMain);
  });
}
