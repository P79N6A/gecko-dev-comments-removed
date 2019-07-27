


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

const SETTINGS_KEY_DATA_ENABLED = "ril.data.enabled";
const SETTINGS_KEY_DATA_ROAMING_ENABLED = "ril.data.roaming_enabled";
const SETTINGS_KEY_DATA_APN_SETTINGS = "ril.data.apnSettings";

const PREF_KEY_RIL_DEBUGGING_ENABLED = "ril.debugging.enabled";


const DEFAULT_PIN = "0000";

const DEFAULT_PUK = "12345678";


Promise.defer = function() { return new Deferred(); };
function Deferred() {
  this.promise = new Promise(function(resolve, reject) {
    this.resolve = resolve;
    this.reject = reject;
  }.bind(this));
  Object.freeze(this);
}

let _pendingEmulatorCmdCount = 0;
let _pendingEmulatorShellCmdCount = 0;


















function runEmulatorCmdSafe(aCommand) {
  log("Emulator command: " + aCommand);
  let deferred = Promise.defer();

  ++_pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --_pendingEmulatorCmdCount;

    log("Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) &&
        aResult[aResult.length - 1] === "OK") {
      deferred.resolve(aResult);
    } else {
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}
















function runEmulatorShellCmdSafe(aCommands) {
  let deferred = Promise.defer();

  ++_pendingEmulatorShellCmdCount;
  runEmulatorShell(aCommands, function(aResult) {
    --_pendingEmulatorShellCmdCount;

    log("Emulator shell response: " + JSON.stringify(aResult));
    deferred.resolve(aResult);
  });

  return deferred.promise;
}

let workingFrame;



















function getSettings(aKey, aAllowError) {
  let request =
    workingFrame.contentWindow.navigator.mozSettings.createLock().get(aKey);
  return request.then(function resolve(aValue) {
      ok(true, "getSettings(" + aKey + ") - success");
      return aValue[aKey];
    }, function reject(aError) {
      ok(aAllowError, "getSettings(" + aKey + ") - error");
    });
}


















function setSettings(aSettings, aAllowError) {
  let lock = window.navigator.mozSettings.createLock();
  let request = lock.set(aSettings);
  let deferred = Promise.defer();
  lock.onsettingstransactionsuccess = function () {
      ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.resolve();
  };
  lock.onsettingstransactionfailure = function (aEvent) {
      ok(aAllowError, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.reject();
  };
  return deferred.promise;
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

    
    
    workingFrame = document.createElement("iframe");
    workingFrame.addEventListener("load", function load() {
      workingFrame.removeEventListener("load", load);

      mobileConnection =
        workingFrame.contentWindow.navigator.mozMobileConnections[aServiceId];

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

    document.body.appendChild(workingFrame);
  });

  return deferred.promise;
}










function getMozMobileConnectionByServiceId(aServiceId) {
  let mobileConn = mobileConnection;
  if (aServiceId !== undefined) {
    mobileConn =
      workingFrame.contentWindow.navigator.mozMobileConnections[aServiceId];
  }
  return mobileConn;
}






function getMozIccManager() {
  return workingFrame.contentWindow.navigator.mozIccManager;
}









function getMozIccByIccId(aIccId) {
  let iccManager = getMozIccManager();

  aIccId = aIccId || iccManager.iccIds[0];
  if (!aIccId) {
    ok(true, "iccManager.iccIds[0] is " + aIccId);
    return null;
  }

  return iccManager.getIccById(aIccId);
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


















function waitForManagerEvent(aEventName, aServiceId, aMatchFun) {
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  return waitForTargetEvent(mobileConn, aEventName, aMatchFun);
}











function getNetworks() {
  let request = mobileConnection.getNetworks();
  return request.then(() => request.result);
}













function selectNetwork(aNetwork) {
  let request = mobileConnection.selectNetwork(aNetwork);
  return request.then(null, () => { throw request.error });
}













function selectNetworkAndWait(aNetwork) {
  let promises = [];

  promises.push(waitForManagerEvent("voicechange"));
  promises.push(selectNetwork(aNetwork));

  return Promise.all(promises);
}










function selectNetworkAutomatically() {
  let request = mobileConnection.selectNetworkAutomatically();
  return request.then(null, () => { throw request.error });
}










function selectNetworkAutomaticallyAndWait() {
  let promises = [];

  promises.push(waitForManagerEvent("voicechange"));
  promises.push(selectNetworkAutomatically());

  return Promise.all(promises);
}













 function setRoamingPreference(aMode) {
  let request = mobileConnection.setRoamingPreference(aMode);
  return request.then(null, () => { throw request.error });
}
















 function setPreferredNetworkType(aType) {
  let request = mobileConnection.setPreferredNetworkType(aType);
  return request.then(null, () => { throw request.error });
}













 function getPreferredNetworkType() {
  let request = mobileConnection.getPreferredNetworkType();
  return request.then(() => request.result, () => { throw request.error });
}














 function setCallForwardingOption(aOptions) {
  let request = mobileConnection.setCallForwardingOption(aOptions);
  return request.then(null, () => { throw request.error });
}















 function getCallForwardingOption(aReason) {
  let request = mobileConnection.getCallForwardingOption(aReason);
  return request.then(() => request.result, () => { throw request.error });
}













 function setVoicePrivacyMode(aEnabled) {
  let request = mobileConnection.setVoicePrivacyMode(aEnabled);
  return request.then(null, () => { throw request.error });
}











 function getVoicePrivacyMode() {
  let request = mobileConnection.getVoicePrivacyMode();
  return request.then(() => request.result, () => { throw request.error });
}











 function setCallBarringOption(aOptions) {
  let request = mobileConnection.setCallBarringOption(aOptions);
  return request.then(null, () => { throw request.error });
}












 function getCallBarringOption(aOptions) {
  let request = mobileConnection.getCallBarringOption(aOptions);
  return request.then(() => request.result, () => { throw request.error });
}










 function changeCallBarringPassword(aOptions) {
  let request = mobileConnection.changeCallBarringPassword(aOptions);
  return request.then(null, () => { throw request.error });
}
















function setDataEnabledAndWait(aEnabled, aServiceId) {
  let deferred = Promise.defer();

  let promises = [];
  promises.push(waitForManagerEvent("datachange", aServiceId));
  promises.push(setDataEnabled(aEnabled));
  Promise.all(promises).then(function keepWaiting() {
    let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
    
    
    let connected = mobileConn.data.connected;
    if (connected == aEnabled) {
      deferred.resolve();
      return;
    }

    return waitForManagerEvent("datachange", aServiceId).then(keepWaiting);
  });

  return deferred.promise;
}
















function setRadioEnabled(aEnabled, aServiceId) {
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  let request = mobileConn.setRadioEnabled(aEnabled);
  return request.then(function onsuccess() {
      ok(true, "setRadioEnabled " + aEnabled + " on " + aServiceId + " success.");
    }, function onerror() {
      ok(false, "setRadioEnabled " + aEnabled + " on " + aServiceId + " " +
         request.error.name);
    });
}
















function setRadioEnabledAndWait(aEnabled, aServiceId) {
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);

  if (mobileConn.radioState === (aEnabled ? "enabled" : "disabled")) {
    return Promise.resolve();
  }

  let expectedSequence = aEnabled ? ["enabling", "enabled"] :
                                    ["disabling", "disabled"];

  let p1 = waitForManagerEvent("radiostatechange", aServiceId, function() {
    let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
    let expectedRadioState = expectedSequence.shift();
    is(mobileConn.radioState, expectedRadioState, "Check radio state");
    return expectedSequence.length === 0;
  });

  let p2 = setRadioEnabled(aEnabled, aServiceId);

  return Promise.all([p1, p2]);
}
















function setClir(aMode, aServiceId) {
  ok(true, "setClir(" + aMode + ", " + aServiceId + ")");
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  let request = mobileConn.setCallingLineIdRestriction(aMode);
  return request.then(null, () => { throw request.error });
}















function getClir(aServiceId) {
  ok(true, "getClir(" + aServiceId + ")");
  let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
  let request = mobileConn.getCallingLineIdRestriction();
  return request.then(() => request.result, () => { throw request.error });
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
        let mobileConn = getMozMobileConnectionByServiceId(aServiceId);
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
      let cid = parseInt(aResults[1].substring(4));
      return { lac: lac, cid: cid };
    });
}












