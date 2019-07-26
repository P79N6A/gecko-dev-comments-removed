

















MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Testing the discovery process of BluetoothAdapter ...");

  return Promise.resolve()
    .then(() => removeEmulatorRemoteDevice(BDADDR_ALL))
    .then(() => addEmulatorRemoteDevice(null))
    .then(function(aRemoteAddress) {
      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "devicefound"));
      promises.push(startDiscovery(aAdapter));
      return Promise.all(promises)
        .then(function(aResults) {
          is(aResults[0].device.address, aRemoteAddress, "BluetoothDevice.address");
        });
    })
    .then(() => stopDiscovery(aAdapter))
    .then(() => removeEmulatorRemoteDevice(BDADDR_ALL));
});
