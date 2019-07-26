


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
    if (Array.isArray(aResult) && aResult[0] === "OK") {
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













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  mobileConnection.addEventListener(aEventName, function onevent(aEvent) {
    mobileConnection.removeEventListener(aEventName, onevent);

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













function setDataEnabledAndWait(aEnabled) {
  let deferred = Promise.defer();

  let promises = [];
  promises.push(waitForManagerEvent("datachange"));
  promises.push(setDataEnabled(aEnabled));
  Promise.all(promises).then(function keepWaiting() {
    
    
    let connected = mobileConnection.data.connected;
    if (connected == aEnabled) {
      deferred.resolve();
      return;
    }

    return waitForManagerEvent("datachange").then(keepWaiting);
  });

  return deferred.promise;
}











function setEmulatorRoamingAndWait(aRoaming) {
  function doSetAndWait(aWhich, aRoaming) {
    let promises = [];
    promises.push(waitForManagerEvent(aWhich + "change"));

    let cmd = "gsm " + aWhich + " " + (aRoaming ? "roaming" : "home");
    promises.push(runEmulatorCmdSafe(cmd));
    return Promise.all(promises)
      .then(() => is(mobileConnection[aWhich].roaming, aRoaming,
                     aWhich + ".roaming"));
  }

  
  return doSetAndWait("voice", aRoaming)
    .then(() => doSetAndWait("data", aRoaming));
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
