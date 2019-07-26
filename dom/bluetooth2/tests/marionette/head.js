














const EMULATOR_ADDRESS = "56:34:12:00:54:52";





const EMULATOR_NAME = "Full Android on Emulator";







const EMULATOR_CLASS = 0x58020c;



const BDADDR_ANY   = "00:00:00:00:00:00";
const BDADDR_ALL   = "ff:ff:ff:ff:ff:ff";
const BDADDR_LOCAL = "ff:ff:ff:00:00:00";


const REMOTE_DEVICE_NAME = "Remote BT Device";

let Promise =
  SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;

let bluetoothManager;

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
      ok(false, "Got an abnormal response from emulator.");
      log("Fail to execute emulator command: [" + aCommand + "]");
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}



















function addEmulatorRemoteDevice(aProperties) {
  let address;
  let promise = runEmulatorCmdSafe("bt remote add")
    .then(function(aResults) {
      address = aResults[0].toUpperCase();
    });

  for (let key in aProperties) {
    let value = aProperties[key];
    let propertyName = key;
    promise = promise.then(function() {
      return setEmulatorDeviceProperty(address, propertyName, value);
    });
  }

  return promise.then(function() {
    return address;
  });
}

















function removeEmulatorRemoteDevice(aAddress) {
  let cmd = "bt remote remove " + aAddress;
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      
      
      return aResults.slice(0, -1);
    });
}




















function setEmulatorDeviceProperty(aAddress, aPropertyName, aValue) {
  let cmd = "bt property " + aAddress + " " + aPropertyName + " " + aValue;
  return runEmulatorCmdSafe(cmd);
}


















function getEmulatorDeviceProperty(aAddress, aPropertyName) {
  let cmd = "bt property " + aAddress + " " + aPropertyName;
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      return aResults[0];
    });
}














function startDiscovery(aAdapter) {
  let deferred = Promise.defer();

  let request = aAdapter.startDiscovery();
  request.onsuccess = function () {
    log("  Start discovery - Success");
    
    
    
    
    deferred.resolve();
  }
  request.onerror = function (aEvent) {
    ok(false, "Start discovery - Fail");
    deferred.reject(aEvent.target.error);
  }

  return deferred.promise;
}














function stopDiscovery(aAdapter) {
  let deferred = Promise.defer();

  let request = aAdapter.stopDiscovery();
  request.onsuccess = function () {
    log("  Stop discovery - Success");
    
    
    
    
    deferred.resolve();
  }
  request.onerror = function (aEvent) {
    ok(false, "Stop discovery - Fail");
    deferred.reject(aEvent.target.error);
  }
  return deferred.promise;
}
















function getSettings(aKey) {
  let deferred = Promise.defer();

  let request = navigator.mozSettings.createLock().get(aKey);
  request.addEventListener("success", function(aEvent) {
    ok(true, "getSettings(" + aKey + ")");
    deferred.resolve(aEvent.target.result[aKey]);
  });
  request.addEventListener("error", function() {
    ok(false, "getSettings(" + aKey + ")");
    deferred.reject();
  });

  return deferred.promise;
}














function setSettings(aSettings) {
  let deferred = Promise.defer();

  let request = navigator.mozSettings.createLock().set(aSettings);
  request.addEventListener("success", function() {
    ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.resolve();
  });
  request.addEventListener("error", function() {
    ok(false, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.reject();
  });

  return deferred.promise;
}













function getBluetoothEnabled() {
  return getSettings("bluetooth.enabled");
}














function setBluetoothEnabled(aEnabled) {
  let obj = {};
  obj["bluetooth.enabled"] = aEnabled;
  return setSettings(obj);
}















function ensureBluetoothManager(aPermissions) {
  let deferred = Promise.defer();

  let permissions = ["bluetooth"];
  if (aPermissions) {
    if (Array.isArray(aPermissions)) {
      permissions = permissions.concat(aPermissions);
    } else if (typeof aPermissions == "string") {
      permissions.push(aPermissions);
    }
  }

  let obj = [];
  for (let perm of permissions) {
    obj.push({
      "type": perm,
      "allow": 1,
      "context": document,
    });
  }

  SpecialPowers.pushPermissions(obj, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    bluetoothManager = window.navigator.mozBluetooth;
    log("navigator.mozBluetooth is " +
        (bluetoothManager ? "available" : "unavailable"));

    if (bluetoothManager instanceof BluetoothManager) {
      deferred.resolve(bluetoothManager);
    } else {
      deferred.reject();
    }
  });

  return deferred.promise;
}













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  bluetoothManager.addEventListener(aEventName, function onevent(aEvent) {
    bluetoothManager.removeEventListener(aEventName, onevent);

    ok(true, "BluetoothManager event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}















function waitForAdapterEvent(aAdapter, aEventName) {
  let deferred = Promise.defer();

  aAdapter.addEventListener(aEventName, function onevent(aEvent) {
    aAdapter.removeEventListener(aEventName, onevent);

    ok(true, "BluetoothAdapter event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}












function setBluetoothEnabledAndWait(aEnabled) {
  let promises = [];

  
  
  
  
  
  
  
  promises.push(waitForManagerEvent(aEnabled ? "enabled" : "disabled"));
  promises.push(setBluetoothEnabled(aEnabled));

  return Promise.all(promises);
}











function getDefaultAdapter() {
  let deferred = Promise.defer();

  let request = bluetoothManager.getDefaultAdapter();
  request.onsuccess = function(aEvent) {
    let adapter = aEvent.target.result;
    if (!(adapter instanceof BluetoothAdapter)) {
      ok(false, "no BluetoothAdapter ready yet.");
      deferred.reject(null);
      return;
    }

    ok(true, "BluetoothAdapter got.");
    
    
    
    
    
    window.setTimeout(function() {
      deferred.resolve(adapter);
    }, 3000);
  };
  request.onerror = function(aEvent) {
    ok(false, "Failed to get default adapter.");
    deferred.reject(aEvent.target.error);
  };

  return deferred.promise;
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

function startBluetoothTestBase(aPermissions, aTestCaseMain) {
  ensureBluetoothManager(aPermissions)
    .then(aTestCaseMain)
    .then(cleanUp, function() {
      ok(false, "Unhandled rejected promise.");
      cleanUp();
    });
}

function startBluetoothTest(aReenable, aTestCaseMain) {
  startBluetoothTestBase(["settings-read", "settings-write"], function() {
    let origEnabled, needEnable;

    return getBluetoothEnabled()
      .then(function(aEnabled) {
        origEnabled = aEnabled;
        needEnable = !aEnabled;
        log("Original 'bluetooth.enabled' is " + origEnabled);

        if (aEnabled && aReenable) {
          log("  Disable 'bluetooth.enabled' ...");
          needEnable = true;
          return setBluetoothEnabledAndWait(false);
        }
      })
      .then(function() {
        if (needEnable) {
          log("  Enable 'bluetooth.enabled' ...");

          
          
          let promises = [];
          promises.push(waitForManagerEvent("adapteradded"));
          promises.push(setBluetoothEnabledAndWait(true));
          return Promise.all(promises);
        }
      })
      .then(getDefaultAdapter)
      .then(aTestCaseMain)
      .then(function() {
        if (!origEnabled) {
          return setBluetoothEnabledAndWait(false);
        }
      });
  });
}
