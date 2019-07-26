





MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

let enabledEventReceived;
function onEnabled() {
  enabledEventReceived = true;
}

let disabledEventReceived;
function onDisabled() {
  disabledEventReceived = true;
}

function test(aEnabled) {
  log("Testing 'bluetooth.enabled' => " + aEnabled);

  let deferred = Promise.defer();

  enabledEventReceived = false;
  disabledEventReceived = false;

  setBluetoothEnabled(aEnabled).then(function() {
    log("  Settings set. Waiting 3 seconds and examine results.");

    window.setTimeout(function() {
      is(bluetoothManager.enabled, aEnabled, "bluetoothManager.enabled");
      is(enabledEventReceived, aEnabled, "enabledEventReceived");
      is(disabledEventReceived, !aEnabled, "disabledEventReceived");

      if (bluetoothManager.enabled === aEnabled &&
          enabledEventReceived === aEnabled &&
          disabledEventReceived === !aEnabled) {
        deferred.resolve();
      } else {
        deferred.reject();
      }
    }, 3000);
  });

  return deferred.promise;
}

startBluetoothTestBase(["settings-read", "settings-write"],
                       function testCaseMain() {
  bluetoothManager.addEventListener("enabled", onEnabled);
  bluetoothManager.addEventListener("disabled", onDisabled);

  return getBluetoothEnabled()
    .then(function(aEnabled) {
      log("Original 'bluetooth.enabled' is " + aEnabled);
      
      return test(!aEnabled).then(test.bind(null, aEnabled));
    })
    .then(function() {
      bluetoothManager.removeEventListener("enabled", onEnabled);
      bluetoothManager.removeEventListener("disabled", onDisabled);
    });
});
