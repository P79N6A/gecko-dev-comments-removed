


const { interfaces: Ci, classes: Cc, utils: Cu } = Components;

function notify() {
  
  console.log({ msg: "Hello again" });
}

function startup(aParams, aReason) {
  Cu.import("resource://gre/modules/Services.jsm");
  let res = Services.io.getProtocolHandler("resource")
                       .QueryInterface(Ci.nsIResProtocolHandler);
  res.setSubstitution("browser_dbg_addon4", aParams.resourceURI);

  
  Cu.import("resource://browser_dbg_addon4/test.jsm");
  
  console.log({ msg: "Hello from the test add-on" });

  Services.obs.addObserver(notify, "addon-test-ping", false);
}

function shutdown(aParams, aReason) {
  Services.obs.removeObserver(notify, "addon-test-ping");

  
  Cu.unload("resource://browser_dbg_addon4/test.jsm");

  let res = Services.io.getProtocolHandler("resource")
                       .QueryInterface(Ci.nsIResProtocolHandler);
  res.setSubstitution("browser_dbg_addon4", null);
}
