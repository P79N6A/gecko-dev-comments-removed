

















MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';







function waitForManagerAttributeChanged() {
  let deferred = Promise.defer();

  bluetoothManager.onattributechanged = function(aEvent) {
    if(aEvent.attrs.indexOf("defaultAdapter")) {
      bluetoothManager.onattributechanged = null;
      ok(true, "BluetoothManager event 'onattributechanged' got.");
      deferred.resolve(aEvent);
    }
  };

  return deferred.promise;
}

startBluetoothTestBase(["settings-read", "settings-write", "settings-api-read", "settings-api-write"],
                       function testCaseMain() {
  let adapters = bluetoothManager.getAdapters();
  ok(Array.isArray(adapters), "Can not got the array of adapters");
  ok(adapters.length, "The number of adapters should not be zero");
  ok(bluetoothManager.defaultAdapter, "defaultAdapter should not be null.");
});
