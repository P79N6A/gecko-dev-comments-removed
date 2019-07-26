





MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';










const EMULATOR_ADDRESS = "56:34:12:00:54:52";





const EMULATOR_NAME = "Full Android on Emulator";







const EMULATOR_CLASS = 0x58020c;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.name, EMULATOR_NAME, "adapter.name");
  is(aAdapter.class, EMULATOR_CLASS, "adapter.class");
  is(aAdapter.address, EMULATOR_ADDRESS, "adapter.address");
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");
  is(aAdapter.discoverableTimeout, 120, "adapter.discoverableTimeout");
});
