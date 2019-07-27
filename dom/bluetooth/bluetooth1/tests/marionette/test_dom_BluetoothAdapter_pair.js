




















MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function replyPairingReq(aAdapter, aPairingEvent) {
  switch (aPairingEvent.method) {
    case 'confirmation':
      log("The pairing passkey is " + aPairingEvent.passkey);
      aAdapter.setPairingConfirmation(aPairingEvent.address, true);
      break;
    case 'pincode':
      let pincode = BT_PAIRING_PINCODE;
      aAdapter.setPinCode(aPairingEvent.address, pincode);
      break;
    case 'passkey':
      let passkey = BT_PAIRING_PASSKEY;
      aAdapter.setPasskey(aPairingEvent.address, passkey);
      break;
    default:
      ok(false, "Unsupported pairing method. [" + aPairingEvent.method + "]");
  }
}

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Testing the pairing process of BluetoothAdapter ...");

  
  navigator.mozSetMessageHandler(BT_PAIRING_REQ,
    (evt) => replyPairingReq(aAdapter, evt));

  return Promise.resolve()
    .then(() => removeEmulatorRemoteDevice(BDADDR_ALL))
    .then(() => addEmulatorRemoteDevice())
    .then((aRemoteAddress) =>
          startDiscoveryAndWaitDevicesFound(aAdapter, [aRemoteAddress]))
    .then((aRemoteAddresses) =>
          stopDiscovery(aAdapter).then(() => aRemoteAddresses))
    
    
    
    
    .then((aRemoteAddresses) => pairDeviceAndWait(aAdapter, aRemoteAddresses.pop()))
    .then(() => getPairedDevices(aAdapter))
    .then((aPairedDevices) => unpair(aAdapter, aPairedDevices.pop().address))
    .then(() => removeEmulatorRemoteDevice(BDADDR_ALL));
});
