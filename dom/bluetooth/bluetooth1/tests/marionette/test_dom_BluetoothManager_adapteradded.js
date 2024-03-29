



MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.name, EMULATOR_NAME, "adapter.name");
  is(aAdapter.class, EMULATOR_CLASS, "adapter.class");
  is(aAdapter.address, EMULATOR_ADDRESS, "adapter.address");
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");
  is(aAdapter.discoverableTimeout, 120, "adapter.discoverableTimeout");
});
