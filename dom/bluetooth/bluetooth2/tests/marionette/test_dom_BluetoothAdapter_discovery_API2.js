
































MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const EXPECTED_NUMBER_OF_REMOTE_DEVICES = 2;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  let discoveryHandle = null;
  return Promise.resolve()
    .then(function() {
      log("[1] Start discovery and verify the correctness ... ");
      let promises = [];
      promises.push(waitForAdapterAttributeChanged(aAdapter, "discovering", true));
      promises.push(aAdapter.startDiscovery());
      return Promise.all(promises);
    })
    .then(function(aResults) {
      log("[2] Attach event handler for 'ondevicefound' ... ");
      discoveryHandle = aResults[1];
      isHandleExpired = false;
      discoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
        if (isHandleExpired) {
          ok(false, "Expired BluetoothDiscoveryHandle received an event.");
        }
      };
    })
    .then(function() {
      log("[3] Stop discovery and and verify the correctness ... ");
      let promises = [];
      if (aAdapter.discovering) {
        promises.push(waitForAdapterAttributeChanged(aAdapter, "discovering", false));
      }
      promises.push(aAdapter.stopDiscovery());
      return Promise.all(promises);
    })
    .then(function() {
      log("[4] Mark the BluetoothDiscoveryHandle from [1] as expired ... ");
      isHandleExpired = true;
    })
    .then(function() {
      log("[5] Start discovery and verify the correctness ... ");
      let promises = [];
      promises.push(waitForAdapterAttributeChanged(aAdapter, "discovering", true));
      promises.push(aAdapter.startDiscovery());
      return Promise.all(promises);
    })
    .then(function(aResults) {
      log("[6] Wait for 'devicefound' events ... ");
      return waitForDevicesFound(aResults[1], EXPECTED_NUMBER_OF_REMOTE_DEVICES);
    })
    .then(function() {
      log("[7] Stop discovery and and verify the correctness ... ");
      let promises = [];
      if (aAdapter.discovering) {
        promises.push(waitForAdapterAttributeChanged(aAdapter, "discovering", false));
      }
      promises.push(aAdapter.stopDiscovery());
      return Promise.all(promises);
    })
    .then(function() {
      log("[8] Call 'startDiscovery' twice continuously ... ");
      return aAdapter.startDiscovery()
        .then(() => aAdapter.startDiscovery())
        .then(() => ok(false, "Call startDiscovery() when adapter is discovering. - Fail"),
              () => ok(true, "Call startDiscovery() when adapter is discovering. - Success"));
    })
    .then(function() {
      log("[9] Call 'stopDiscovery' twice continuously ... ");
      return aAdapter.stopDiscovery()
        .then(() => aAdapter.stopDiscovery())
        .then(() => ok(true, "Call stopDiscovery() when adapter isn't discovering. - Success"),
              () => ok(false, "Call stopDiscovery() when adapter isn't discovering. - Fail"));
    })
    .then(function() {
      log("[10] Clean up the event handler of [2] ... ");
      if (discoveryHandle) {
        discoveryHandle.ondevicefound = null;
      }
    });
});
