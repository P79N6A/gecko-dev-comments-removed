












const EMULATOR_ADDRESS = "56:34:12:00:54:52";





const EMULATOR_NAME = "Full Android on Emulator";







const EMULATOR_CLASS = 0x58020c;



const BDADDR_ANY   = "00:00:00:00:00:00";
const BDADDR_ALL   = "ff:ff:ff:ff:ff:ff";
const BDADDR_LOCAL = "ff:ff:ff:00:00:00";


const REMOTE_DEVICE_NAME = "Remote_BT_Device";


Promise.defer = function() { return new Deferred(); }
function Deferred()  {
  this.promise = new Promise(function(resolve, reject) {
    this.resolve = resolve;
    this.reject = reject;
  }.bind(this));
  Object.freeze(this);
}

let bluetoothManager;

let pendingEmulatorCmdCount = 0;















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
  let lock = window.navigator.mozSettings.createLock();
  let request = lock.set(aSettings);
  let deferred = Promise.defer();
  lock.onsettingstransactionsuccess = function () {
    ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.resolve();
  };
  lock.onsettingstransactionfailure = function (aEvent) {
    ok(false, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.reject();
  };
  return deferred.promise;
}












function getBluetoothEnabled() {
  log("bluetoothManager.defaultAdapter.state: " + bluetoothManager.defaultAdapter.state);

  return (bluetoothManager.defaultAdapter.state == "enabled");
}














function setBluetoothEnabled(aEnabled) {
  let obj = {};
  obj["bluetooth.enabled"] = aEnabled;
  return setSettings(obj);
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



















function waitForAdapterStateChanged(aAdapter, aStateChangesInOrder) {
  let deferred = Promise.defer();

  let stateIndex = 0;
  let prevStateIndex = 0;
  let statesArray = [];
  let changedAttrs = [];
  aAdapter.onattributechanged = function(aEvent) {
    for (let i in aEvent.attrs) {
      changedAttrs.push(aEvent.attrs[i]);
      switch (aEvent.attrs[i]) {
        case "state":
          log("  'state' changed to " + aAdapter.state);

          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          let stateIndex = aStateChangesInOrder.indexOf(aAdapter.state);
          if (stateIndex >= prevStateIndex && stateIndex + 1 > statesArray.length) {
            statesArray.push(aAdapter.state);
            prevStateIndex = stateIndex;

            if (statesArray.length == aStateChangesInOrder.length) {
              aAdapter.onattributechanged = null;
              ok(true, "BluetoothAdapter event 'onattributechanged' got.");
              deferred.resolve(changedAttrs);
            }
          } else {
            ok(false, "The order of 'onattributechanged' events is unexpected.");
          }

          break;
        case "name":
          log("  'name' changed to " + aAdapter.name);
          if (aAdapter.state == "enabling") {
            isnot(aAdapter.name, "", "adapter.name");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.name, "", "adapter.name");
          }
          break;
        case "address":
          log("  'address' changed to " + aAdapter.address);
          if (aAdapter.state == "enabling") {
            isnot(aAdapter.address, "", "adapter.address");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.address, "", "adapter.address");
          }
          break;
        case "discoverable":
          log("  'discoverable' changed to " + aAdapter.discoverable);
          if (aAdapter.state == "enabling") {
            is(aAdapter.discoverable, true, "adapter.discoverable");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.discoverable, false, "adapter.discoverable");
          }
          break;
        case "discovering":
          log("  'discovering' changed to " + aAdapter.discovering);
          if (aAdapter.state == "enabling") {
            is(aAdapter.discovering, true, "adapter.discovering");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.discovering, false, "adapter.discovering");
          }
          break;
        case "unknown":
        default:
          ok(false, "Unknown attribute '" + aEvent.attrs[i] + "' changed." );
          break;
      }
    }
  };

  return deferred.promise;
}



















function waitForAdapterAttributeChanged(aAdapter, aAttrName, aExpectedValue) {
  let deferred = Promise.defer();

  aAdapter.onattributechanged = function(aEvent) {
    let i = aEvent.attrs.indexOf(aAttrName);
    if (i >= 0) {
      switch (aEvent.attrs[i]) {
        case "state":
          log("  'state' changed to " + aAdapter.state);
          is(aAdapter.state, aExpectedValue, "adapter.state");
          break;
        case "name":
          log("  'name' changed to " + aAdapter.name);
          is(aAdapter.name, aExpectedValue, "adapter.name");
          break;
        case "address":
          log("  'address' changed to " + aAdapter.address);
          is(aAdapter.address, aExpectedValue, "adapter.address");
          break;
        case "discoverable":
          log("  'discoverable' changed to " + aAdapter.discoverable);
          is(aAdapter.discoverable, aExpectedValue, "adapter.discoverable");
          break;
        case "discovering":
          log("  'discovering' changed to " + aAdapter.discovering);
          is(aAdapter.discovering, aExpectedValue, "adapter.discovering");
          break;
        case "unknown":
        default:
          ok(false, "Unknown attribute '" + aAttrName + "' changed." );
          break;
      }
      aAdapter.onattributechanged = null;
      deferred.resolve(aEvent);
    }
  };

  return deferred.promise;
}

















function waitForDevicesFound(aDiscoveryHandle, aExpectedNumberOfDevices) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let devicesArray = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    ok(aEvent instanceof BluetoothDeviceEvent,
      "aEvent should be a BluetoothDeviceEvent");

    devicesArray.push(aEvent);
    if (devicesArray.length >= aExpectedNumberOfDevices) {
      aDiscoveryHandle.ondevicefound = null;
      deferred.resolve(devicesArray);
    }
  };

  return deferred.promise;
}

















