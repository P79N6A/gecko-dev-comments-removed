


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

const SETTINGS_KEY_DATA_ENABLED = "ril.data.enabled";
const SETTINGS_KEY_DATA_ROAMING_ENABLED = "ril.data.roaming_enabled";
const SETTINGS_KEY_DATA_APN_SETTINGS = "ril.data.apnSettings";

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












function wrapDomRequestAsPromise(aRequest) {
  let deferred = Promise.defer();

  ok(aRequest instanceof DOMRequest,
     "aRequest is instanceof " + aRequest.constructor);

  aRequest.addEventListener("success", function(aEvent) {
    deferred.resolve(aEvent);
  });
  aRequest.addEventListener("error", function(aEvent) {
    deferred.reject(aEvent);
  });

  return deferred.promise;
}



















function getSettings(aKey, aAllowError) {
  let request = navigator.mozSettings.createLock().get(aKey);
  return wrapDomRequestAsPromise(request)
    .then(function resolve(aEvent) {
      ok(true, "getSettings(" + aKey + ") - success");
      return aEvent.target.result[aKey];
    }, function reject(aEvent) {
      ok(aAllowError, "getSettings(" + aKey + ") - error");
    });
}

















function setSettings(aSettings, aAllowError) {
  let request = navigator.mozSettings.createLock().set(aSettings);
  return wrapDomRequestAsPromise(request)
    .then(function resolve() {
      ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
    }, function reject() {
      ok(aAllowError, "setSettings(" + JSON.stringify(aSettings) + ")");
    });
}



















function setSettings1(aKey, aValue, aAllowError) {
  let settings = {};
  settings[aKey] = aValue;
  return setSettings(settings, aAllowError);
}




function getDataEnabled(aAllowError) {
  return getSettings(SETTINGS_KEY_DATA_ENABLED, aAllowError);
}




function setDataEnabled(aEnabled, aAllowError) {
  return setSettings1(SETTINGS_KEY_DATA_ENABLED, aEnabled, aAllowError);
}




function getDataRoamingEnabled(aAllowError) {
  return getSettings(SETTINGS_KEY_DATA_ROAMING_ENABLED, aAllowError);
}




function setDataRoamingEnabled(aEnabled, aAllowError) {
  return setSettings1(SETTINGS_KEY_DATA_ROAMING_ENABLED, aEnabled, aAllowError);
}




function getDataApnSettings(aAllowError) {
  return getSettings(SETTINGS_KEY_DATA_APN_SETTINGS, aAllowError);
}




function setDataApnSettings(aApnSettings, aAllowError) {
  return setSettings1(SETTINGS_KEY_DATA_APN_SETTINGS, aApnSettings, aAllowError);
}

let mobileConnection;



















function ensureMobileConnection(aAdditionalPermissions, aServiceId) {
  let deferred = Promise.defer();

  aAdditionalPermissions = aAdditionalPermissions || [];
  aServiceId = aServiceId || 0;

  if (aAdditionalPermissions.indexOf("mobileconnection") < 0) {
    aAdditionalPermissions.push("mobileconnection");
  }
  let permissions = [];
  for (let perm of aAdditionalPermissions) {
    permissions.push({ "type": perm, "allow": 1, "context": document });
  }

  SpecialPowers.pushPermissions(permissions, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    
    
    let ifr = document.createElement("iframe");
    ifr.addEventListener("load", function load() {
      ifr.removeEventListener("load", load);

      mobileConnection =
        ifr.contentWindow.navigator.mozMobileConnections[aServiceId];

      if (mobileConnection) {
        log("navigator.mozMobileConnections[" + aServiceId + "] is instance of " +
            mobileConnection.constructor);
      } else {
        log("navigator.mozMobileConnections[" + aServiceId + "] is undefined");
      }

      if (mobileConnection instanceof MozMobileConnection) {
        deferred.resolve(mobileConnection);
      } else {
        deferred.reject();
      }
    });

    document.body.appendChild(ifr);
  });

  return deferred.promise;
}
















