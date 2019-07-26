



















MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function testAdapterGetter(aAdapter, aPropertyName, aParamName, aExpected) {
  let cmd = "bt property " + BDADDR_LOCAL + " " + aParamName;
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      is(aResults[1], "OK", "The status report from emulator command.");
      log("  Got adapter " + aResults[0]);
      is(aResults[0], aParamName + ": " + aExpected, "BluetoothAdapter." + aPropertyName);
    });
}

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking the correctness of BluetoothAdapter properties ...");

  return Promise.resolve()
    .then(() => testAdapterGetter(aAdapter, "name",         "name",         aAdapter.name))
    .then(() => testAdapterGetter(aAdapter, "address",      "address",      aAdapter.address))
    .then(() => testAdapterGetter(aAdapter, "class",        "cod",          "0x" + aAdapter.class.toString(16)))
    .then(() => testAdapterGetter(aAdapter, "discoverable", "discoverable", aAdapter.discoverable.toString()))
    .then(() => testAdapterGetter(aAdapter, "discovering",  "discovering",  aAdapter.discovering.toString()));
});