function waitForSpecifiedDevicesFound(aDiscoveryHandle, aRemoteAddresses) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let devicesArray = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    ok(aEvent instanceof BluetoothDeviceEvent,
      "aEvent should be a BluetoothDeviceEvent");

    if (aRemoteAddresses.indexOf(aEvent.device.address) != -1) {
      devicesArray.push(aEvent);
    }
    if (devicesArray.length == aRemoteAddresses.length) {
      aDiscoveryHandle.ondevicefound = null;
      ok(true, "BluetoothAdapter has found all remote devices.");
      deferred.resolve(devicesArray);
    }
  };

  return deferred.promise;
}



















function verifyDevicePairedUnpairedEvent(aAdapter, aDeviceEvent) {
  let deferred = Promise.defer();

  let devices = aAdapter.getPairedDevices();
  let isPromisePending = true;
  if (aDeviceEvent.device) {
    log("  - Verify 'devicepaired' event");
    for (let i in devices) {
      if (devices[i].address == aDeviceEvent.device.address) {
        deferred.resolve(aDeviceEvent);
        isPromisePending = false;
      }
    }
    if (isPromisePending) {
      deferred.reject(aDeviceEvent);
      isPromisePending = false;
    }
  } else if (aDeviceEvent.address) {
    log("  - Verify 'deviceunpaired' event");
    for (let i in devices) {
      if (devices[i].address == aDeviceEvent.address) {
        deferred.reject(aDeviceEvent);
        isPromisePending = false;
      }
    }
    if (isPromisePending) {
      deferred.resolve(aDeviceEvent);
      isPromisePending = false;
    }
  } else {
    log("  - Exception occurs. Unexpected aDeviceEvent properties.");
    deferred.reject(aDeviceEvent);
    isPromisePending = false;
  }

  return deferred.promise;
}











function addEventHandlerForPairingRequest(aAdapter, aSpecifiedBdAddress) {
  log("  - Add event handlers for pairing requests.");

  aAdapter.pairingReqs.ondisplaypasskeyreq = function onDisplayPasskeyReq(evt) {
    let passkey = evt.handle.passkey; 
    ok(typeof passkey === 'string', "type checking for passkey.");
    log("  - Received 'ondisplaypasskeyreq' event with passkey: " + passkey);

    let device = evt.device;
    if (!aSpecifiedBdAddress || device.address == aSpecifiedBdAddress) {
      cleanupPairingListener(aAdapter.pairingReqs);
    }
  };

  aAdapter.pairingReqs.onenterpincodereq = function onEnterPinCodeReq(evt) {
    log("  - Received 'onenterpincodereq' event.");

    let device = evt.device;
    if (!aSpecifiedBdAddress || device.address == aSpecifiedBdAddress) {
      
      let UserEnteredPinCode = "0000";
      let pinCode = UserEnteredPinCode;

      evt.handle.setPinCode(pinCode).then(
        function onResolve() {
          log("  - 'setPinCode' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'setPinCode' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };

  aAdapter.pairingReqs.onpairingconfirmationreq
    = function onPairingConfirmationReq(evt) {
    let passkey = evt.handle.passkey; 
    ok(typeof passkey === 'string', "type checking for passkey.");
    log("  - Received 'onpairingconfirmationreq' event with passkey: " + passkey);

    let device = evt.device;
    if (!aSpecifiedBdAddress || device.address == aSpecifiedBdAddress) {
      evt.handle.accept().then(
        function onResolve() {
          log("  - 'accept' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'accept' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };

  aAdapter.pairingReqs.onpairingconsentreq = function onPairingConsentReq(evt) {
    log("  - Received 'onpairingconsentreq' event.");

    let device = evt.device;
    if (!aSpecifiedBdAddress || device.address == aSpecifiedBdAddress) {
      evt.handle.accept().then(
        function onResolve() {
          log("  - 'accept' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'accept' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };
}







function cleanupPairingListener(aPairingReqs) {
  aPairingReqs.ondisplaypasskeyreq = null;
  aPairingReqs.onenterpasskeyreq = null;
  aPairingReqs.onpairingconfirmationreq = null;
  aPairingReqs.onpairingconsentreq = null;
}











function isUuidsEqual(aUuidArray1, aUuidArray2) {
  if (!Array.isArray(aUuidArray1) || !Array.isArray(aUuidArray2)) {
    return false;
  }

  if (aUuidArray1.length != aUuidArray2.length) {
    return false;
  }

  for (let i = 0, l = aUuidArray1.length; i < l; i++) {
    if (aUuidArray1[i] != aUuidArray2[i]) {
      return false;
    }
  }
  return true;
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
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
  startBluetoothTestBase([], function() {
    let origEnabled, needEnable;
    return Promise.resolve()
      .then(function() {
        origEnabled = getBluetoothEnabled();

        needEnable = !origEnabled;
        log("Original state of bluetooth is " + bluetoothManager.defaultAdapter.state);

        if (origEnabled && aReenable) {
          log("Disable Bluetooth ...");
          needEnable = true;

          isnot(bluetoothManager.defaultAdapter, null,
            "bluetoothManager.defaultAdapter")

          return bluetoothManager.defaultAdapter.disable();
        }
      })
      .then(function() {
        if (needEnable) {
          log("Enable Bluetooth ...");

          isnot(bluetoothManager.defaultAdapter, null,
            "bluetoothManager.defaultAdapter")

          return bluetoothManager.defaultAdapter.enable();
        }
      })
      .then(() => bluetoothManager.defaultAdapter)
      .then(aTestCaseMain)
      .then(function() {
        if (!origEnabled) {
          log("Disable Bluetooth ...");

          return bluetoothManager.defaultAdapter.disable();
        }
      });
  });
}
