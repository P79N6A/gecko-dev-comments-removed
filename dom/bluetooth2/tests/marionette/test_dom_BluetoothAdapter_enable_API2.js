





















MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");
  
  
  

  log("  adapter.address: " + aAdapter.address);
  log("  adapter.name: " + aAdapter.name);

  let originalAddr = aAdapter.address;
  let originalName = aAdapter.name;

  return Promise.resolve()
    .then(function() {
      log("[1] Disable Bluetooth and check the correctness of 'onattributechanged'");
      let promises = [];
      promises.push(waitForAdapterStateChanged(aAdapter, ["disabling", "disabled"]));
      promises.push(aAdapter.disable());
      return Promise.all(promises);
    })
    .then(function(aResults) {
      isnot(aResults[0].indexOf("address"), -1, "Indicator of 'address' changed event");
      if (originalName != "") {
        isnot(aResults[0].indexOf("name"), -1, "Indicator of 'name' changed event");
      }
      is(aAdapter.address, "", "adapter.address");
      is(aAdapter.name, "", "adapter.name");
    })
    .then(function() {
      log("[2] Enable Bluetooth and check the correctness of 'onattributechanged'");
      let promises = [];
      promises.push(waitForAdapterStateChanged(aAdapter, ["enabling", "enabled"]));
      promises.push(aAdapter.enable());
      return Promise.all(promises);
    })
    .then(function(aResults) {
      isnot(aResults[0].indexOf("address"), -1, "Indicator of 'address' changed event");
      if (originalName != "") {
        isnot(aResults[0].indexOf("name"), -1, "Indicator of 'name' changed event");
      }
      is(aAdapter.address, originalAddr, "adapter.address");
      is(aAdapter.name, originalName, "adapter.name");
    })
});
