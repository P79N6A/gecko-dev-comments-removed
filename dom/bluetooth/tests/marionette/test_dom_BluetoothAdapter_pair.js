






















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
    .then(function(aRemoteAddress) {
      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "devicefound"));
      promises.push(startDiscovery(aAdapter));
      return Promise.all(promises)
        .then(function(aResults) {
          is(aResults[0].device.address, aRemoteAddress, "BluetoothDevice.address");
          return aResults[0].device.address;
        });
    })
    .then(function(aRemoteAddress) {
      let promises = [];
      promises.push(stopDiscovery(aAdapter));
      promises.push(waitForAdapterEvent(aAdapter, "pairedstatuschanged"));
      promises.push(pair(aAdapter, aRemoteAddress));
      return Promise.all(promises);
    })
    .then(() => getPairedDevices(aAdapter))
    .then((aPairedDevices) => unpair(aAdapter, aPairedDevices.pop().address))
    .then(() => removeEmulatorRemoteDevice(BDADDR_ALL));
});
