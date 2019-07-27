



MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function waitEitherEnabledOrDisabled() {
  let deferred = Promise.defer();

  function onEnabledDisabled(aEvent) {
    bluetoothManager.removeEventListener("adapteradded", onEnabledDisabled);
    bluetoothManager.removeEventListener("disabled", onEnabledDisabled);

    ok(true, "Got event " + aEvent.type);
    deferred.resolve(aEvent.type === "adapteradded");
  }

  
  
  
  
  bluetoothManager.addEventListener("adapteradded", onEnabledDisabled);
  bluetoothManager.addEventListener("disabled", onEnabledDisabled);

  return deferred.promise;
}

function test(aEnabled) {
  log("Testing 'bluetooth.enabled' => " + aEnabled);

  let deferred = Promise.defer();

  
  
  
  let promises = [];
  promises.push(waitEitherEnabledOrDisabled());
  promises.push(setBluetoothEnabled(aEnabled));
  Promise.all(promises)
    .then(function(aResults) {
      



      log("  Examine results " + JSON.stringify(aResults));

      is(bluetoothManager.enabled, aEnabled, "bluetoothManager.enabled");
      is(aResults[0], aEnabled, "'adapteradded' event received");

      if (bluetoothManager.enabled === aEnabled && aResults[0] === aEnabled) {
        deferred.resolve();
      } else {
        deferred.reject();
      }
    });

  return deferred.promise;
}

startBluetoothTestBase(["settings-read", "settings-write", "settings-api-read", "settings-api-write"],
                       function testCaseMain() {
  return getBluetoothEnabled()
    .then(function(aEnabled) {
      log("Original 'bluetooth.enabled' is " + aEnabled);
      
      return test(!aEnabled).then(test.bind(null, aEnabled));
    });
});
