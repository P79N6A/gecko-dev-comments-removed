
















































MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const ADDRESS_OF_TARGETED_REMOTE_DEVICE = "0c:bd:51:20:a1:e0";

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  return Promise.resolve()
    .then(function() {
      log("[1] Unpair device if it's already paired ... ");
      let devices = aAdapter.getPairedDevices();
      for (let i in devices) {
        if (devices[i].address == ADDRESS_OF_TARGETED_REMOTE_DEVICE) {
          log("  - The device has already been paired. Unpair it ...");
          return aAdapter.unpair(devices[i].address);
        }
      }
      log("  - The device hasn't been paired. Skip to Step [2] ...");
      return;
    })
    .then(function() {
      log("[2] Start discovery ... ");
      return aAdapter.startDiscovery();
    })
    .then(function(discoveryHandle) {
      log("[3] Wait for the specified 'devicefound' event ... ");
      return waitForSpecifiedDevicesFound(discoveryHandle, [ADDRESS_OF_TARGETED_REMOTE_DEVICE]);
    })
    .then(function(deviceEvents) {
      log("[4] Type checking for BluetoothDeviceEvent and BluetoothDevice ... ");

      let device = deviceEvents[0].device;
      ok(deviceEvents[0] instanceof BluetoothDeviceEvent, "device should be a BluetoothDeviceEvent");
      ok(device instanceof BluetoothDevice, "device should be a BluetoothDevice");

      log("  - BluetoothDevice.address: " + device.address);
      log("  - BluetoothDevice.name: " + device.name);
      log("  - BluetoothDevice.cod: " + device.cod);
      log("  - BluetoothDevice.paired: " + device.paired);
      log("  - BluetoothDevice.uuids: " + device.uuids);

      return device;
    })
    .then(function(device) {
      log("[5] Pair and wait for confirmation ... ");

      addEventHandlerForPairingRequest(aAdapter, device.address);

      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "devicepaired"));
      promises.push(aAdapter.pair(device.address));
      return Promise.all(promises);
    })
    .then(function(results) {
      log("[6] Get paired devices and verify 'devicepaired' event ... ");
      return verifyDevicePairedUnpairedEvent(aAdapter, results[0]);
    })
    .then(function(deviceEvent) {
      log("[7] Pair again... ");
      return aAdapter.pair(deviceEvent.device.address).then(deviceEvent.device.address);
    })
    .then(function(deviceAddress) {
      log("[8] Unpair ... ");
      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "deviceunpaired"));
      promises.push(aAdapter.unpair(deviceAddress));
      return Promise.all(promises);
    })
    .then(function(results) {
      log("[9] Get paired devices and verify 'deviceunpaired' event ... ");
      return verifyDevicePairedUnpairedEvent(aAdapter, results[0])
    })
    .then(function(deviceEvent) {
      log("[10] Unpair again... ");
      return aAdapter.unpair(deviceEvent.address);
    })
    .then(function() {
      log("[11] Stop discovery ... ");
      return aAdapter.stopDiscovery();
    });
});
