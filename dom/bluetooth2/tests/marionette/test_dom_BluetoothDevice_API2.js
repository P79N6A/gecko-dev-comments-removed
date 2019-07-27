


































MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const EXPECTED_NUMBER_OF_REMOTE_DEVICES = 1;

let hasReceivedUuidsChanged = false;
let originalUuids;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable at step [0]");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  return Promise.resolve()
    .then(function(discoveryHandle) {
      log("[1] Start discovery ... ");
      return aAdapter.startDiscovery();
    })
    .then(function(discoveryHandle) {
      log("[2] Wait for 'devicefound' events ... ");
      return waitForDevicesFound(discoveryHandle, EXPECTED_NUMBER_OF_REMOTE_DEVICES);
    })
    .then(function(deviceEvents) {
      log("[3] Type checking for BluetoothDeviceEvent and BluetoothDevice ... ");

      for (let i in deviceEvents) {
        let deviceEvt = deviceEvents[i];
        is(deviceEvt.address, null, "deviceEvt.address");
        isnot(deviceEvt.device, null, "deviceEvt.device");
        ok(deviceEvt.device instanceof BluetoothDevice,
          "deviceEvt.device should be a BluetoothDevice");

        let device = deviceEvt.device;
        ok(typeof device.address === 'string', "type checking for address.");
        ok(typeof device.name === 'string', "type checking for name.");
        ok(device.cod instanceof BluetoothClassOfDevice, "type checking for cod.");
        ok(typeof device.paired === 'boolean', "type checking for paired.");
        ok(Array.isArray(device.uuids), "type checking for uuids.");

        originalUuids = device.uuids;

        log("  - BluetoothDevice.address: " + device.address);
        log("  - BluetoothDevice.name: " + device.name);
        log("  - BluetoothDevice.cod: " + device.cod);
        log("  - BluetoothDevice.paired: " + device.paired);
        log("  - BluetoothDevice.uuids: " + device.uuids);
      }
      return deviceEvents[0].device;
    })
    .then(function(device) {
      log("[4] Attach the 'onattributechanged' handler for the remote device ... ");
      device.onattributechanged = function(aEvent) {
        for (let i in aEvent.attrs) {
          switch (aEvent.attrs[i]) {
            case "cod":
              log("  'cod' changed to " + device.cod);
              break;
            case "name":
              log("  'name' changed to " + device.name);
              break;
            case "paired":
              log("  'paired' changed to " + device.paired);
              break;
            case "uuids":
              log("  'uuids' changed to " + device.uuids);
              hasReceivedUuidsChanged = true;
              break;
            case "unknown":
            default:
              ok(false, "Unknown attribute '" + aEvent.attrs[i] + "' changed." );
              break;
          }
        }
      };
      return device;
    })
    .then(function(device) {
      log("[5] Fetch the UUIDs of remote device ... ");
      let promises = [];
      promises.push(Promise.resolve(device));
      promises.push(device.fetchUuids());
      return Promise.all(promises);
    })
    .then(function(aResults) {
      log("[6] Verify the UUIDs ... ");
      let device = aResults[0];
      let uuids = aResults[1];

      ok(Array.isArray(uuids), "type checking for 'fetchUuids'.");

      ok(isUuidsEqual(uuids, device.uuids),
        "device.uuids should equal to the result from 'fetchUuids'");

      bool isUuidsChanged = !isUuidsEqual(originalUuids, device.uuids);
      is(isUuidsChanged, hasReceivedUuidsChanged, "device.uuids has changed.");

      device.onattributechanged = null;
    })
    .then(function() {
      log("[7] Stop discovery ... ");
      return aAdapter.stopDiscovery();
    })
});