function setEmulatorGsmLocation(aLac, aCid) {
  let cmd = "gsm location " + aLac + " " + aCid;
  return runEmulatorCmdSafe(cmd);
}















function setEmulatorGsmLocationAndWait(aLac, aCid,
                                       aWaitVoice = true, aWaitData = false) {
  let promises = [];
  if (aWaitVoice) {
    promises.push(waitForManagerEvent("voicechange"));
  }
  if (aWaitData) {
    promises.push(waitForManagerEvent("datachange"));
  }
  promises.push(setEmulatorGsmLocation(aLac, aCid));
  return Promise.all(promises);
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























function setEmulatorOperatorNamesAndWait(aOperator, aLongName, aShortName,
                                         aMcc, aMnc,
                                         aWaitVoice = true, aWaitData = false) {
  let promises = [];
  if (aWaitVoice) {
    promises.push(waitForManagerEvent("voicechange"));
  }
  if (aWaitData) {
    promises.push(waitForManagerEvent("datachange"));
  }
  promises.push(setEmulatorOperatorNames(aOperator, aLongName, aShortName,
                                         aMcc, aMnc));
  return Promise.all(promises);
}











function setEmulatorGsmSignalStrength(aRssi) {
  let cmd = "gsm signal " + aRssi;
  return runEmulatorCmdSafe(cmd);
}














function setEmulatorGsmSignalStrengthAndWait(aRssi,
                                             aWaitVoice = true,
                                             aWaitData = false) {
  let promises = [];
  if (aWaitVoice) {
    promises.push(waitForManagerEvent("voicechange"));
  }
  if (aWaitData) {
    promises.push(waitForManagerEvent("datachange"));
  }
  promises.push(setEmulatorGsmSignalStrength(aRssi));
  return Promise.all(promises);
}













function setEmulatorLteSignalStrength(aRxlev, aRsrp, aRssnr) {
  let cmd = "gsm lte_signal " + aRxlev + " " + aRsrp + " " + aRssnr;
  return runEmulatorCmdSafe(cmd);
}
















function setEmulatorLteSignalStrengthAndWait(aRxlev, aRsrp, aRssnr,
                                             aWaitVoice = true,
                                             aWaitData = false) {
  let promises = [];
  if (aWaitVoice) {
    promises.push(waitForManagerEvent("voicechange"));
  }
  if (aWaitData) {
    promises.push(waitForManagerEvent("datachange"));
  }
  promises.push(setEmulatorLteSignalStrength(aRxlev, aRsrp, aRssnr));
  return Promise.all(promises);
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
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return _pendingEmulatorCmdCount === 0 &&
           _pendingEmulatorShellCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  
  let debugPref = SpecialPowers.getBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED);
  SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, true);

  return Promise.resolve()
    .then(aTestCaseMain)
    .catch((aError) => {
      ok(false, "promise rejects during test: " + aError);
    })
    .then(() => {
      
      SpecialPowers.setBoolPref(PREF_KEY_RIL_DEBUGGING_ENABLED, debugPref);
      cleanUp();
    })
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
