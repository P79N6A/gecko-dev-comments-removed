





let Promise =
  SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;

let bluetoothManager;
















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
  SpecialPowers.flushPermissions(function() {
    
    ok(true, "permissions flushed");

    finish();
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
