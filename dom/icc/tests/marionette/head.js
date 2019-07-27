


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