function waitForManagerEvent(aEventName, aServiceId) {
  let deferred = Promise.defer();

  let mobileConn = mobileConnection;
  if (aServiceId !== undefined) {
    mobileConn = navigator.mozMobileConnections[aServiceId];
  }

  mobileConn.addEventListener(aEventName, function onevent(aEvent) {
    mobileConn.removeEventListener(aEventName, onevent);

    ok(true, "MobileConnection event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}











function getNetworks() {
  let request = mobileConnection.getNetworks();
  return wrapDomRequestAsPromise(request)
    .then(() => request.result);
}













function selectNetwork(aNetwork) {
  let request = mobileConnection.selectNetwork(aNetwork);
  return wrapDomRequestAsPromise(request)
    .then(null, () => { throw request.error });
}













function selectNetworkAndWait(aNetwork) {
  let promises = [];

  promises.push(waitForManagerEvent("voicechange"));
  promises.push(selectNetwork(aNetwork));

  return Promise.all(promises);
}










function selectNetworkAutomatically() {
  let request = mobileConnection.selectNetworkAutomatically();
  return wrapDomRequestAsPromise(request)
    .then(null, () => { throw request.error });
}










function selectNetworkAutomaticallyAndWait() {
  let promises = [];

  promises.push(waitForManagerEvent("voicechange"));
  promises.push(selectNetworkAutomatically());

  return Promise.all(promises);
}
















function setDataEnabledAndWait(aEnabled, aServiceId) {
  let deferred = Promise.defer();

  let promises = [];
  promises.push(waitForManagerEvent("datachange", aServiceId));
  promises.push(setDataEnabled(aEnabled));
  Promise.all(promises).then(function keepWaiting() {
    let mobileConn = mobileConnection;
    if (aServiceId !== undefined) {
      mobileConn = navigator.mozMobileConnections[aServiceId];
    }
    
    
    let connected = mobileConn.data.connected;
    if (connected == aEnabled) {
      deferred.resolve();
      return;
    }

    return waitForManagerEvent("datachange", aServiceId).then(keepWaiting);
  });

  return deferred.promise;
}
















function setEmulatorVoiceDataStateAndWait(aWhich, aState, aServiceId) {
  let promises = [];
  promises.push(waitForManagerEvent(aWhich + "change", aServiceId));

  let cmd = "gsm " + aWhich + " " + aState;
  promises.push(runEmulatorCmdSafe(cmd));
  return Promise.all(promises);
}














function setEmulatorRoamingAndWait(aRoaming, aServiceId) {
  function doSetAndWait(aWhich, aRoaming, aServiceId) {
    let state = (aRoaming ? "roaming" : "home");
    return setEmulatorVoiceDataStateAndWait(aWhich, state, aServiceId)
      .then(() => {
        let mobileConn = mobileConnection;
        if (aServiceId !== undefined) {
          mobileConn = navigator.mozMobileConnections[aServiceId];
        }
        is(mobileConn[aWhich].roaming, aRoaming,
                     aWhich + ".roaming")
      });
  }

  
  return doSetAndWait("voice", aRoaming, aServiceId)
    .then(() => doSetAndWait("data", aRoaming, aServiceId));
}











function getEmulatorGsmLocation() {
  let cmd = "gsm location";
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      
      
      
      is(aResults[0].substring(0,3), "lac", "lac output");
      is(aResults[1].substring(0,2), "ci", "ci output");

      let lac = parseInt(aResults[0].substring(5));
      lac = (lac < 0 ? 65535 : lac);
      let cid = parseInt(aResults[1].substring(4));
      cid = (cid < 0 ? 268435455 : cid);

      return { lac: lac, cid: cid };
    });
}












function setEmulatorGsmLocation(aLac, aCid) {
  let cmd = "gsm location " + aLac + " " + aCid;
  return runEmulatorCmdSafe(cmd);
}











function getEmulatorOperatorNames() {
  let cmd = "operator dumpall";
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      let operators = [];

      for (let i = 0; i < aResults.length - 1; i++) {
        let names = aResults[i].split(',');
        operators.push({
          longName: names[0],
          shortName: names[1],
          mccMnc: names[2],
        });
      }

      ok(true, "emulator operators list: " + JSON.stringify(operators));
      return operators;
    });
}





















function setEmulatorOperatorNames(aOperator, aLongName, aShortName, aMcc, aMnc) {
  const EMULATOR_OPERATORS = [ "home", "roaming" ];

  let index = EMULATOR_OPERATORS.indexOf(aOperator);
  if (index < 0) {
    throw "invalid operator";
  }

  let cmd = "operator set " + index + " " + aLongName + "," + aShortName;
  if (aMcc && aMnc) {
    cmd = cmd + "," + aMcc + aMnc;
  }
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      let exp = "^" + aLongName + "," + aShortName + ",";
      if (aMcc && aMnc) {
        cmd = cmd + aMcc + aMnc;
      }

      let re = new RegExp(exp);
      ok(aResults[index].match(new RegExp(exp)),
         "Long/short name and/or mcc/mnc should be changed.");
    });
}

let _networkManager;




function getNetworkManager() {
  if (!_networkManager) {
    _networkManager = Cc["@mozilla.org/network/manager;1"]
                    .getService(Ci.nsINetworkManager);
    ok(_networkManager, "NetworkManager");
  }

  return _networkManager;
}

let _numOfRadioInterfaces;




function getNumOfRadioInterfaces() {
  if (!_numOfRadioInterfaces) {
    try {
      _numOfRadioInterfaces = SpecialPowers.getIntPref("ril.numRadioInterfaces");
    } catch (ex) {
      _numOfRadioInterfaces = 1;  
    }
  }

  return _numOfRadioInterfaces;
}




function cleanUp() {
  waitFor(function() {
    SpecialPowers.flushPermissions(function() {
      
      ok(true, "permissions flushed");

      finish();
    });
  }, function() {
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















function startTestCommon(aTestCaseMain, aAdditionalPermissions, aServiceId) {
  startTestBase(function() {
    return ensureMobileConnection(aAdditionalPermissions, aServiceId)
      .then(aTestCaseMain);
  });
}
















function startDSDSTestCommon(aTestCaseMain, aAdditionalPermissions, aServiceId) {
    if (getNumOfRadioInterfaces() > 1) {
      startTestBase(function() {
        return ensureMobileConnection(aAdditionalPermissions, aServiceId)
          .then(aTestCaseMain);
      });
    } else {
      log("Skipping DSDS tests on single SIM device.")
      ok(true);  
      cleanUp();
    }
}